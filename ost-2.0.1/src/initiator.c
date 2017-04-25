
/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to this
 * license. If you do not agree to this license, do not download, install, copy
 * or use the software. Intel License Agreement
 *
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * -Redistributions of source code must retain the above copyright notice, this
 *  list of conditions and the following disclaimer.
 *
 * -Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * -The name of Intel Corporation may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/tcp.h>
#include <asm/uaccess.h>
#include <linux/utsname.h>
#include <net/sock.h>
#include "driver.h"
#define PRIu64 "llu"
#else
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#endif
#include "initiator.h"

/*
 * Globals
 */

static ISCSI_LOCK_T g_lock[CONFIG_INITIATOR_MAX_TARGETS];
static void *g_cookie[CONFIG_INITIATOR_MAX_TARGETS];
static ISCSI_WORKER_T g_session_worker[CONFIG_INITIATOR_MAX_TARGETS];
static INITIATOR_TARGET_T g_target[CONFIG_INITIATOR_MAX_TARGETS];
static int g_num_targets = 0;
static unsigned g_tag = 0xabc123;
static ISCSI_SPIN_LOCK_T g_tag_lock[CONFIG_INITIATOR_MAX_TARGETS];
static ISCSI_WORKER_T g_session_manager;
static ISCSI_QUEUE_T g_session_q;
static int g_initiator_state;
#ifndef __KERNEL__
static ISCSI_QUEUE_T g_alarm_q;
#endif
static char g_hostname[256];
static HASH_T g_cmd_hash[CONFIG_INITIATOR_MAX_TARGETS];
static int g_verbose;
static int g_nop_out_waiting[CONFIG_INITIATOR_MAX_TARGETS];
static ISCSI_NOP_OUT_T g_nop_out[CONFIG_INITIATOR_MAX_TARGETS];
#ifdef __KERNEL__
static int g_reported = 0;
#endif
static int g_family;

#if 0
#ifdef CONFIG_EXPERIMENTAL
static int g_ramdisk_bypass = 1;
static int g_ramdisk_failure = 0;
static uint64_t g_ramdisk_size = 0;
static unsigned char *g_ramdisk = NULL;
int g_pinning=0;
#endif
#endif

#if CONFIG_DRIVER_RAMDISK == 1
extern uint64_t g_ramdisk_num_blocks;
extern void *g_ramdisk_ptr;
extern unsigned g_ramdisk_blocklen;
#endif

/*
 * Session manager functions
 */

static int session_manager_proc(void *arg);
static int login_phase_i(INITIATOR_SESSION_T *sess, char *text, int text_len);
static int logout_phase_i(INITIATOR_SESSION_T *sess);

/*
 * Tx functions.  INITIATOR_CMD_T pointers are enqueued to the Tx worker
 * for a given session by the enqueue worker.  The Tx worker will send
 * out these commands and wait for the Rx worker to process the response.
 *  The pointer is inserted into the hashtable g_tag_hash, keyed by the
 * initiator tag of the iSCSI commands.
 */

static ISCSI_QUEUE_T g_tx_queue[CONFIG_INITIATOR_MAX_TARGETS];
static int tx_worker_proc_i(void *arg);
static int text_command_i(INITIATOR_CMD_T *cmd);
static int login_command_i(INITIATOR_CMD_T *cmd);
static int logout_command_i(INITIATOR_CMD_T *cmd);
static int scsi_command_i(INITIATOR_CMD_T *cmd);
static int nop_out_i(INITIATOR_CMD_T *cmd);

/* 
 * Rx functions. Upon receipt of an incoming PDU, the Rx worker will first
 * extract the tag (if it exists for the PDU) and then the associated 
 * INITIATOR_CMD_T pointer stored in the hash table.  One of Rx functions
 * will be called to processs the PDU. The Rx worker will invoke the callback
 * function associated with the command once the command has been retired.
 */

static int rx_worker_proc_i(void *arg);
static int login_response_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd, 
                            unsigned char *header);
static int text_response_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd, 
                           unsigned char *header);
static int logout_response_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd,
                             unsigned char *header);
static int scsi_response_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd, 
                           unsigned char *header);
static int scsi_read_data_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd, 
                            unsigned char *header);
static int scsi_r2t_rx_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd, 
                         unsigned char *header);
static int scsi_r2t_tx_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd);
static int nop_in_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd, 
                    unsigned char *header);
static int reject_i(INITIATOR_SESSION_T *sess, unsigned char *header);
static int async_msg_i(INITIATOR_SESSION_T *sess, unsigned char *header);

/*
 * Misc. Prototypes
 */

static int session_init_i(INITIATOR_SESSION_T **sess, uint64_t tid);
static int wait_callback_i(void *arg);
static int get_target_config(char *filename);
static int iscsi_done_i(INITIATOR_CMD_T *cmd);

/*
 * Private Functions
 */

/**
 * Configure iSCSI targets.
 */
static int get_target_config (char *filename) {
    int tid = -1;
    char *buffer = NULL;
    int bytes = 0;
    long file_length = 0;
    char *ptr, *ptr2;
#ifdef __KERNEL__ 
    mm_segment_t oldfs;
#endif
    HAND fh = HAND_INIT; 

    /* open configuration file */
    if (!strlen(filename)) {
        TRACE_ERROR("target config filename is NULL\n");
        return -1;
    }

    fh = OPEN(filename, O_RDONLY, 0);
    if (IS_ERR(fh)) {
        TRACE_ERROR("error opening \"%s\"\n", filename);
        return -1;
    }

#ifdef __KERNEL__ 
    if (!fh->f_op || !fh->f_op->read) {
        TRACE_ERROR("File handle has no operations registered\n");
        CLOSE(fh, NULL);
        return -1;
    }
    fh->f_pos =0;
#if 0
    if (!fh->f_op->llseek) {
        TRACE_ERROR("fh has no llseek\n");
        return -1;
    }
#endif
#endif

#if 0
    file_length = SEEK(fh, 0L, 2);
    PRINT("Target configuration file length is:%lu\n", file_length);
    if (file_length== 0) {
        CLOSE(fh, NULL);
        return -1;
    }
    SEEK(fh, 0L, 0);
#else
    file_length = 65536;
#endif
    buffer = (char*) iscsi_malloc (file_length+1);
    if (buffer == NULL) {
        TRACE(TRACE_DEBUG, 0,
              "get_target_config(): iscsi_malloc returned NULL\n");
        CLOSE(fh, NULL);
        return -1;
    }

#ifdef __KERNEL__ 
    oldfs= get_fs ();
    set_fs (KERNEL_DS);
    bytes = fh->f_op->read (fh, buffer, file_length, &fh->f_pos);
    set_fs (oldfs);
#else
    bytes = read (fh, buffer, file_length);
#endif
    CLOSE(fh, NULL) ;

    buffer[bytes] = '\0';
    TRACE(TRACE_DEBUG, 0,
          " number of  bytes read from target configuration file:%d\n", bytes);
    TRACE(TRACE_DEBUG, 0," target configuration file contents :%s\n", buffer);

    /* remove all c-style comments */
    ptr = buffer;
    while (ptr) {
        if ((ptr=strstr(buffer, "/*"))) {
            if ((ptr2=strstr(ptr+1, "*/"))) {
                memcpy(ptr, ptr2+2, buffer+file_length-ptr2-2);
                file_length -= (ptr2-ptr)+2;
                buffer[file_length] = '\0';
            }
        }
        ptr = strstr(buffer, "/*");
    }    

    /* remove all shell-style comments */
    ptr = buffer;
    while (ptr) {
        if ((ptr=strchr(buffer, '#'))) {
            if ((ptr2=strchr(ptr+1, '\n'))) {
                memcpy(ptr, ptr2+1, buffer+file_length-ptr2-1);
                file_length -= (ptr2-ptr)+1;
                buffer[file_length] = '\0';
            } else {
                file_length -= strlen(ptr);
                buffer[file_length] = '\0';
            }
        }
        ptr = strchr(buffer, '#');
    }

#define ASSIGN_INT_FROM_INT(TID, VAR, INT) {                    \
        if (TID==-1) {                                          \
            int j;                                              \
            for (j=0; j<CONFIG_INITIATOR_MAX_TARGETS; j++) {    \
                g_target[j].VAR = INT;                          \
            }                                                   \
        } else {                                                \
            g_target[TID].VAR = INT;                            \
        }}

#define ASSIGN_INT(TID, VAR, PTR, LEN) {                        \
        if (TID==-1) {                                          \
            int j;                                              \
            for (j=0; j<CONFIG_INITIATOR_MAX_TARGETS; j++) {    \
                g_target[j].VAR = iscsi_atoi(PTR+LEN);          \
            }                                                   \
        } else {                                                \
            g_target[TID].VAR = iscsi_atoi(PTR+LEN);            \
        }}

#define ASSIGN(TID, VAR, PTR, LEN) {                            \
        if (TID==-1) {                                          \
            int j;                                              \
            for (j=0; j<CONFIG_INITIATOR_MAX_TARGETS; j++) {    \
                strncpy(g_target[j].VAR, PTR+LEN, MAX_LEN-LEN); \
            }                                                   \
        } else {                                                \
            strncpy(g_target[TID].VAR, PTR+LEN, MAX_LEN-LEN);   \
        }}

    /*
     * Assign any defaults here. When tid is -1, ASSIGN will
     * assign the value to all targets.
     */
    ASSIGN_INT_FROM_INT(-1, port, ISCSI_PORT);

    /* tokenize and read fields */
    if (buffer) {
        char* token;
        int commented = 0;

        ptr = buffer; token = strsep (&ptr," \n");
        while (token!=NULL) {
            if (!commented) {
                if (strstr (token, "ip=")) {
                    tid++; 
                    if (tid > CONFIG_INITIATOR_MAX_TARGETS) {
                        TRACE_ERROR("CONFIG_INITIATOR_MAX_TARGETS is %i\n", 
                                    CONFIG_INITIATOR_MAX_TARGETS);
                        return -1;
                    }
                    ASSIGN(tid, ip, token, 3);
                }
		else if (strstr (token, "fix=")) 
		    ASSIGN_INT(tid, fix_address, token, 4)
		    else if (strstr (token, "port=")) 
			ASSIGN_INT(tid, port, token, 5)
			else if (strstr (token, "which=")) 
			    ASSIGN(tid, which, token, 6)
			    else if (strstr (token, "TargetUser=")) 
				ASSIGN(tid, TargetUser, token, 11)
				else if (strstr (token, "TargetPass=")) 
				    ASSIGN(tid, TargetPass, token, 11)
				    else if (strstr (token, "InitiatorUser=")) 
					ASSIGN(tid, InitiatorUser, token, 14)
					else if (strstr (token, "InitiatorPass=")) 
					    ASSIGN(tid, InitiatorPass, token, 14)
					    else if (strstr (token, "ImmediateData=")) 
						ASSIGN(tid, ImmediateData, token, 14)
						else if (strstr (token, "InitialR2T=")) 
						    ASSIGN(tid, InitialR2T, token, 11)
						    else if (strstr (token, "MaxRecvDataSegmentLength=")) 
							ASSIGN(tid, MaxRecvDataSegmentLength, token, 25)
							else if (strstr (token, "FirstBurstLength=")) 
							    ASSIGN(tid, FirstBurstLength, token, 17)
							    else if (strstr (token, "MaxBurstLength=")) 
								ASSIGN(tid, MaxBurstLength, token, 15)
								else if (strstr (token, "TargetName=")) 
								    ASSIGN(tid, TargetName, token, 11)
#if CONFIG_DRIVER_RAMDISK == 1
								    else if (strstr (token, "ramdisk=")) { 
									g_ramdisk_num_blocks = iscsi_atoi(token+8);
									//PRINT("iscsi: ramdisk=%"PRIu64"\n", 
									//    g_ramdisk_num_blocks);
								    }
#endif
								    else if (strlen (token) && !strstr(token, "ramdisk=")) 
									TRACE_ERROR("ignoring token \"%s\" in config file\n",
										    token);
            }
            token= strsep (&ptr, " \n");
        }
        iscsi_free (buffer);
    }
    g_num_targets = tid+1;

#if CONFIG_DRIVER_RAMDISK == 1
    if (g_ramdisk_num_blocks) {
	if ((g_ramdisk_ptr = vmalloc(g_ramdisk_num_blocks * g_ramdisk_blocklen))) {
	    PRINT("iscsi: allocated RAMDISK (%"PRIu64" blocks, %"PRIu64" MB)\n", 
		  g_ramdisk_num_blocks, (g_ramdisk_num_blocks * g_ramdisk_blocklen)>>20);
	} else {
	    PRINT("iscsi: *** FAILED TO ALLOCATE RAMDISK ***\n");
	}
    }
#endif

    return 0;
}  

static int session_init_i(INITIATOR_SESSION_T **sess, uint64_t isid) {
    INITIATOR_SESSION_T *s;
    ISCSI_PARAMETER_T **l;
    int one = 1;

    /* Get free session */
    QUEUE_REMOVE_ELSE(&g_session_q, s, return -1);
    memset(s, 0, sizeof(INITIATOR_SESSION_T));
    s->isid = isid;

    /* Hash of outstanding commands */
    if (hash_init(&s->cmds, CONFIG_INITIATOR_QUEUE_DEPTH)!=0) {
        TRACE_ERROR("hash_init() failed\n"); 
        return -1;
    }   

    /* Create socket */
    if (iscsi_sock_create(&s->sock, AF_INET)!=0) {
        TRACE_ERROR("iscsi_sock_create() failed\n");
        return -1;
    }
#if 1
#ifdef __arm__
    if (iscsi_sock_setsockopt(&s->sock, 6, TCP_NODELAY, &one, sizeof(one))!=0) {
        TRACE_ERROR("iscsi_sock_setsockopt() failed\n");
        return -1;
    }
#else
    if (iscsi_sock_setsockopt(&s->sock, SOL_TCP, TCP_NODELAY, &one, 
                              sizeof(one))!=0) {
        TRACE_ERROR("iscsi_sock_setsockopt() failed\n");
        return -1;
    }
#endif
#endif

    /*
     * ISCSI_PARAM_TYPE_LIST format: <type> <key> <dflt> <valid list values>
     * ISCSI_PARAM_TYPE_BINARY format: <type> <key> <dflt> <valid binary values>
     * ISCSI_PARAM_TYPE_NUMERICAL format: <type> <key> <dflt> <max>
     * ISCSI_PARAM_TYPE_DECLARATIVE format: <type> <key> <dflt> ""
     */

    s->params = NULL; l = &(s->params);

    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_LIST,
                        "AuthMethod", "None", "None,CHAP", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_LIST,        
                        "CHAP_A", "None", "5", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,   
                        "CHAP_N", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,  
                        "CHAP_R", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,   
                        "CHAP_I", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,   
                        "CHAP_C", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_LIST,          
                        "HeaderDigest", "None", "None", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_LIST,          
                        "DataDigest", "None", "None", return -1); 
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL,     
                        "MaxConnections", "1", "1", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,   
                        "SendTargets", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARE_MULTI, 
                        "TargetName", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,   
                        "InitiatorName", g_hostname, "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,   
                        "TargetAlias", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,   
                        "InitiatorAlias", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARE_MULTI, 
                        "TargetAddress", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_BINARY_OR,     
                        "InitialR2T", "Yes", "Yes,No", return -1);  
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_BINARY_AND,    
                        "ImmediateData", "Yes", "Yes,No", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL_Z,   
                        "MaxBurstLength", "262144", "1048576", return -1); 
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL_Z,   
                        "FirstBurstLength", "65536", "1048576", return -1); 
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL_Z,   
                        "MaxRecvDataSegmentLength", "8192", "65536", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,   
                        "TargetPortalGroupTag", "1", "65535" , return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL,     
                        "DefaultTime2Wait", "2", "2", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL,     
                        "DefaultTime2Retain", "20", "20", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL,     
                        "MaxOutstandingR2T", "1", "1", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_BINARY_OR,     
                        "DataPDUInOrder", "Yes", "Yes,No", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_BINARY_OR,     
                        "DataSequenceInOrder", "Yes", "Yes,No", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL,     
                        "ErrorRecoveryLevel", "0", "0", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,   
                        "SessionType", "Normal", "Normal,Discovery", return -1);
    g_target[isid].state |= SESSION_STATE_INITIALIZED;

    /* start Tx worker */
    *sess = s;
    START_WORKER_ELSE("iscsi tx", isid, s->tx_worker, tx_worker_proc_i, 
                      NULL, goto failure);
    return 0;

 failure:
    g_target[isid].state |= SESSION_STATE_INITIALIZED | SESSION_STATE_ERROR;
    return -1;
}

int session_destroy_i(INITIATOR_SESSION_T **sess_ptr) {
    INITIATOR_CMD_T *cmd;
    uint64_t isid;
    INITIATOR_SESSION_T *sess = *sess_ptr;
    int aborted = 0;
    int requeued = 0;
    int attempts = 3;
    static ISCSI_QUEUE_T tmp_queue;

    if (*sess_ptr == NULL) return 0;

    /* make sure nothing else is destroying this */
    isid = sess->isid;
    ISCSI_LOCK_ELSE(&g_lock[isid], return -1);
    if (!g_target[isid].state) {
        TRACE_ERROR("session %"PRIu64" already destroyed??\n", isid);
        ISCSI_UNLOCK_ELSE(&g_lock[isid], return -1);
        return -1;
    }
    if (g_target[isid].state & SESSION_STATE_DESTROYING) {
        TRACE_ERROR("session %"PRIu64" being destroyed??\n", isid);
        ISCSI_UNLOCK_ELSE(&g_lock[isid], return -1);
        return -1;
    }
    g_target[isid].state |= SESSION_STATE_DESTROYING;
    ISCSI_UNLOCK_ELSE(&g_lock[isid], return -1);

    /* wait for session to finish initializing */
    while (!(g_target[isid].state & SESSION_STATE_INITIALIZED)&&attempts--) {
        TRACE_ERROR("session %"PRIu64" still initializing (%i attempts "
                    "left)\n", isid, attempts);
        ISCSI_SLEEP(1);
    }
    if (attempts==0) {
        TRACE_ERROR("session %"PRIu64" never finished initializing?? -- "
                    "giving up\n", isid);
    }
    
    /* stop tx worker */

    STOP_WORKER_ELSE(sess->tx_worker, 
                     (iscsi_sock_shutdown(sess->sock, 
                                          ISCSI_SOCK_SHUTDOWN_SEND)==0), 
                     return -1);
    
    /* look through the tx queue for commands to abort */
    QUEUE_INIT_ELSE(&tmp_queue, CONFIG_INITIATOR_QUEUE_DEPTH, return -1);
    while ((cmd = iscsi_queue_remove(&g_tx_queue[isid]))) {
        if ((cmd->type == ISCSI_LOGIN_CMD)||(cmd->type == ISCSI_TEXT_CMD)
            ||(cmd->type == ISCSI_LOGOUT_CMD)) {
            TRACE_ERROR("cmd 0x%x in tx queue -- ABORTING\n", cmd->tag);
            EQUAL_ELSE("initiator_abort", initiator_abort(cmd), 0, return -1);
            aborted++;
        } else {
            QUEUE_INSERT_ELSE(&tmp_queue, cmd, return -1);
        }
    }
    while ((cmd = iscsi_queue_remove(&tmp_queue))) {
        QUEUE_INSERT_ELSE(&g_tx_queue[isid], cmd, return -1);
    }

    /* requeue any SCSI commands */
    while ((cmd=hash_remove_head(&g_cmd_hash[isid]))) {
        if ((cmd->type == ISCSI_LOGIN_CMD)||(cmd->type == ISCSI_TEXT_CMD)
            ||(cmd->type == ISCSI_LOGOUT_CMD)) {
            TRACE(TRACE_SESSION, 0, "cmd 0x%x outstanding -- ABORTING\n", 
                  cmd->tag);
            EQUAL_ELSE("initiator_abort", initiator_abort(cmd), 0, return -1);
            aborted++;
        } else {
            TRACE(TRACE_SESSION, 0, 
                  "tid %"PRIu64" cmd 0x%x type %i outstanding - REQUEUING\n", 
                  isid, cmd->tag, cmd->type);
            cmd->r2t_request = 0;
            cmd->tx_done = 0;
            cmd->status = 0;
            cmd->done = 0;
            cmd->aborted = 0;
            if (cmd->type == ISCSI_SCSI_CMD) {
                ((ISCSI_SCSI_CMD_T *)(cmd->ptr))->bytes_todev = 0;
                ((ISCSI_SCSI_CMD_T *)(cmd->ptr))->bytes_fromdev = 0;
            }
            QUEUE_INSERT_ELSE(&g_tx_queue[isid], cmd, return -1);
        } 
    }
    if (aborted+requeued)
        TRACE(TRACE_SESSION, 0, 
              "commands outstanding (%i aborted %i requeued)\n",
              aborted, requeued);

    /* cleanup */
    if (g_cookie[isid]) {
        iscsi_free_atomic(g_cookie[isid]);
        g_cookie[isid] = NULL;
    }

    ISCSI_LOCK_ELSE(&g_lock[isid], return -1);
    if (param_list_destroy(sess->params)!=0) 
        TRACE_ERROR("param_list_destroy() failed\n");
    ISCSI_UNLOCK_ELSE(&g_lock[isid], return -1);

    if (g_target[isid].state & SESSION_STATE_CONNECTED) {
        if (iscsi_sock_close(sess->sock)!=0) 
            TRACE_ERROR("iscsi_sock_close() failed\n");
    } 
    QUEUE_INSERT_ELSE(&g_session_q, sess, return -1);
  
    /* the hash should always be empty */
    EQUAL_ELSE("hash count", hash_count(&g_cmd_hash[isid]), 0, return -1);

    /* clear state */
    g_target[isid].state = 0;
    *sess_ptr = NULL;

    return 0;
}

#define IS_DISCOVERY        1
#define IS_SECURITY         1
#define SESS_TYPE_DISCOVERY 1
#define SESS_TYPE_NORMAL    2
#define SESS_TYPE_NONE      3

/*
 * These should not be directly modified.  Set them in ips.conf
 */

#define PARAMS_OUT_ERROR {ISCSI_UNLOCK_ELSE(&g_lock[tid],return -1);return -1;}

static int params_out(INITIATOR_SESSION_T *sess, char *text, int *len, 
                      int sess_type, int security) {
    uint64_t tid = sess->isid;
    static int hostno = 0;
    char hostname[256];

    ISCSI_LOCK_ELSE(&g_lock[tid], return -1);
    if (g_target[tid].state & SESSION_STATE_DESTROYING) {
        TRACE_ERROR("session %"PRIu64" is being destroyed\n", tid);
        PARAMS_OUT_ERROR;
    }

    sprintf(hostname, "%s-%i", g_hostname, hostno++);


    /* initiator name and alias */
    PARAM_TEXT_ADD_ELSE(sess->params, "InitiatorName", hostname, text, 
                        len, 1, PARAMS_OUT_ERROR);
    PARAM_TEXT_ADD_ELSE(sess->params, "InitiatorAlias", "Intel", text, 
                        len, 1, PARAMS_OUT_ERROR);

    /* security mode */
    if (security == IS_SECURITY) {
        if (strlen(g_target[tid].InitiatorUser)) {
            PARAM_TEXT_ADD_ELSE(sess->params, "AuthMethod", "CHAP", text, 
                                len, 1, PARAMS_OUT_ERROR);
        } else {
            PARAM_TEXT_ADD_ELSE(sess->params, "AuthMethod", "None", text, 
                                len, 1, PARAMS_OUT_ERROR);
        }
    }

    /* session type */ 
    switch (sess_type){
    case SESS_TYPE_DISCOVERY:
        PARAM_TEXT_ADD_ELSE(sess->params, "SessionType", "Discovery", text, 
                            len, 1, PARAMS_OUT_ERROR);
        break;
    case SESS_TYPE_NORMAL:
        PARAM_TEXT_ADD_ELSE(sess->params, "SessionType", "Normal", text, 
                            len, 1, PARAMS_OUT_ERROR);
        PARAM_TEXT_ADD_ELSE(sess->params, "TargetName", 
                            g_target[tid].TargetName, text, len, 1, 
                            PARAMS_OUT_ERROR);
        break;
    default:
        if (strlen(g_target[tid].InitialR2T))
            PARAM_TEXT_ADD_ELSE(sess->params, "InitialR2T", 
                                g_target[tid].InitialR2T, text, len, 1, 
                                PARAMS_OUT_ERROR);
        if (strlen(g_target[tid].ImmediateData))
            PARAM_TEXT_ADD_ELSE(sess->params, "ImmediateData", 
                                g_target[tid].ImmediateData, text, len, 1, 
                                PARAMS_OUT_ERROR);
        if (strlen(g_target[tid].MaxRecvDataSegmentLength))
            PARAM_TEXT_ADD_ELSE(sess->params, "MaxRecvDataSegmentLength", 
                                g_target[tid].MaxRecvDataSegmentLength, text, 
                                len, 1, PARAMS_OUT_ERROR);
        if (strlen(g_target[tid].FirstBurstLength))
            PARAM_TEXT_ADD_ELSE(sess->params, "FirstBurstLength", 
                                g_target[tid].FirstBurstLength, text, len, 1, 
                                PARAMS_OUT_ERROR);
        if (strlen(g_target[tid].MaxBurstLength))
            PARAM_TEXT_ADD_ELSE(sess->params, "MaxBurstLength", 
                                g_target[tid].MaxBurstLength, text, len, 1, 
                                PARAMS_OUT_ERROR);
        break;
    }

    PARAM_TEXT_PARSE_ELSE(sess->params, text, *len, NULL, NULL, 1, 
                          &g_cookie[tid], g_target[tid].InitiatorUser, 
                          g_target[tid].InitiatorPass, 
                          g_target[tid].TargetUser, g_target[tid].TargetPass, 
                          PARAMS_OUT_ERROR);
    ISCSI_UNLOCK_ELSE(&g_lock[tid], return -1);
    return 0;
}

#define FFN_ERROR {if (cmd != NULL) iscsi_free_atomic(cmd); \
        if (text_cmd != NULL) iscsi_free_atomic(text_cmd); return -1;}

int full_feature_negotiation_phase_i(INITIATOR_SESSION_T *sess, char *text, 
                                     int text_len) {
    INITIATOR_CMD_T *cmd = NULL;
    ISCSI_TEXT_CMD_T *text_cmd = NULL;
    ISCSI_WQ_T wait;

    // Allocate command pointers

    if ((cmd=iscsi_malloc_atomic(sizeof(INITIATOR_CMD_T)))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        return -1;
    }
    memset(cmd, 0, sizeof(INITIATOR_CMD_T));
    if ((text_cmd=iscsi_malloc_atomic(sizeof(ISCSI_TEXT_CMD_T)))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        if (cmd != NULL) iscsi_free_atomic(cmd); // initiator command
        return -1;
    }
    memset(text_cmd, 0, sizeof(ISCSI_TEXT_CMD_T));

    /*
     * Note that <final>, <length> and <text> are updated
     * by text_response_i when we receive offers from
     * the target.
     */
    text_cmd->text = text;
    text_cmd->length = text_len;

    do {

        /* Build text command */
        text_cmd->final = 1;
        text_cmd->cont = 0;
        ISCSI_SET_TAG_ELSE(&text_cmd->tag, sess->isid, return -1);
        text_cmd->transfer_tag = 0xffffffff;

        /* Build wait for callback */
        ISCSI_WQ_INIT(&wait);

        /* Build initiator command */
        cmd->type = ISCSI_TEXT_CMD;
        cmd->ptr = text_cmd;
        cmd->callback = wait_callback_i;
        cmd->callback_arg = &wait;
        cmd->isid = sess->isid;

        /* give initiator command to tx worker */
        if (g_target[sess->isid].state & SESSION_STATE_DESTROYING) {
            TRACE_ERROR("session is being destroyed - exiting\n");
            FFN_ERROR;
        }
        QUEUE_INSERT_ELSE(&g_tx_queue[sess->isid], cmd, FFN_ERROR);
        ISCSI_WAKE_ELSE(NO_SIGNAL, &sess->tx_worker.wq, FFN_ERROR);
        ISCSI_WAIT(&wait, cmd->done);

        /* See if we're done.  text_response_i() overwrites text_cmd->final
         * with the final bit in the text response from the target. */
        if (!text_cmd->final) {
            TRACE(TRACE_DEBUG, 0, 
                  "negotiation needed (sending %i bytes response)\n", 
                  text_cmd->length);
        }
    } while (!text_cmd->final);

    /* Free command pointers */
    iscsi_free_atomic(cmd->ptr);  /* text command */
    iscsi_free_atomic(cmd);       /* initiator command */

    return 0;
}

#define DP_CLEANUP {if (text != NULL) iscsi_free_atomic(text);}
#define DP_ERROR {DP_CLEANUP;g_target[tid].state|=SESSION_STATE_ERROR;\
        return -1;}

int discovery_phase(uint64_t tid) {
    INITIATOR_SESSION_T *sess = g_target[tid].sess;
    char *ptr = NULL; 
    char *colon_ptr, *comma_ptr;
    char port[64];
    char *text = NULL;
    int text_len = 0;
    int i = 0;

    if ((text=iscsi_malloc_atomic(1024)) == NULL) {
        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        return -1;
    }

    /* Login to target */
    TRACE(TRACE_SESSION, 0, 
          "entering Discovery login phase with target %"PRIu64"\n", tid);
    text[0] = '\0';
    if (params_out(sess, text, &text_len, SESS_TYPE_DISCOVERY, 
                   IS_SECURITY)!=0) {
        TRACE_ERROR("params_out() failed\n");
        DP_ERROR;
    }
    if (login_phase_i(sess, text, text_len)!=0) {
        TRACE_ERROR("login_phase_i() failed for tid %"PRIu64"\n", tid);
        DP_ERROR;
    }
    TRACE(TRACE_SESSION, 0, 
          "now full feature for Discovery with target %"PRIu64"\n", tid);

    /* Full Feature Phase Negotiation (for SendTargets) */
    text_len = 0; text[0] = '\0';
    PARAM_TEXT_ADD_ELSE(sess->params, "SendTargets", "All",  text, 
                        &text_len, 1, DP_ERROR);
    PARAM_TEXT_PARSE_ELSE(sess->params, text, text_len, NULL, NULL, 
                          1, &g_cookie[tid], 
                          g_target[sess->isid].InitiatorUser, 
                          g_target[sess->isid].InitiatorPass, 
                          g_target[sess->isid].TargetUser, 
                          g_target[sess->isid].TargetPass, 
                          DP_ERROR);

    if (full_feature_negotiation_phase_i(sess, text, text_len)!=0) {
        TRACE_ERROR("full_feature_negotiation_phase_i() failed\n");
        DP_ERROR;
    }

    /* If <which> is set, then we search through the TargetNames returned
       by SendTargets until we find a match. If we don't find a match, 
       or g_which_target isn't set, then
       we just pick the first one. */

    if (param_val(sess->params, "TargetName")==NULL) {
        TRACE_ERROR("SendTargets failed\n");
        DP_ERROR;
    }
    while ((ptr=param_val_which(sess->params, "TargetName", i))!=NULL) {
        if (strstr(ptr, g_target[tid].which)!=NULL) {
            strcpy(g_target[tid].TargetName, ptr);
            break;
        }
        i++;    
    }
    if (ptr==NULL) {
        if (param_val(sess->params, "TargetName")) {
            strcpy(g_target[tid].TargetName, param_val(sess->params, 
                                                       "TargetName"));
        } 
    }

    if (!strlen(param_val(sess->params, "TargetAddress"))) {
        TRACE_ERROR("No targets discovered for tid %"PRIu64"\n", tid);
        DP_ERROR;
    }
    if (!g_target[sess->isid].fix_address&&param_val_which(sess->params, 
                                                           "TargetAddress", 
                                                           i)) {
        ptr = param_val_which(sess->params, "TargetAddress", i);
        colon_ptr = strchr(ptr, ':');
        if ((comma_ptr=strchr(ptr, ','))==NULL) {
            TRACE_ERROR("portal group tag is missing in \"%s\"\n", 
                        param_val(sess->params, "TargetAddress"));
            DP_ERROR;
        }
        if (colon_ptr) {
            strncpy(g_target[tid].ip, ptr, colon_ptr-ptr);
            strncpy(port, colon_ptr+1, comma_ptr-colon_ptr-1);
            port[comma_ptr-colon_ptr-1] = '\0';
            g_target[tid].port = iscsi_atoi(port);
        } else {
            strncpy(g_target[tid].ip, ptr, comma_ptr-ptr);
            g_target[tid].port = ISCSI_PORT;
        }
    } else {
        if (!g_target[sess->isid].fix_address) {
            TRACE_ERROR("SendTargets failed\n");
            DP_ERROR;
        }
    }

    /* Logout from target */
    if (logout_phase_i(sess)!=0) {
        TRACE_ERROR("logout_phase_i() failed\n");
        DP_ERROR;
    }

    DP_CLEANUP;
    return 0;
}

#define FFP_CLEANUP {if (text != NULL) iscsi_free_atomic(text);}
#define FFP_ERROR {FFP_CLEANUP; return -1;}

int full_feature_phase(INITIATOR_SESSION_T *sess) {
    char *text;
    int text_len;

    if ((text=iscsi_malloc_atomic(1024)) == NULL) {
        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        return -1;
    }

    // Set text parameters

    text[0] = '\0'; text_len = 0;
    if (params_out(sess, text, &text_len, SESS_TYPE_NORMAL, IS_SECURITY)!=0) {
        TRACE_ERROR("params_out() failed\n");
        FFP_ERROR;
    }

    // Send login command
  
    TRACE(TRACE_SESSION, 0, "entering login phase\n");
    if (login_phase_i(sess, text, text_len)!=0) {
        if (!sess->redirected) TRACE_ERROR("login_phase_i() failed\n");
        FFP_ERROR;
    } 
    TRACE(TRACE_SESSION, 0, "login phase successful\n");

    FFP_CLEANUP;
    return 0;
}

int initiator_init(char *config_file, int verbose, int family) {
    int i;

    g_initiator_state = 0;
    g_verbose = verbose;
    g_family = family;

#if 0
#ifdef __KERNEL__
    PRINT("iscsi: max targets is %i\n", CONFIG_INITIATOR_MAX_TARGETS);
    PRINT("iscsi: max queue depth is %i\n", CONFIG_INITIATOR_QUEUE_DEPTH);
#endif
#endif

#if 0
#ifdef CONFIG_EXPERIMENTAL
#ifdef __KERNEL__
    g_ramdisk_size = 1<<20;
    g_ramdisk_size *= 5;
    PRINT("allocating %"PRIu64" MB ramdisk\n", g_ramdisk_size>>20);
#else
    g_ramdisk_size = 1<<20;
    g_ramdisk_bypass=1;
#endif
    if ((g_ramdisk=iscsi_malloc(g_ramdisk_size))==NULL) {
        TRACE_ERROR("failed to allocate %"PRIu64" MB ramdisk\n", 
                    g_ramdisk_size>>20);
        g_ramdisk_failure = 1;
    }
#endif
#endif

    sprintf(g_hostname, "iqn.2003-08.com.intel:");
    /* so it's unique to this machine */
    sprintf(g_hostname+strlen(g_hostname), "%x:", OST_GETPID);
#ifdef __KERNEL__
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,12)
    sprintf(g_hostname+strlen(g_hostname), "%s", system_utsname.nodename);
#else
    sprintf(g_hostname+strlen(g_hostname), "%s", "?");
#endif
#else
    gethostname(g_hostname+strlen(g_hostname), 64);
#endif

    /* get target info */
    memset(g_target, 0, sizeof(g_target));
    if (get_target_config(config_file)!=0) {
        TRACE_ERROR("get_target_config() failed\n");
        return -1;
    }

#ifdef __KERNEL__
    if (!g_num_targets && !g_ramdisk_num_blocks) {
#else
    if (!g_num_targets) {
#endif
        TRACE_ERROR("NO TARGETS SPECIFIED\n");
        return -1;
    }

    /* initialization for each target */
    for (i=0; i<CONFIG_INITIATOR_MAX_TARGETS; i++) {
        g_nop_out_waiting[i] = 0;
        hash_init(&g_cmd_hash[i], CONFIG_INITIATOR_QUEUE_DEPTH);
        QUEUE_INIT_ELSE(&g_tx_queue[i], CONFIG_INITIATOR_QUEUE_DEPTH, 
                        return -1);
        ISCSI_LOCK_INIT_ELSE(&g_lock[i], return -1);
    }

#ifndef __KERNEL__
    QUEUE_INIT_ELSE(&g_alarm_q, CONFIG_INITIATOR_QUEUE_DEPTH+1, return -1);
#endif

    START_WORKER_ELSE("session_manager", (uint64_t) 0, g_session_manager, 
                      session_manager_proc, NULL, return -1);

    return g_num_targets;
}

int initiator_shutdown(void) {
    int i;

    NOT_EQUAL_ELSE("g_initiator_state", g_initiator_state, 
                   INITIATOR_STATE_SHUTDOWN, return -1);
    g_initiator_state = INITIATOR_STATE_SHUTDOWN;

    /* stop workers */
    STOP_WORKER_ELSE(g_session_manager, NO_SIGNAL, return -1);

    /* abort any outstanding commands */
    for (i=0; i<CONFIG_INITIATOR_MAX_TARGETS; i++) {
#if 0
        while ((cmd=iscsi_queue_remove(&g_tx_queue[i]))||
               (cmd=hash_remove_head(&g_cmd_hash[i]))) {
            TRACE_ERROR("aborting 0x%x\n", cmd->tag);
            EQUAL_ELSE("initiator_abort", initiator_abort(cmd), 0, return -1);
        }
#endif
        WARN_NOT_EQUAL("queue depth", iscsi_queue_depth(&g_tx_queue[i]), 0);
        iscsi_queue_destroy(&g_tx_queue[i]);
        WARN_NOT_EQUAL("hash count", hash_count(&g_cmd_hash[i]), 0);
        hash_destroy(&g_cmd_hash[i]);
        ISCSI_LOCK_DESTROY_ELSE(&g_lock[i], return -1);
    }
#ifndef __KERNEL__
    while (iscsi_queue_remove(&g_alarm_q)) {}
    iscsi_queue_destroy(&g_alarm_q);
#endif

#if 0
#ifdef CONFIG_EXPERIMENTAL
    iscsi_free(g_ramdisk);
#endif
#endif

    return 0;
}

static int wait_callback_i(void *ptr) {
    ISCSI_WQ_T *wait = (ISCSI_WQ_T *)(((INITIATOR_CMD_T*) ptr)->callback_arg);

    TRACE(TRACE_ISCSI, 1, "got callback for cmd %p\n", ptr);
    ISCSI_WAKE_ELSE(((INITIATOR_CMD_T*)ptr)->done=1, wait, return -1);

    return 0;
}

int initiator_abort(INITIATOR_CMD_T *cmd) {
    INITIATOR_CMD_T *start = NULL, *ptr;
    int flag = 0;
    cmd->status = -1;
    cmd->aborted = 1;
   
    while ((ptr=iscsi_queue_remove(&g_tx_queue[cmd->isid]))) {
        if (ptr->tag == cmd->tag) {
            break; 
        }
        QUEUE_INSERT_ELSE(&g_tx_queue[cmd->isid], ptr, return -1);
        if (start == ptr) {
            break;
        }
        if (!flag) {
            start = ptr;
            flag = 1;
        }
    }
#if 0
    if (iscsi_queue_remove_this(&g_tx_queue[cmd->isid], cmd)) {
        TRACE(TRACE_EH, 0, "removed 0x%x from tx queue\n", cmd->tag);
    }
#endif
    if (iscsi_done_i(cmd)!=0) {
        TRACE_ERROR("iscsi_done_i() failed\n");
        return -1;
    }
    return 0;
}

int initiator_session_reset(uint64_t tid) {
    g_target[tid].state |= SESSION_STATE_ERROR;
    return 0;
}


#ifndef __KERNEL__
static void alarm_signal_handler(int arg) {
    INITIATOR_CMD_T *cmd = NULL;

    QUEUE_REMOVE_ELSE(&g_alarm_q, cmd, return);
    if (cmd == NULL) {
        TRACE_ERROR("Nothing to abort!!\n"); 
        return;
    }
    if (initiator_abort(cmd)!=0) {
        TRACE_ERROR("initiator_abort() failed for 0x%x\n", cmd->tag);
    }
    return;
}
#endif

int initiator_command(int type, void *ptr, uint64_t tid, INITIATOR_CMD_T *cmd, 
                      int timeout) {
    ISCSI_WQ_T wait;

#ifndef __KERNEL__
    INITIATOR_CMD_T *cmd_ptr;
    int attempt = 0;

    if (timeout == -1) timeout = 30;
    if (timeout == 0) timeout = 3600;
 retry: if (timeout) {
        QUEUE_INSERT_ELSE(&g_alarm_q, cmd, return -1);
        signal (SIGALRM, alarm_signal_handler);
        alarm(timeout);
    }
#endif

    TRACE(TRACE_ISCSI, 0, "type %i tid %"PRIu64"\n", type, tid);    
    ISCSI_WQ_INIT(&wait);
    if (initiator_enqueue(type, ptr, tid, cmd, wait_callback_i, &wait)!=0) {
        TRACE_ERROR("initiator_enqueue() failed\n");
        return -1;
    } else {
        ISCSI_WAIT(&wait, cmd->done);
        TRACE(TRACE_ISCSI, 1, "type %i tid %"PRIu64" tag 0x%x completed\n", 
              type, tid, cmd->tag);
    }
    ISCSI_WQ_DESTROY(&wait);

#if 1
#ifndef __KERNEL__
    if (timeout) {
        if (cmd->aborted) {
            TRACE_ERROR("cmd 0x%x timed out (attempt %i)\n", cmd->tag, attempt);
#if 0
            if (attempt < 3) {
                attempt++;
                goto retry;
            }
#endif
        } else {
            QUEUE_REMOVE_THIS_ELSE(&g_alarm_q, cmd, cmd_ptr, exit(1));
            alarm(0);
        }
    }
#endif
#endif

    return cmd->status;
}

/*
 * initiator_enqueue() may be called from within interrupt context, so this
 * function can never block. All we do is enqueue the cmd to the enqueue
 * worker which will make sure it gets onto the right tx queue.
 */

int initiator_enqueue(int type, void *ptr, uint64_t tid, INITIATOR_CMD_T *cmd, 
                      int (*callback)(void *arg), void *callback_arg) {
    LESS_THAN_ELSE("tid", tid, g_num_targets, return -1);
    memset(cmd, 0, sizeof(INITIATOR_CMD_T));
    cmd->type = type;
    cmd->ptr = ptr;
    cmd->isid = tid;
    cmd->callback = callback;
    cmd->callback_arg = callback_arg;
    cmd->status = -1;

    ISCSI_SET_TAG_ELSE(&cmd->tag, tid, return -1);

    if (type==ISCSI_SCSI_CMD){
        TRACE(TRACE_SCSI, 1, "===BEGIN CDB===\n");
        PRINT_BUFF(TRACE_SCSI, 1, ((ISCSI_SCSI_CMD_T*) ptr)->cdb, 16, 16);
        TRACE(TRACE_SCSI, 1, "===END CDB===\n");
    }

#if 0
#ifdef CONFIG_EXPERIMENTAL
{
    ISCSI_SCSI_CMD_T *ptr = (ISCSI_SCSI_CMD_T *)(cmd->ptr);
    unsigned lba;
    unsigned short len;

    if (g_ramdisk_bypass || g_ramdisk_failure) goto normal;
    if (!g_pinning && (ptr->cdb[6]==77)) goto normal;
    if ((type==ISCSI_SCSI_CMD)&&
        ((ptr->cdb[0]==0x2a)||(ptr->cdb[0]==0x28))) {
        lba=htonl((*(unsigned*)(ptr->cdb+2)));
        len=htons((*(unsigned short*)(ptr->cdb+7)));

        /* Write to cache */
        if (ptr->cdb[0]==0x2a) {
            if (lba*512+ptr->trans_len > g_ramdisk_size) {
                TRACE_ERROR("exceeding g_ramdisk size on write\n");
                goto normal;
            }
            if (ptr->send_sg_len) {
                int i;
                unsigned offset=0;
                struct iovec *sg = (struct iovec *) ptr->send_data;

                for (i=0; i<ptr->send_sg_len; i++) {
                    memcpy(g_ramdisk+lba*512+offset, sg[i].iov_base, 
                           sg[i].iov_len);
                    offset += sg[i].iov_len;
                }
            } else {
                memcpy(g_ramdisk+lba*512, ptr->send_data, ptr->trans_len);
            }
            ptr->bytes_todev = ptr->trans_len;

        /* Read from cache */
        } else if (ptr->cdb[0]==0x28) {
            if (lba*512+ptr->trans_len > g_ramdisk_size) {
                TRACE_ERROR("exceeding g_ramdisk size on read\n");
                goto normal;
            }
            if (ptr->recv_sg_len) {
                int i;
                unsigned offset=0;
                struct iovec *sg = (struct iovec *) ptr->recv_data;

                for (i=0; i<ptr->recv_sg_len; i++) {
                    memcpy(sg[i].iov_base, g_ramdisk+lba*512+offset, 
                           sg[i].iov_len);
                    offset += sg[i].iov_len;
                }
            } else {
                memcpy(ptr->recv_data, g_ramdisk+lba*512, ptr->trans_len);
            }
            ptr->bytes_fromdev = ptr->trans_len;
        }

        /* Call done function */
	cmd->status = 0;
        HASH_INSERT_ELSE(&g_cmd_hash[tid], cmd, cmd->tag, return -1);
        iscsi_done_i(cmd);
        return 0;
    }
}
normal:
#endif
#endif

    QUEUE_INSERT_ELSE(&g_tx_queue[tid], cmd, return -1);
    if ((!(g_target[tid].state & SESSION_STATE_ERROR)) &&
        (g_target[tid].state & SESSION_STATE_FULL_FEATURE) &&
        (!(g_target[tid].state & SESSION_STATE_DESTROYING))) {
        ISCSI_WAKE_ELSE(NO_SIGNAL, &g_target[tid].sess->tx_worker.wq, 
                        return -1);
    } 
    return 0;
}

static int get_full_feature(uint64_t tid) {
    INITIATOR_SESSION_T *sess;

    /* initialize session */
    if (session_init_i(&g_target[tid].sess, tid)!=0) {  
        TRACE(TRACE_SESSION, 0, "session_init_i() failed (ptr %p)\n", 
              g_target[tid].sess);
        return -1;
    }
    sess = g_target[tid].sess;
    TRACE(TRACE_SESSION, 0, "session %"PRIu64" initialized ok (ptr %p)\n", 
          tid, sess);

    /* do discovery login if no TargetName specified */
    sess->discovery = strlen(g_target[tid].TargetName)?0:1;
    if (sess->discovery) {
        TRACE(TRACE_SESSION, 0, 
              "doing discovery phase with target %"PRIu64"\n", tid);
        if (discovery_phase(tid)==0) {
            /* kind of a hack.  After a successful discovery phase, we mark
               the session as being in error.  The next attempt to get it
               into full feature will use the new TargetName. */
            TRACE(TRACE_SESSION, 0, 
                  "discovery w/ tid %"PRIu64" complete (destroying session)\n", 
                  tid);
            g_target[tid].attempts = 0;
            g_target[tid].errors--;
            sess->discovery = 0;
        } 
        return -1;
    } 

    /* get into full feature phase */
    TRACE(TRACE_SESSION, 0, "entering full feature with target %"PRIu64"\n", 
          tid);
    if (full_feature_phase(sess)!=0) {
        if (!sess->redirected) TRACE_ERROR("full_feature_phase() failed\n");
        return -1;
    }    
    TRACE(TRACE_SESSION, 0, "now full feature with target %"PRIu64"\n", tid);
   
    /* set the mostly accessed parameters for easy retrieval */
    set_session_parameters(sess->params, &sess->sess_params);

    /* session is now ready to accept scsi commands */    
    g_target[sess->isid].state |= SESSION_STATE_FULL_FEATURE;

    return 0;
}

/*
 * The session worker is responsible for getting a session into full
 * feature phase. It reports to the session manager.
 */

#define SESSION_WORKER_FAILURE {g_target[tid].state |= SESSION_STATE_ERROR; \
        EXIT(me, -1);}

static int session_worker_proc(void *arg) {
    ISCSI_WORKER_T *me = (ISCSI_WORKER_T *) arg;
    uint64_t tid = me->id;
    extern int iscsi_report_target(int);
    int rc = 0;

    /* let session manager know we've started */
    START_ELSE(me, 1, SESSION_WORKER_FAILURE);
 
    /* rate limit our login attempts */
    if (g_target[tid].attempts>10) {
        SLEEP_UNLESS_THEN(5, WORKER_STOPPING(me), EXIT(me,0))
            } else if (g_target[tid].attempts>2) {
        SLEEP_UNLESS_THEN(1, WORKER_STOPPING(me), EXIT(me,0))
            }

    /* get session into full feature phase */
    if ((rc=get_full_feature(tid))!=0) {
        g_target[tid].state |= SESSION_STATE_ERROR;
    } 

    /* report the target (we do these in order so that the drives
       are assigned in order in the kernel) */
#ifdef __KERNEL__
#if (!CONFIG_DRIVER_PROBE)
    if ((g_target[tid].state&SESSION_STATE_FULL_FEATURE)&&
        !(g_target[tid].state&SESSION_STATE_ERROR)) {
        while (g_reported != tid) {
            ISCSI_USLEEP(10000);
            if WORKER_STOPPING(me) {
                    EXIT(me,0);
                }
        }
        g_target[tid].state |= SESSION_STATE_REPORTING;
        iscsi_report_target(tid);
        g_target[tid].state &= ~SESSION_STATE_REPORTING;
        g_reported++;
    } 
#endif
#endif

    /* wait for shutdown signal from manager */
    ISCSI_WAIT(&me->wq, WORKER_STOPPING(me));
    EXIT(me, 0);
}

/*
 * Session watchdog
 */

#ifdef CONFIG_INITIATOR_WATCHDOG
static ISCSI_WORKER_T g_session_watchdog[CONFIG_INITIATOR_MAX_TARGETS];

typedef struct watchdog_cmd_t {
    INITIATOR_CMD_T initiator_cmd;
    ISCSI_NOP_OUT_T nop_cmd;
    ISCSI_SCSI_CMD_T scsi_cmd;
    ISCSI_WQ_T wq;
    int done;
} WATCHDOG_CMD_T;

int watchdog_callback(void *arg) {
    WATCHDOG_CMD_T *watchdog_cmd = (WATCHDOG_CMD_T *) arg;

    ISCSI_WAKE_ELSE(((watchdog_cmd->done=1)||1), &watchdog_cmd->wq, return -1);
    return 0;
}

static int session_watchdog_proc(void *arg) {
    ISCSI_WORKER_T *me = (ISCSI_WORKER_T *) arg;
    WATCHDOG_CMD_T watchdog_cmd;

    START_ELSE(me, 1, EXIT(me, -1));
    memset(&watchdog_cmd, 0, sizeof(WATCHDOG_CMD_T));
    ISCSI_WQ_INIT(&watchdog_cmd.wq);
    SLEEP_UNLESS_THEN(5, WORKER_STOPPING(me), EXIT(me,0))
        while (1) {
            watchdog_cmd.done = 0;
            EQUAL_ELSE("initiator_enqueue", 
                       initiator_enqueue(ISCSI_NOP_OUT, 
                                         &watchdog_cmd.nop_cmd, me->id,
                                         &watchdog_cmd.initiator_cmd, 
                                         watchdog_callback, &watchdog_cmd), 
                       0, EXIT(me,-1)); 
            g_target[me->id].nop_out_count++;
            SLEEP_UNLESS_THEN(5, (WORKER_STOPPING(me)||watchdog_cmd.done), 
                              NO_ACTION);
            if (!watchdog_cmd.done) {
                PRINT("iscsi watchdog: tid %i did not respond to NOP_OUT "
                      "0x%x\n", me->id, watchdog_cmd.initiator_cmd.tag);
                initiator_abort(&watchdog_cmd.initiator_cmd);
                g_target[me->id].state |= SESSION_STATE_ERROR;
                SLEEP_UNLESS_THEN(5, WORKER_STOPPING(me), EXIT(me,0));
            } else if (WORKER_STOPPING(me)) {
                EXIT(me,0);
            }
            SLEEP_UNLESS_THEN(5, WORKER_STOPPING(me), EXIT(me,0));
        }
}
#endif

/*
 * The session manager monitors all sessions and creates a
 * session worker to keep each on alive.
 */

static int session_manager_proc(void *arg) {
    INITIATOR_SESSION_T *sess = NULL;
    ISCSI_WORKER_T *me = (ISCSI_WORKER_T *) arg;
    int i;

    /* initialize sessions */
    QUEUE_INIT_ELSE(&g_session_q, g_num_targets, EXIT(me, -1));
    for (i=0; i<g_num_targets; i++) {
        if ((sess=iscsi_malloc_atomic(sizeof(INITIATOR_SESSION_T)))==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n"); 
            EXIT(me, -1);
        }
        QUEUE_INSERT_ELSE(&g_session_q, sess, EXIT(me, -1));
        ISCSI_SPIN_LOCK_INIT_ELSE(&g_tag_lock[i], EXIT(me, -1));
    }

#ifdef CONFIG_INITIATOR_WATCHDOG
    /* let the dogs out */
    for (i=0; i<g_num_targets; i++) {
        START_WORKER_ELSE("watchdog", i, g_session_watchdog[i], 
                          session_watchdog_proc, NULL, EXIT(me, -1));
    }
#endif

    /* signal that we've started */
    START_ELSE(me, 1, EXIT(me, -1));

    /* loop over connections, checking for errors */
    while (!WORKER_STOPPING(me)) {
        for (i=0; i<g_num_targets; i++) {
            if (g_target[i].state & SESSION_STATE_ERROR) {
                STOP_WORKER_ELSE(g_session_worker[i], 
                                 (session_destroy_i(&g_target[i].sess)==0), 
                                 EXIT(me, -1));
                g_target[i].state = 0;
                g_target[i].errors++;
            } else if (!(g_target[i].state & SESSION_STATE_STARTED)) {
                g_target[i].state |= SESSION_STATE_STARTED;
                START_WORKER_ELSE("sess_worker", (uint64_t) i, 
                                  g_session_worker[i], session_worker_proc, 
                                  NULL, return -1);
                g_target[i].attempts++;
            } else if (g_target[i].state & SESSION_STATE_FULL_FEATURE) {
                g_target[i].attempts = 0;
            }
        }
        ISCSI_USLEEP(1000);
    }

#ifdef CONFIG_INITIATOR_WATCHDOG
    /* call in the dogs */
    for (i=0; i<g_num_targets; i++) {
        STOP_WORKER_ELSE(g_session_watchdog[i], NO_SIGNAL, EXIT(me, -1));
    }
#endif

    /* clean up sessions (need to add a logout phase) */
    for (i=0; i<g_num_targets; i++) {
        if (g_target[i].state & SESSION_STATE_STARTED) {
            while (g_target[i].state & SESSION_STATE_REPORTING) {
                ISCSI_USLEEP(100000);
            }
            STOP_WORKER_ELSE(g_session_worker[i], 
                             (session_destroy_i(&g_target[i].sess)==0), 
                             EXIT(me, -1));
            g_target[i].state = 0;
        }
        ISCSI_SPIN_LOCK_DESTROY_ELSE(&g_tag_lock[i], EXIT(me, -1));
    }
    while ((sess=iscsi_queue_remove(&g_session_q))!=NULL) 
        iscsi_free_atomic(sess);    
    iscsi_queue_destroy(&g_session_q);

    EXIT(me, 0);
}

/***********
 * Private *
 ***********/

static int iscsi_done_i(INITIATOR_CMD_T *cmd) {
    int rc = 0;

    NOT_NULL_ELSE("cmd", cmd, return -1);
    TRACE(TRACE_ISCSI, 0, 
          "cmd 0x%x (cmd ptr %p) done (tid %"PRIu64", type %i, status %i, "
          "aborted %i)\n", cmd->tag, cmd, cmd->isid, cmd->type, cmd->status, 
          cmd->aborted);

    /* remove from hash of outanding commands */
    if ((hash_remove(&g_cmd_hash[cmd->isid], cmd->tag)==NULL) && 
        !cmd->aborted) {
        TRACE_ERROR("cmd 0x%x not in hash\n", cmd->tag);
        return -1;
    }

    if (!cmd->aborted) {
        /* check for correct transfer */
        if (cmd->type == ISCSI_SCSI_CMD) {
            ISCSI_SCSI_CMD_T *ptr = (ISCSI_SCSI_CMD_T *) cmd->ptr;
  
            if ((ptr->fromdev)&&(ptr->todev)) {
                EQUAL_ELSE("bytes_todev", ptr->bytes_todev, ptr->trans_len, 
                           cmd->status = -1);
                EQUAL_ELSE("bytes_fromdev", ptr->bytes_fromdev, 
                           ptr->bidi_trans_len, cmd->status = -1);
            } else if (ptr->fromdev) {
                WARN_NOT_EQUAL("bytes_fromdev", ptr->bytes_fromdev, 
                               ptr->trans_len);
            } else if (ptr->todev) {
                EQUAL_ELSE("bytes_todev", ptr->bytes_todev, ptr->trans_len, 
                           cmd->status = -1);
            } else {
                EQUAL_ELSE("bytes_todev", ptr->bytes_todev, 0, 
                           cmd->status = -1);
                EQUAL_ELSE("bytes_fromdev", ptr->bytes_fromdev, 0, 
                           cmd->status = -1);
            }
        }
    }

    /* issue callback */
    NOT_EQUAL_ELSE("cmd->callback", (unsigned long) cmd->callback, 0, 
                   return -1);
    NOT_EQUAL_ELSE("cmd->callback_arg", (unsigned long) cmd->callback_arg, 
                   0, return -1);
    if (cmd->callback(cmd)!=0) {
        TRACE_ERROR("callback failed for 0x%x\n", cmd->tag);
        rc = -1;
    }

    return rc;
}

/*
 * Tx Worker (one per connection)
 */

static int tx_connect_and_start_rx(ISCSI_WORKER_T *me, 
                                   INITIATOR_SESSION_T *sess) {
    uint64_t tid;
    int rc;
#ifndef __KERNEL__
    char cmd[1024];
#endif

    tid = sess->isid;

    if ((rc=iscsi_sock_connect(sess->sock, g_target[tid].ip, 
                               g_target[tid].port, g_family))!=0) {
        if (g_target[tid].attempts<=10) {
            PRINT("iscsi: failed connection with target %"PRIu64" [%s:%i] "
                  "(attempt %i)", tid, g_target[tid].ip, g_target[tid].port, 
                  g_target[tid].attempts);
            if (g_target[tid].attempts==10) PRINT(" -- suppressing errors");
            PRINT("\n");
        }
#ifndef __KERNEL__
        sprintf(cmd, "touch /tmp/%"PRIu64".target_error", tid); 
        if (system(cmd)!=0) {
            fprintf(stderr, "error creating /tmp/%"PRIu64".target_error\n", 
                    tid);
        }
#endif
        return -1;
    }    
    g_target[tid].state |= SESSION_STATE_CONNECTED;
    START_WORKER_ELSE("iscsi rx", tid, sess->rx_worker, 
                      rx_worker_proc_i, sess, return -1);

    return 0;
}

/*
 * There is one tx thread per session.  This is the only thread
 * that writes to the socket.
 */
static int tx_worker_proc_i(void *arg) {
    ISCSI_WORKER_T *me = (ISCSI_WORKER_T *) arg;
    INITIATOR_CMD_T *cmd;
    uint64_t tid = me->id;
    INITIATOR_SESSION_T *sess = g_target[tid].sess;
    int rc = 0;  

    START_ELSE(me, (tx_connect_and_start_rx(me,sess)==0), EXIT(me, -1));
    while (1) {
    next: TRACE(TRACE_SESSION, 1, "tx_worker[%i]: waiting for work\n", me->id);
        ISCSI_WAIT(&me->wq, WORKER_STOPPING(me)||
                   iscsi_queue_depth(&g_tx_queue[tid])||
                   g_nop_out_waiting[tid]);
        if (WORKER_STOPPING(me)) {
            goto success;
        } else if (iscsi_queue_depth(&g_tx_queue[tid])) {

            /* get command, do a few checks, do debugging output  */
            QUEUE_REMOVE_ELSE(&g_tx_queue[tid], cmd, goto failure);

            /* only process certain commands until we go full feature */
            if (!(g_target[tid].state & SESSION_STATE_FULL_FEATURE) && 
                !((cmd->type==ISCSI_LOGIN_CMD)||
                  (cmd->type==ISCSI_TEXT_CMD)||
                  (cmd->type==ISCSI_LOGOUT_CMD))) {
                QUEUE_INSERT_ELSE(&g_tx_queue[tid], cmd, goto failure);
                ISCSI_USLEEP(1000);
                goto next;
            } 

            /* Add this command to the list of outstanding commands.  We must
             * do this before actually executing the command, else there's a
             * race condition with the rx thread: if the rx thread gets the
             * response before we add the command to the hash, it won't knowg
             * what command the response is associated with. */

            if (!hash_get(&g_cmd_hash[tid], cmd->tag)) {
                HASH_INSERT_ELSE(&g_cmd_hash[tid], cmd, cmd->tag, return -1);
            } 
            EQUAL_ELSE("tid", cmd->isid, tid, goto failure);
            if (cmd->aborted) goto next;
            if (cmd->type == ISCSI_SCSI_CMD) {
                TRACE(TRACE_ISCSI, 1, "tag 0x%x, type %i, op 0x%x\n", 
                      cmd->tag, cmd->type, 
                      ((ISCSI_SCSI_CMD_T *)(cmd->ptr))->cdb[0]);
            } else {
                TRACE(TRACE_ISCSI, 1, "tag 0x%x, type %i\n", 
                      cmd->tag, cmd->type);
            }

            /* execute command */
            cmd->tx_done = 0; 
            switch(cmd->type) {
            case (ISCSI_LOGIN_CMD):
                ((ISCSI_LOGIN_CMD_T *)(cmd->ptr))->tag = cmd->tag;
                TRACE(TRACE_ISCSI, 0, 
                      "tx_worker[%"PRIu64"]: ISCSI_LOGIN_CMD 0x%x\n", 
                      tid, cmd->tag);
                EQUAL_ELSE("login_command_i", login_command_i(cmd), 
                           0, goto failure); 
                break;
            case (ISCSI_TEXT_CMD):
                ((ISCSI_TEXT_CMD_T *)(cmd->ptr))->tag = cmd->tag;
                TRACE(TRACE_ISCSI, 0, 
                      "tx_worker[%"PRIu64"]: ISCSI_TEXT_CMD 0x%x\n", 
                      tid, cmd->tag);
                EQUAL_ELSE("text_command_i", text_command_i(cmd), 0, 
                           goto failure);
                break;
            case (ISCSI_SCSI_CMD):
                ((ISCSI_SCSI_CMD_T *)(cmd->ptr))->tag = cmd->tag;
                if (cmd->r2t_request) {
                    EQUAL_ELSE("scsi_r2t_tx_i", scsi_r2t_tx_i(sess, cmd), 
                               0, goto failure);
                    cmd->r2t_request--;
                } else {
                    TRACE(TRACE_ISCSI, 0, 
                          "tx_worker[%"PRIu64"]: ISCSI_SCSI_CMD 0x%x "
                          "(op 0x%x, lun %"PRIu64", fromdev %i, todev %i)\n", 
                          tid, cmd->tag, 
                          ((ISCSI_SCSI_CMD_T *)(cmd->ptr))->cdb[0], 
                          ((ISCSI_SCSI_CMD_T *)(cmd->ptr))->lun,
                          ((ISCSI_SCSI_CMD_T*)(cmd->ptr))->fromdev, 
                          ((ISCSI_SCSI_CMD_T*)(cmd->ptr))->todev);
                    EQUAL_ELSE("bytes_todev", 
                               ((ISCSI_SCSI_CMD_T *)(cmd->ptr))->bytes_todev, 
                               0, goto failure);
                    EQUAL_ELSE("scsi_command_i", scsi_command_i(cmd), 
                               0, goto failure);
                }                               
                break;
            case (ISCSI_NOP_OUT):
                TRACE(TRACE_ISCSI, 0, 
                      "tx_worker[%"PRIu64"]: ISCSI_NOP_OUT 0x%x\n", 
                      tid, cmd->tag);
                if (((ISCSI_NOP_OUT_T *)(cmd->ptr))->tag != 0xffffffff) {
                    /* response required */
                    ((ISCSI_NOP_OUT_T *)(cmd->ptr))->tag = cmd->tag; 
                    ((ISCSI_NOP_OUT_T *)(cmd->ptr))->transfer_tag = 0xffffffff;
                }
                EQUAL_ELSE("nop_out_i", nop_out_i(cmd), 0, goto failure);
                break;
            case (ISCSI_LOGOUT_CMD):
                ((ISCSI_LOGOUT_CMD_T *)(cmd->ptr))->tag = cmd->tag;
                TRACE(TRACE_ISCSI, 0, 
                      "tx_worker[%"PRIu64"]: ISCSI_LOGOUT_CMD 0x%x\n", 
                      tid, cmd->tag);
                EQUAL_ELSE("logout_command_i", logout_command_i(cmd), 0, 
                           goto failure);
                break;
            default:
                TRACE_ERROR("tx_worker[%"PRIu64"]: unknown iSCSI op 0x%x\n", 
                            tid, cmd->type);
                cmd->status = -1;
                break;
            }

            /* The Rx thread will receive a response for the command and
               execute the callback.  We need to make sure the callback 
               function is not executed before the Tx thread has completed
               sending the command.  This is what tx_done is used for.  The
               last step is to set tx_done and signal the rx thread.  
               NOP_OUT (without ping) will have no response for the rx
               thread to process - so we execute the callback here directly. */
            if ((cmd->type == ISCSI_NOP_OUT)&&
                (((ISCSI_NOP_OUT_T*)(cmd->ptr))->tag == 0xffffffff)) {          
                EQUAL_ELSE("iscsi_done_i", iscsi_done_i(cmd), 0, goto failure);
            } else {
                ISCSI_WAKE_ELSE(cmd->tx_done = 1, 
                                &sess->rx_worker.wq, goto failure);
            }
        } else if (g_nop_out_waiting[tid]) {
            INITIATOR_CMD_T cmd;

            memset(&cmd, 0, sizeof(INITIATOR_CMD_T));
            cmd.ptr = &g_nop_out[tid];
            cmd.isid = tid;
            EQUAL_ELSE("nop_out_i", nop_out_i(&cmd), 0, goto failure);
            g_nop_out_waiting[tid] = 0;
        } else {
            //TRACE_ERROR("tx worker woken up for no reason\n");
            //goto failure;
        }
    }
 failure: rc = -1; g_target[tid].state |= SESSION_STATE_ERROR;
 success: STOP_WORKER_ELSE(sess->rx_worker, 
                           (iscsi_sock_shutdown(sess->sock, 
                                                ISCSI_SOCK_SHUTDOWN_RECV)==0),
                           EXIT(me, -1));
    EXIT(me, rc);
}

/*
 * There is one Rx worker per connection.
 */

#define RX_FAILURE {g_target[tid].state |= SESSION_STATE_ERROR; EXIT(me, -1);}

static int rx_worker_proc_i(void *arg) {
    ISCSI_WORKER_T *me = (ISCSI_WORKER_T *) arg;
    INITIATOR_SESSION_T *sess = (INITIATOR_SESSION_T *) me->ptr;
    unsigned char header[ISCSI_HEADER_LEN];
    INITIATOR_CMD_T *cmd = NULL;
    unsigned tag;
    uint64_t tid = me->id;

    START_ELSE(me, 1, RX_FAILURE);
    while (!WORKER_STOPPING(me)) {
        if (iscsi_sock_msg(sess->sock, 0, ISCSI_HEADER_LEN, header, 0)!=
            ISCSI_HEADER_LEN) {
            TRACE(TRACE_SESSION, 0, 
                  "rx_worker[%"PRIu64"]: iscsi_sock_msg() failed\n", tid);
            RX_FAILURE;
        }
        if (WORKER_STOPPING(me)) {
            TRACE(TRACE_SESSION, 0, "rx worker got shutdown signal!\n");
            goto rx_exit;
        }
        if ((ISCSI_OPCODE(header)!=ISCSI_REJECT)&&
            (ISCSI_OPCODE(header)!=ISCSI_ASYNC)) {
            tag = NTOHL(*((unsigned *)(header+16))); 
            if ((cmd = hash_get(&g_cmd_hash[tid], tag))==NULL) {
                if (ISCSI_OPCODE(header)!=ISCSI_NOP_IN) {
                    TRACE_ERROR("no cmd for op 0x%x tag 0x%x (EXITING) \n", 
                                ISCSI_OPCODE(header), tag);
                    RX_FAILURE;
                }
            } else {                
                /* wait until the tx thread is finished with it */
                ISCSI_WAIT(&sess->rx_worker.wq, cmd->tx_done);
            }
        }

        /* execute response */
        switch(ISCSI_OPCODE(header)) {
        case(ISCSI_SCSI_RSP):
            TRACE(TRACE_ISCSI, 0, 
                  "rx_worker[%"PRIu64"]: ISCSI_SCSI_RSP 0x%x\n", tid, 
                  ISCSI_ITT(header));
            EQUAL_ELSE("scsi_response_i", 
                       scsi_response_i(sess, cmd, header), 0, RX_FAILURE);
            break;
        case(ISCSI_READ_DATA):
            TRACE(TRACE_ISCSI, 0, 
                  "rx_worker[%"PRIu64"]: ISCSI_READ_DATA 0x%x\n", tid,  
                  ISCSI_ITT(header));
            EQUAL_ELSE("scsi_read_data_i", 
                       scsi_read_data_i(sess, cmd, header), 0, RX_FAILURE);
            break;
        case ISCSI_R2T:
            TRACE(TRACE_ISCSI, 0, "rx_worker[%"PRIu64"]: ISCSI_R2T 0x%x\n", 
                  tid, ISCSI_ITT(header));
            EQUAL_ELSE("scsi_r2t_rx_i", scsi_r2t_rx_i(sess, cmd, header), 
                       0, RX_FAILURE);
            break;
        case ISCSI_NOP_IN:
            TRACE(TRACE_ISCSI, 0, 
                  "rx_worker[%"PRIu64"]: ISCSI_NOP_IN (ITT 0x%x, TTT 0x%x)\n",
                  tid, ISCSI_ITT(header), ISCSI_TTT(header));
            EQUAL_ELSE("nop_in_i", nop_in_i(sess, cmd, header), 0, RX_FAILURE);
            break;
        case ISCSI_LOGIN_RSP:
            TRACE(TRACE_ISCSI, 0, 
                  "rx_worker[%"PRIu64"]: ISCSI_LOGIN_RSP 0x%x\n", 
                  tid, ISCSI_ITT(header));
            EQUAL_ELSE("login_response_i", 
                       login_response_i(sess, cmd, header), 0, RX_FAILURE);
            break;
        case ISCSI_TEXT_RSP:
            TRACE(TRACE_ISCSI, 0, 
                  "rx_worker[%"PRIu64"]: ISCSI_TEXT_RSP\n", tid);
            EQUAL_ELSE("text_response_i", text_response_i(sess, cmd, header), 
                       0, RX_FAILURE);
            break;
        case ISCSI_LOGOUT_RSP:
            TRACE(TRACE_ISCSI, 0, "rx_worker[%"PRIu64"]: ISCSI_LOGOUT_RSP\n", 
                  tid);
            EQUAL_ELSE("logout_response_i", 
                       logout_response_i(sess, cmd, header), 0, RX_FAILURE);
            break;
        case ISCSI_REJECT:
            TRACE(TRACE_ISCSI, 0, "rx_worker[%"PRIu64"]: ISCSI_REJECT\n", tid);
            EQUAL_ELSE("reject_i", reject_i(sess, header), 0, RX_FAILURE);
            break;
        case ISCSI_ASYNC:
            TRACE(TRACE_ISCSI, 0, "rx_worker[%"PRIu64"]: ISCSI_ASYNC\n", tid);
            EQUAL_ELSE("async_msg", async_msg_i(sess, header), 0, RX_FAILURE);
            break;
        default:
            TRACE_ERROR("rx_worker[%"PRIu64"]: unexpected iSCSI op 0x%x\n", 
                        tid, ISCSI_OPCODE(header));
            RX_FAILURE;
        }
    }
 rx_exit: EXIT(me, 0);
}

static int text_command_i(INITIATOR_CMD_T *cmd) {
    ISCSI_TEXT_CMD_T* text_cmd = (ISCSI_TEXT_CMD_T*) cmd->ptr;
    INITIATOR_SESSION_T *sess = g_target[cmd->isid].sess;
    unsigned char header[ISCSI_HEADER_LEN];

    // Send text command PDU

    text_cmd->ExpStatSN = sess->ExpStatSN;
    text_cmd->CmdSN = sess->CmdSN++;
    TRACE(TRACE_DEBUG, 0, "sending text command\n");
    if (iscsi_text_cmd_encap(header, text_cmd)!=0) {
        TRACE_ERROR("(iscsi_text_cmd_encap() failed\n");
        return -1;    
    }
    if (iscsi_sock_send_header_and_data(sess->sock, header, ISCSI_HEADER_LEN, 
                                        text_cmd->text, text_cmd->length, 0) !=
        ISCSI_HEADER_LEN+text_cmd->length) {
        TRACE_ERROR("iscsi_sock_send_header_and_data() failed.\n");
        return -1;
    }

    TRACE(TRACE_DEBUG, 0, "text command sent ok\n"); 

    return 0;
}

static int login_command_i(INITIATOR_CMD_T *cmd) {
    ISCSI_LOGIN_CMD_T* login_cmd = (ISCSI_LOGIN_CMD_T*) cmd->ptr;
    INITIATOR_SESSION_T *sess = g_target[cmd->isid].sess;
    unsigned char header[ISCSI_HEADER_LEN];

    // Send login command PDU
    login_cmd->ExpStatSN = sess->ExpStatSN;
    TRACE(TRACE_DEBUG, 0, "sending login command\n");
    if (iscsi_login_cmd_encap(header, login_cmd)!=0) {
        TRACE_ERROR("(iscsi_login_cmd_encap() failed\n");
        return -1;
    }

    if (iscsi_sock_send_header_and_data(sess->sock, header, ISCSI_HEADER_LEN, 
                                        login_cmd->text, login_cmd->length, 0) 
        != ISCSI_HEADER_LEN+login_cmd->length) {
        TRACE_ERROR("iscsi_sock_send_header_and_data() failed.\n");
        return -1;
    }

    TRACE(TRACE_DEBUG, 0, "login command sent ok\n"); 

    return 0;
}

#define LO_CLEANUP {                                            \
        if (cmd != NULL) iscsi_free_atomic(cmd);                \
        if (logout_cmd != NULL) iscsi_free_atomic(logout_cmd);  \
        ISCSI_WQ_DESTROY(&wait);                                \
    }
#define LO_ERROR {LO_CLEANUP; return -1;}

static int logout_phase_i(INITIATOR_SESSION_T *sess) {
    INITIATOR_CMD_T *cmd = NULL;
    ISCSI_LOGOUT_CMD_T *logout_cmd = NULL;
    ISCSI_WQ_T wait;

    ISCSI_WQ_INIT(&wait);

    /* build logout command */
    if ((cmd=iscsi_malloc_atomic(sizeof(INITIATOR_CMD_T)))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        return -1;
    }
    memset(cmd, 0, sizeof(INITIATOR_CMD_T));
    if ((logout_cmd=iscsi_malloc_atomic(sizeof(ISCSI_LOGOUT_CMD_T)))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        LO_ERROR;
    }
    memset(logout_cmd, 0, sizeof(ISCSI_LOGOUT_CMD_T));
    logout_cmd->cid = sess->cid;
    logout_cmd->reason = ISCSI_LOGOUT_CLOSE_SESSION;
    ISCSI_SET_TAG_ELSE(&logout_cmd->tag, sess->isid, return -1);
    logout_cmd->ExpStatSN = sess->ExpStatSN;
    logout_cmd->CmdSN = sess->CmdSN++;

    /* build initiator command */
    cmd->type = ISCSI_LOGOUT_CMD;
    cmd->ptr = logout_cmd;
    cmd->callback = wait_callback_i;
    cmd->callback_arg = &wait;
    cmd->isid = sess->isid;

    /* give to tx worker and wait for completion */
    if (g_target[sess->isid].state & SESSION_STATE_DESTROYING) {
        TRACE_ERROR("session is being destroyed - exiting\n");
        LO_CLEANUP;
        return -1;
    }
    ISCSI_WAKE_ELSE((iscsi_queue_insert(&g_tx_queue[cmd->isid], cmd)==0), 
                    &sess->tx_worker.wq, LO_CLEANUP; return -1);
    ISCSI_WAIT(&wait, cmd->done);

    LO_CLEANUP;
    return 0;
}

#define LI_CLEANUP {                                            \
        if (cmd != NULL) iscsi_free_atomic(cmd);                \
        if (login_cmd != NULL) iscsi_free_atomic(login_cmd);    \
        ISCSI_WQ_DESTROY(&wait);                                \
    }
#define LI_ERROR {LI_CLEANUP; return -1;}

static int login_phase_i(INITIATOR_SESSION_T *sess, char *text, int text_len) {
    INITIATOR_CMD_T *cmd = NULL;
    ISCSI_LOGIN_CMD_T *login_cmd = NULL;
    ISCSI_WQ_T wait;

    ISCSI_WQ_INIT(&wait);

    /* build login command */
    if ((cmd=iscsi_malloc_atomic(sizeof(INITIATOR_CMD_T)))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        return -1;
    }
    memset(cmd, 0, sizeof(INITIATOR_CMD_T));
    if ((login_cmd=iscsi_malloc_atomic(sizeof(ISCSI_LOGIN_CMD_T)))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        if (cmd != NULL) iscsi_free_atomic(cmd);
        return -1;
    }
    memset(login_cmd, 0, sizeof(ISCSI_LOGIN_CMD_T));
    login_cmd->text = text;     
    login_cmd->length = text_len;  
    login_cmd->transit = 1;
    login_cmd->csg = ISCSI_LOGIN_STAGE_SECURITY;
    login_cmd->nsg = ISCSI_LOGIN_STAGE_NEGOTIATE;
    ISCSI_SET_TAG_ELSE(&cmd->tag, sess->isid, return -1);
    login_cmd->CmdSN = sess->CmdSN = 1;

    /* enter login phase */

    do {

        /* Build login command.  Note that the <length> and <text> fields
           may get updated by login_response_i.  Such is the case when we
           receive offers from the target.  The new <length> and <text>
           fields will represent the response that we need to send to the
           target on the next login. */
        login_cmd->cont = 0;
        login_cmd->version_min = ISCSI_VERSION;
        login_cmd->version_max = ISCSI_VERSION;
        login_cmd->cid = sess->cid = sess->isid;
        login_cmd->isid = sess->isid;
        login_cmd->tsih = 0;

        /* Build initiator command */
        cmd->type = ISCSI_LOGIN_CMD;
        cmd->ptr = login_cmd;
        cmd->callback = wait_callback_i;
        cmd->callback_arg = &wait;
        cmd->isid = sess->isid;
        cmd->done = 0;

        /* Give initiator command directly to tx worker */
        if (g_target[sess->isid].state & SESSION_STATE_DESTROYING) {
            TRACE_ERROR("session is being destroyed - exiting\n");
            LI_ERROR;
        }
        ISCSI_WAKE_ELSE((iscsi_queue_insert(&g_tx_queue[cmd->isid], cmd)==0),
                        &sess->tx_worker.wq, LI_ERROR);
        TRACE(TRACE_SESSION, 0, "cmd 0x%x enqueued for login (waiting)\n",
              cmd->tag);
        ISCSI_WAIT(&wait, cmd->done);
        TRACE(TRACE_SESSION, 0, 
              "cmd 0x%x enqueued for login (done waiting)\n", cmd->tag);

        /* see why cmd->status is not being set on ABORT */
        if ((cmd->status)||(g_target[sess->isid].state & 
                            SESSION_STATE_DESTROYING)) {
            if (cmd->status) {
                if (!sess->redirected) TRACE_ERROR("INITIATOR_CMD_T failed\n");
            } else {
                TRACE_ERROR("***session %"PRIu64" is being destroyed***\n", 
                            cmd->isid);
            }
            if (cmd != NULL) iscsi_free_atomic(cmd);                
            if (login_cmd != NULL) iscsi_free_atomic(login_cmd);
            ISCSI_WQ_DESTROY(&wait); 
            return -1;
            LI_ERROR;
        }
        if (!(g_target[sess->isid].state & SESSION_STATE_LOGGED_IN)) {
            TRACE(TRACE_ISCSI, 1, 
                  "negotiation needed (sending %i bytes response)\n", 
                  login_cmd->length);
        }
    } while (!(g_target[sess->isid].state & SESSION_STATE_LOGGED_IN));
    LI_CLEANUP;
    return 0;
}

#define TI_CLEANUP {if (text_in != NULL) iscsi_free_atomic(text_in); \
        if (text_out != NULL) iscsi_free_atomic(text_out);}
#define TI_ERROR {cmd->status=-1; goto callback;}

static int text_response_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd, 
                           unsigned char *header) {
    ISCSI_TEXT_CMD_T *text_cmd;
    ISCSI_TEXT_RSP_T text_rsp;
    ISCSI_PARAMETER_T *l = sess->params;
    char *text_in = NULL;
    char *text_out = NULL;
    int len_in = 0;
    int len_out = 0;
    int ret = 0;

    if (cmd) {
        text_cmd = (ISCSI_TEXT_CMD_T*) cmd->ptr;
    } else {
        TRACE_ERROR("no INITIATOR_CMD_T specified for ISCSI_TEXT_CMD_T??\n");
        return -1;
    }
  
    // Check arguments & update numbering

    if (iscsi_text_rsp_decap(header, &text_rsp)!=0) {
        TRACE_ERROR("text_response_decap() failed\n");
        TI_ERROR;
    }

    RETURN_NOT_EQUAL("Tag", text_rsp.tag, text_cmd->tag, TI_ERROR, -1);
    RETURN_NOT_EQUAL("Transfer Tag", text_rsp.transfer_tag, 0xffffffff, 
                     TI_ERROR, -1);
    RETURN_NOT_EQUAL("StatSN", text_rsp.StatSN, sess->ExpStatSN, TI_ERROR, -1);
    RETURN_NOT_EQUAL("ExpCmdSN", text_rsp.ExpCmdSN, sess->CmdSN, TI_ERROR, -1);
    sess->ExpStatSN = text_rsp.StatSN + 1;

    // Parse input text parameters and generate any response

    if ((len_in=text_rsp.length)) {
        TRACE(TRACE_DEBUG, 0, "allocating %i bytes input parameters\n", len_in);
        if ((text_in=iscsi_malloc_atomic(len_in+1))==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
            TI_ERROR;
        }
        if ((text_out=iscsi_malloc_atomic(ISCSI_PARAM_MAX_TEXT_LEN))==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
            if (text_in != NULL) iscsi_free_atomic(text_in);
            TI_ERROR;
        }
        if (iscsi_sock_msg(sess->sock, 0, len_in, text_in, 0)!=len_in) {
            TRACE_ERROR("iscsi_sock_msg() failed\n");
            TI_ERROR;
        }
        text_in[len_in] = '\0';
        TRACE(TRACE_DEBUG, 0, "read %i bytes input parameters ok\n", len_in);

        // Reset the value lists for TargetName and TargetAddress

        if (param_val_reset(sess->params, "TargetName")!=0) {
            TRACE_ERROR("parm_val_reset() failed\n");
            TI_ERROR;
        }
        if (param_val_reset(sess->params, "TargetAddress")!=0) {
            TRACE_ERROR("parm_val_reset() failed\n");
            TI_ERROR;
        }

        // Parse the incoming answer

        PARAM_TEXT_PARSE_ELSE(l, text_in, len_in, text_out, &len_out, 0, 
                              &g_cookie[sess->isid], 
                              g_target[sess->isid].InitiatorUser, 
                              g_target[sess->isid].InitiatorPass, 
                              g_target[sess->isid].TargetUser, 
                              g_target[sess->isid].TargetPass, TI_ERROR);

        if (len_out) {

            RETURN_NOT_EQUAL("text_rsp.final", text_rsp.final, 0, TI_ERROR, -1);

            // Copy response text into text_cmd->text and update the
            // length text_cmd->length.  This will be sent out on the
            // next text command.

            PARAM_TEXT_PARSE_ELSE(l, text_out, len_out, NULL, NULL, 1, 
                                  &g_cookie[sess->isid], 
                                  g_target[sess->isid].InitiatorUser, 
                                  g_target[sess->isid].InitiatorPass, 
                                  g_target[sess->isid].TargetUser, 
                                  g_target[sess->isid].TargetPass, TI_ERROR);

            TRACE(TRACE_DEBUG, 0, 
                  "need to send %i bytes response back to target\n", len_out); 
            text_cmd->length = len_out;
            memcpy(text_cmd->text, text_out, len_out);
        } else {
            text_cmd->length = 0;
        }
    } 
    text_cmd->final = text_rsp.final;

    // Issue callback

    TRACE(TRACE_DEBUG, 0, "ISCSI_TEXT_CMD_T done\n");
 callback:
    if (cmd->status == -1) ret = -1;
    if (iscsi_done_i(cmd)!=0) {
        TRACE_ERROR("iscsi_done_i() failed\n");
        ret = -1;
    }
    TI_CLEANUP;
    return ret;
}

#define LIR_CLEANUP {if (text_in != NULL) iscsi_free_atomic(text_in); \
        if (text_out != NULL) iscsi_free_atomic(text_out);}
#define LIR_ERROR {cmd->status=-1; goto callback;}

static int login_response_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd,
                            unsigned char *header) {
    ISCSI_LOGIN_CMD_T *login_cmd;
    ISCSI_LOGIN_RSP_T login_rsp;
    char *text_in = NULL;
    char *text_out = NULL;
    int len_in = 0;
    int len_out = 0;

    if ((text_out=iscsi_malloc_atomic(ISCSI_PARAM_MAX_TEXT_LEN))==NULL) {
        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        cmd->status = -1;
        goto callback;
    }
    if (cmd) {
        login_cmd = (ISCSI_LOGIN_CMD_T*) cmd->ptr;
    } else {
        TRACE_ERROR("no INITIATOR_CMD_T specified for ISCSI_LOGIN_CMD_T??\n");
        LIR_ERROR;
    }
  
    /* Read login response */
    if (iscsi_login_rsp_decap(header, &login_rsp)!=0) {
        TRACE_ERROR("login_response_decap() failed\n");
        LIR_ERROR;
    }
    RETURN_GREATER("Length (should this be hardcoded?)", login_rsp.length, 
                   8192, LIR_CLEANUP, -1);

    /* Read & parse text response */
    if ((len_in=login_rsp.length)) {
        if ((text_in=iscsi_malloc_atomic(len_in+1))==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
            LIR_ERROR;
        }
        if (iscsi_sock_msg(sess->sock, 0, len_in, text_in, 0)!=len_in) {
            TRACE_ERROR("iscsi_sock_msg() failed\n");
            LIR_ERROR;
        }
        PARAM_TEXT_PARSE_ELSE(sess->params, text_in, len_in, text_out, 
                              &len_out, 0, &g_cookie[sess->isid],
                              g_target[sess->isid].InitiatorUser, 
                              g_target[sess->isid].InitiatorPass, 
                              g_target[sess->isid].TargetUser, 
                              g_target[sess->isid].TargetPass, LIR_ERROR);
        if (login_rsp.transit) WARN_NOT_EQUAL("len_out", len_out, 0);
    } 

    /* Check for correct authentication and authorization */
    if (login_rsp.status_class==2 && login_rsp.status_detail==1) {
        TRACE_ERROR("Initiator authentication failure\n");
        LIR_ERROR;
    } else if (login_rsp.status_class==2 && login_rsp.status_detail==2) {
        TRACE_ERROR("Initiator authorization failure\n");
        LIR_ERROR;
    }

    /* Check for a redirection */
    if (login_rsp.status_class!=0 && login_rsp.status_detail==1) {
        char address[256];

        strcpy(address, param_val(sess->params, "TargetAddress"));
        if (g_target[sess->isid].fix_address) {
            /*
            fprintf(stderr, "Ignoring redirection to %s:%i\n", 
                    g_target[sess->isid].ip, g_target[sess->isid].port);
            */
        } else {
            if (strchr(address, ':')) {
                g_target[sess->isid].port = iscsi_atoi(strchr(address, ':')+1);
                address[strchr(address, ':')-address] = '\0';
            }
            /*
            fprintf(stderr, "Redirection to %s:%i\n", g_target[sess->isid].ip, 
                    g_target[sess->isid].port);
            */            
            strcpy(g_target[sess->isid].ip, address);
        }

        /* same hack as we do in discovery (just kill the session and
           the next attempt will use the updated TargetName */

        sess->redirected = 1;
          
        LIR_ERROR;
    }

    /* Check args */
    if (login_rsp.status_class!=0) {
        TRACE_ERROR("Bad Status-Class: got %i, expected %i (detail 0x%x)\n", 
                    login_rsp.status_class, 0, login_rsp.status_detail);
        LIR_ERROR;
    }
    if (login_rsp.tag!=login_cmd->tag) {
        TRACE_ERROR("Bad Tag: got %u, expected %u\n", login_rsp.tag, 
                    login_cmd->tag);
        LIR_ERROR;
    }
    sess->ExpStatSN = login_rsp.StatSN + 1; 

    if (login_rsp.transit) {
        if (login_cmd->transit != 1) {
            TRACE_ERROR("incoming transit bit not set, csg = %d, nsg = %d\n",
                        login_cmd->csg, login_cmd->nsg);
            LIR_ERROR;
        }
        switch (login_rsp.nsg) {
        case ISCSI_LOGIN_STAGE_NEGOTIATE:
            login_cmd->csg = login_cmd->nsg;
            login_cmd->nsg = ISCSI_LOGIN_STAGE_FULL_FEATURE;
            if (params_out(sess, text_out, &len_out, SESS_TYPE_NONE, 
                           !IS_SECURITY) !=0) {
                TRACE_ERROR("params_out() failed\n");
                LIR_ERROR;
            }
            login_cmd->length = len_out;
            memcpy(login_cmd->text, text_out, len_out);
            break;
   
        case ISCSI_LOGIN_STAGE_FULL_FEATURE:
   
            /* Check post conditions */
            RETURN_EQUAL("TSIH", 0, login_rsp.tsih, LIR_ERROR, -1);
            RETURN_NOT_EQUAL("ISID", (unsigned int) login_rsp.isid, 
                             (unsigned int) login_cmd->isid, LIR_ERROR, -1);
            /*
            RETURN_NOT_EQUAL("ExpCmdSN", login_rsp.ExpCmdSN, login_cmd->CmdSN, 
                             LIR_ERROR, -1);            
            WARN_NOT_EQUAL("ExpCmdSN", login_rsp.ExpCmdSN, login_cmd->CmdSN);
            */
            RETURN_GREATER("MaxCmdSN", login_rsp.ExpCmdSN, 
                           login_rsp.MaxCmdSN, LIR_ERROR, -1);

            /* Set remaining session parameters */
            sess->CmdSN = login_rsp.ExpCmdSN;
            sess->MaxCmdSN = login_rsp.MaxCmdSN;
            sess->tsih = login_rsp.tsih;
            sess->isid = login_rsp.isid;

            if (!param_equiv(sess->params, "SessionType", "Normal")) {
                g_target[sess->isid].state |= SESSION_STATE_LOGGED_IN;
            } else if (!param_equiv(sess->params, "SessionType", "Discovery")) {
                g_target[sess->isid].state |= SESSION_STATE_LOGGED_IN;
            } else {
                TRACE_ERROR("Unknown SessionType \"%s\"\n", 
                            param_val(sess->params, "SessionType"));
                LIR_ERROR;
            }

            if (g_verbose) {
                char TargetName[256];
                char InitiatorName[256];

                strncpy(InitiatorName, 
                        param_val(sess->params, "InitiatorName"), 50);
                InitiatorName[50] = '\0';
                strncpy(TargetName, g_target[sess->isid].TargetName, 50);
                TargetName[50] = '\0';

                PRINT("****************************************************"
                      "****************************\n");
                PRINT("*                                LOGIN SUCCESSFUL   "
                      "                           *\n");
                PRINT("*                                                   "
                      "                           *\n");
                PRINT("* %25s:%50s *\n", "InitiatorName", InitiatorName);
                if (strlen(param_val(sess->params, "InitiatorName"))>50) {
                    PRINT("* %25s:%50s *\n", "InitiatorName (cont)", 
                          param_val(sess->params, "InitiatorName")+50);
                }
                PRINT("* %25s:%50s *\n", "TargetName", TargetName);
                if (strlen(param_val(sess->params, "TargetName"))>50) {
                    PRINT("* %25s:%50s *\n", "TargetName (cont)", 
                          param_val(sess->params, "TargetName")+50);
                }
                PRINT("* %25s:%50s *\n", "Type", 
                      param_val(sess->params, "SessionType"));
                if (!param_equiv(sess->params, "AuthMethod", "CHAP")) {
                    PRINT("* %25s:%50s *\n", "TargetUser",  
                          (strlen(g_target[sess->isid].TargetUser)>0)?
                          g_target[sess->isid].TargetUser:"N/A");
                }
                PRINT("* %25s:%50"PRIu64" *\n", "ISID", sess->isid);
                PRINT("* %25s:%50u *\n", "TSIH", sess->tsih);
                PRINT("* %25s:%50s *\n", "InitialR2T",  
                      param_val(sess->params, "InitialR2T")); 
                PRINT("* %25s:%50s *\n", "ImmediateData",  
                      param_val(sess->params, "ImmediateData"));
                PRINT("* %25s:%50s *\n", "MaxRecvDataSegmentLength",  
                      param_val(sess->params, "MaxRecvDataSegmentLength"));
                PRINT("* %25s:%50s *\n", "FirstBurstLength",  
                      param_val(sess->params, "FirstBurstLength"));
                PRINT("* %25s:%50s *\n", "MaxBurstLength",  
                      param_val(sess->params, "MaxBurstLength"));
                PRINT("****************************************************"
                      "****************************\n");
            }
           
            break;
        default:
            LIR_ERROR;
        }
    }
    else {
        TRACE(TRACE_DEBUG, 0, "received partial login response\n");

        // Copy response text into login_cmd->text and update the
        // length login_cmd->length.  This will be sent out on the
        // next login command.

        if (len_out) {
            PARAM_TEXT_PARSE_ELSE(sess->params, text_out, len_out, NULL, 
                                  NULL, 1, &g_cookie[sess->isid], 
                                  g_target[sess->isid].InitiatorUser, 
                                  g_target[sess->isid].InitiatorPass, 
                                  g_target[sess->isid].TargetUser, 
                                  g_target[sess->isid].TargetPass, LIR_ERROR);
            TRACE(TRACE_DEBUG, 0, 
                  "need to send %i bytes response back to target\n", len_out); 

            login_cmd->length = len_out;
            memcpy(login_cmd->text, text_out, len_out);
            if (strncmp(text_out, "CHAP_N=", strlen("CHAP_N=")) == 0) {
                login_cmd->nsg = ISCSI_LOGIN_STAGE_NEGOTIATE;
                login_cmd->transit = 1;
            }
        } else {
            login_cmd->length = 0;
        }
    } 

    // Callback

 callback:
    TRACE(TRACE_ISCSI, 0, 
          "ISCSI_LOGIN_CMD_T done (cmd status %i, iscsi status %i)\n",
          cmd->status, login_rsp.status_class);
    if (iscsi_done_i(cmd)!=0) {
        TRACE_ERROR("iscsi_done_i() failed\n");
        LIR_CLEANUP;
        return -1;
    }

    LIR_CLEANUP;
    return 0;
}

static int logout_command_i(INITIATOR_CMD_T *cmd) {
    ISCSI_LOGOUT_CMD_T *logout_cmd = (ISCSI_LOGOUT_CMD_T*) cmd->ptr;
    INITIATOR_SESSION_T *sess = g_target[cmd->isid].sess;
    unsigned char header[ISCSI_HEADER_LEN];

    // Send logout command PDU

    TRACE(TRACE_DEBUG, 0, "sending logout command\n");
    if (iscsi_logout_cmd_encap(header, logout_cmd)!=0) {
        TRACE_ERROR("(iscsi_logout_cmd_encap() failed\n");
        return -1;
    }
    if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, header, 0)!=
        ISCSI_HEADER_LEN) {
        TRACE_ERROR("iscsi_sock_msg() failed.\n");
        return -1;
    }
    TRACE(TRACE_DEBUG, 0, "logout command sent ok\n");

    return 0;
}

#define LOR_ERROR {cmd->status=-1; goto callback;}

static int logout_response_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd, 
                             unsigned char *header) {
    ISCSI_LOGOUT_CMD_T *logout_cmd;
    ISCSI_LOGOUT_RSP_T logout_rsp;
  
    if (cmd) {
        if (cmd->ptr) {
            logout_cmd = (ISCSI_LOGOUT_CMD_T *) cmd->ptr;
        } else {
            TRACE_ERROR("no ISCSI_LOGOUT_CMD_T for INITIATOR_CMD_T??\n");
            LOR_ERROR;
        }
    } else {
        TRACE_ERROR("no INITIATOR_CMD_T for ISCSI_LOGOUT_CMD_T??\n");
        return -1;
    }
    if (iscsi_logout_rsp_decap(header, &logout_rsp)!=0) {
        TRACE_ERROR("iscsi_logout_rsp_decap() failed\n");
        LOR_ERROR;
    }
    RETURN_NOT_EQUAL("Response", logout_rsp.response, 
                     ISCSI_LOGOUT_STATUS_SUCCESS, LOR_ERROR, -1);
    RETURN_NOT_EQUAL("Tag", logout_rsp.tag, logout_cmd->tag, LOR_ERROR, -1);

    // Check and update numbering
    RETURN_NOT_EQUAL("StatSN", logout_rsp.StatSN, sess->ExpStatSN, 
                     LOR_ERROR, -1);
    sess->ExpStatSN++; 
    RETURN_NOT_EQUAL("ExpCmdSN", logout_rsp.ExpCmdSN, sess->CmdSN, 
                     LOR_ERROR, -1);
    sess->MaxCmdSN = logout_rsp.MaxCmdSN;

    // Callback
    cmd->status = 0;
    TRACE(TRACE_DEBUG, 0, 
          "LOGOUT_CMD_T done (cmd status %i, iscsi status %i)\n", 
          cmd->status, logout_rsp.response);
 callback:
    if (iscsi_done_i(cmd)!=0) {
        TRACE_ERROR("iscsi_done_i() failed\n");
        return -1;
    }

    if (g_verbose) {
        PRINT("***********************************************************"
              "*********************\n");
        PRINT("*                               LOGOUT SUCCESSFUL          "
              "                    *\n");
        PRINT("*                                                          "
              "                    *\n");
        PRINT("* %25s:%50s *\n", "Type", param_val(sess->params, \
                                                   "SessionType"));
        PRINT("* %25s:%50"PRIu64" *\n", "ISID", sess->isid);
        PRINT("* %25s:%50u *\n", "TSIH", sess->tsih);
        PRINT("***********************************************************"
              "*********************\n");
    }

    return 0;
}

static int nop_out_i(INITIATOR_CMD_T *cmd) {
    unsigned char header[ISCSI_HEADER_LEN];
    ISCSI_NOP_OUT_T *nop_out = cmd->ptr;
    INITIATOR_SESSION_T *sess = g_target[cmd->isid].sess;
    int rc, length = nop_out->length;

    // Encapsulate and send NOP
    nop_out->CmdSN = sess->CmdSN;
    nop_out->ExpStatSN = sess->ExpStatSN;

    if (nop_out->tag  == 0xffffffff) {
        WARN_NOT_EQUAL("I bit", nop_out->immediate, 1);   
    } else {
        sess->CmdSN++;
    }
    if (iscsi_nop_out_encap(header, nop_out)!=0) {
        TRACE_ERROR("iscsi_nop_out_encap() failed\n");
        return -1;
    } 

    /* We need to make a copy of nop_out->length and save in the variable
       length.  Otherwise, we may get a seg fault - as if this is a NOP_OUT
       without ping, the Tx thread will issue the callback function
       immediately after we return - thereby unallocating the NOP_OUT and
       initiator command structures.*/
 
    if ((rc=iscsi_sock_send_header_and_data(sess->sock, header, 
                                            ISCSI_HEADER_LEN, nop_out->data, 
                                            length, 0))!= ISCSI_HEADER_LEN+
        length) {
        TRACE_ERROR("iscsi_sock_msg() failed: got %i expected %i\n", 
                    rc, ISCSI_HEADER_LEN+length);
        return -1;
    }

    cmd->status = 0;
    return 0;
}

static int scsi_command_i(INITIATOR_CMD_T *cmd) {
    ISCSI_SCSI_CMD_T *scsi_cmd = (ISCSI_SCSI_CMD_T *) cmd->ptr;
    unsigned char header[ISCSI_HEADER_LEN];
    uint64_t tid = cmd->isid;
    INITIATOR_SESSION_T *sess = g_target[tid].sess;
    ISCSI_WRITE_DATA_T data;
    struct iovec sg_singleton;
    struct iovec *sg, *sg_copy, *sg_copy_orig, *sg_which;
    int sg_len, sg_len_copy, sg_len_which;
    int fragment_flag = 0;

    if (scsi_cmd->bytes_todev) {
        TRACE_ERROR("before is %u??\n", scsi_cmd->bytes_todev);
        return -1;
    }
  
    sg = sg_copy = sg_copy_orig = sg_which = NULL;
    sg_len = sg_len_copy = sg_len_which = 0;
    scsi_cmd->status = 0;

    if (sess->sess_params.max_burst_length == 0) {
        TRACE_ERROR("MaxBurstLength is 0 (%u) tid %"PRIu64"\n", 
                    sess->sess_params.max_burst_length, tid);
        return -1;
    }
  
    TRACE(TRACE_ISCSI, 0, "tx_worker[%"PRIu64"]: tag 0x%x scsi op 0x%x lun %"
          PRIu64" trans_len %i length %i send_sg_len %i recv_sg_len %i\n", 
          tid, scsi_cmd->tag, scsi_cmd->cdb[0], scsi_cmd->lun, 
          scsi_cmd->trans_len, 
          scsi_cmd->length, scsi_cmd->send_sg_len, scsi_cmd->recv_sg_len);

    RETURN_GREATER("target id", (unsigned int) tid, g_num_targets-1, 
                   NO_CLEANUP, -1);

    // Set and check scsi_cmd

#if 0 
    if( scsi_cmd->trans_len > sess->sess_params.max_burst_length ) {
        TRACE_WARN("scsi_cmd->trans_len (%u) > MaxBurstLength (%u)\n",
                    scsi_cmd->trans_len, sess->sess_params.max_burst_length);
        //return -1;
    }
#endif
    if( scsi_cmd->length > scsi_cmd->trans_len ) {
        TRACE_ERROR("scsi_cmd->length (%u) > scsi_cmd->trans_len (%u)\n",
                    scsi_cmd->length, scsi_cmd->trans_len);
        return -1;
    }

    scsi_cmd->ExpStatSN = sess->ExpStatSN;
    scsi_cmd->CmdSN = sess->CmdSN;
    scsi_cmd->bytes_todev = scsi_cmd->bytes_fromdev = 0;

    // Always use iovec for data

    if (scsi_cmd->todev) {
        if (scsi_cmd->send_sg_len) {                  // Data already an iovec
            sg = (struct iovec *) scsi_cmd->send_data;
            sg_len = scsi_cmd->send_sg_len;
        } else {                                      // Make iovec for data
            sg_singleton.iov_base = scsi_cmd->send_data;
            sg_singleton.iov_len = scsi_cmd->trans_len;
            sg = &sg_singleton;
            sg_len = 1;
        } 
    }

    // Send command PDU

    if (scsi_cmd->todev && sess->sess_params.immediate_data) {
        if (sess->sess_params.max_data_seg_length) {
            scsi_cmd->length = MIN(sess->sess_params.max_data_seg_length, 
                                   scsi_cmd->trans_len);
        } else {
            scsi_cmd->length = scsi_cmd->trans_len;
        }
        if (scsi_cmd->length==scsi_cmd->trans_len) {
            scsi_cmd->final = 1;   /* all data fit as immediate data */
        } else if (sess->sess_params.initial_r2t) {
            scsi_cmd->final = 1;  /* we'll wait for an R2T */
        } else {
            scsi_cmd->final = 0;  /* we'll be sending data PDUs */
        }
    } else {
        scsi_cmd->length = 0;
        if (sess->sess_params.initial_r2t) {
            scsi_cmd->final = 1;  /* we'll wait for an R2T */
        } else {
            scsi_cmd->final = 0;  /* we'll be sending data PDUs */
        }
    }
    if (iscsi_scsi_cmd_encap(header, scsi_cmd)!=0) {
        TRACE_ERROR("iscsi_scsi_cmd_encap() failed\n");
        goto error;
    }

    // If we're sending any immediate data, we need to make a new iovec that
    // contains only the immediata data (a subset of the original iovec).

    TRACE(TRACE_ISCSI, 0, "sending command PDU with %u bytes immediate data\n",
          scsi_cmd->length);
    if (scsi_cmd->length && sess->sess_params.immediate_data) {
#ifdef NO_SG_MALLOC
        sg_copy = cmd->sg_copy_1;
#else
        if ((sg_copy=iscsi_malloc_atomic(sg_len*sizeof(struct iovec)))==NULL) { /* MALLOC 1 */
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
            goto error;
        }
#endif
        fragment_flag++;
        sg_copy_orig = sg_copy;
        memcpy(sg_copy, sg, sizeof(struct iovec)*sg_len); sg_len_copy = sg_len; 
        if (modify_iov(&sg_copy, &sg_len_copy, 0, scsi_cmd->length)!=0) {
            TRACE_ERROR("modify_iov() failed\n");
            goto error;
        }   
        if (scsi_cmd->ahs) {
            if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, header, 0)
                !=ISCSI_HEADER_LEN) {
                TRACE_ERROR("iscsi_sock_msg() failed\n");
                goto error;
            }
            if (iscsi_sock_msg(sess->sock, 1, scsi_cmd->ahs_len, 
                               scsi_cmd->ahs, 0)!=scsi_cmd->ahs_len) {
                TRACE_ERROR("iscsi_sock_msg() failed\n");
                goto error;
            }
            if (iscsi_sock_msg(sess->sock, 1, scsi_cmd->length, sg_copy, 
                               sg_len_copy)!=scsi_cmd->length) {
                TRACE_ERROR("iscsi_sock_msg() failed\n");
                goto error;
            }
        } else {
            if (iscsi_sock_send_header_and_data(sess->sock, header, 
                                                ISCSI_HEADER_LEN, 
                                                sg_copy, scsi_cmd->length, 
                                                sg_len_copy)
                !=ISCSI_HEADER_LEN+scsi_cmd->length) {
                TRACE_ERROR("iscsi_sock_send_header_and_data() failed\n");
                goto error;
            }
        }
        TRACE(TRACE_ISCSI, 0, 
              "incrementing bytes_todev by %u (scsi_cmd %p, tag 0x%x)\n", 
              scsi_cmd->length, scsi_cmd, scsi_cmd->tag);
        scsi_cmd->bytes_todev += scsi_cmd->length;
    } else {
        if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, header, 0)!=
            ISCSI_HEADER_LEN) {
            TRACE_ERROR("iscsi_sock_msg() failed\n");
            goto error;
        }
        if (scsi_cmd->ahs_len) {
            if (iscsi_sock_msg(sess->sock, 1, scsi_cmd->ahs_len, 
                               scsi_cmd->ahs, 0)!=scsi_cmd->ahs_len) {
                TRACE_ERROR("iscsi_sock_msg() failed\n");
                goto error;
            }
        }
    }
    TRACE(TRACE_ISCSI, 0, 
          "command PDU sent with %u bytes immediate data (%u bytes AHS)\n", 
          scsi_cmd->length, scsi_cmd->ahs_len);

    /* Send data PDUS if 1) we're not in R2T mode and 2) we haven't sent
       everything as immediate data and 3) we have not reached the first
       burst when sending immediate data */

    if (scsi_cmd->todev
        && (!sess->sess_params.initial_r2t)
        && (scsi_cmd->bytes_todev!=scsi_cmd->trans_len)
        && ((!sess->sess_params.first_burst_length)
            ||(scsi_cmd->bytes_todev < 
               sess->sess_params.first_burst_length))) {

        unsigned DataSN = 0;
 
        TRACE(TRACE_ISCSI, 0, "preparing to send %i bytes write data\n", 
              scsi_cmd->trans_len-scsi_cmd->bytes_todev); 

        do {
            memset(&data, 0, sizeof(ISCSI_WRITE_DATA_T));

            /* Take into account that MaxRecvPDULength and FirstBurstLength
               could both be "0" (no limit) */
            if (sess->sess_params.max_data_seg_length)  {
                if (sess->sess_params.first_burst_length)  {
                    data.length = MIN_3(
                                        sess->sess_params.first_burst_length - 
                                        scsi_cmd->bytes_todev,
                                        sess->sess_params.max_data_seg_length,
                                        scsi_cmd->trans_len-scsi_cmd->
                                        bytes_todev );
                } else {
                    data.length = MIN(
                                      sess->sess_params.max_data_seg_length,
                                      scsi_cmd->trans_len-scsi_cmd->
                                      bytes_todev);
                }
            } else {
                if (sess->sess_params.first_burst_length)  {
                    data.length = MIN(
                                      sess->sess_params.first_burst_length - 
                                      scsi_cmd->bytes_todev,
                                      scsi_cmd->trans_len-scsi_cmd->
                                      bytes_todev);
                } else {
                    data.length = scsi_cmd->trans_len-scsi_cmd->bytes_todev;
                }
            }

            RETURN_EQUAL("data.length", data.length, 0, {if (fragment_flag) 
                        iscsi_free_atomic(sg_copy);}, -1);

            if ((scsi_cmd->bytes_todev+data.length==scsi_cmd->trans_len)||
                (scsi_cmd->bytes_todev+data.length>=sess->
                 sess_params.first_burst_length)) {
                data.final = 1;
            }
            data.tag = scsi_cmd->tag;
            data.transfer_tag = 0xffffffff;
            data.ExpStatSN = sess->ExpStatSN;
            data.DataSN = DataSN++;
            data.offset = scsi_cmd->bytes_todev;

            if (iscsi_write_data_encap(header, &data)!=0) {
                TRACE_ERROR("iscsi_write_data_encap() failed\n");
                goto error;
            }

            if (data.length!=scsi_cmd->trans_len) {

                // Make copy of iovec and modify with offset and length

                if (!fragment_flag) {
#ifdef NO_SG_MALLOC
                    sg_copy = cmd->sg_copy_2;
#else
                    if ((sg_copy=
                         iscsi_malloc_atomic(sg_len*sizeof(struct iovec))) /* MALLOC 2 */
                        ==NULL) {
                        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
                        goto error;
                    }
#endif
                    sg_copy_orig = sg_copy;
                    fragment_flag++;
                }
                sg_copy = sg_copy_orig;
		if (sg_len > CONFIG_INITIATOR_MAX_SG) {
		    PANIC("sg_len %i is greater than CONFIG_INITIATOR_MAX_SG %i\n",
			  sg_len, CONFIG_INITIATOR_MAX_SG);
		}
                memcpy(sg_copy, sg, sizeof(struct iovec)*sg_len); 
                sg_len_copy = sg_len; 
                if (modify_iov(&sg_copy, &sg_len_copy, scsi_cmd->bytes_todev, 
                               data.length)!=0) {
                    TRACE_ERROR("modify_iov() failed\n");
                    goto error;
                }   

                sg_which = sg_copy;
                sg_len_which = sg_len_copy;

            } else { 

                // Data was not fragmented; use the original iovec.

                sg_which = sg;
                sg_len_which = sg_len;
            }

            TRACE(TRACE_DEBUG, 0, 
                  "sending write data PDU (offset %u, len %u, sg_len %u)\n", 
                  data.offset, data.length, sg_len_which);

            if (iscsi_sock_send_header_and_data(sess->sock, header, 
                                                ISCSI_HEADER_LEN, 
                                                sg_which, data.length, 
                                                sg_len_which)
                !=ISCSI_HEADER_LEN+data.length) {
                TRACE_ERROR("iscsi_sock_send_header_and_data() failed\n");
                goto error;
            }
                
            TRACE(TRACE_DEBUG, 0, "sent write data PDU (offset %u, len %u)\n", 
                  data.offset, data.length);
            scsi_cmd->bytes_todev += data.length;
        } while ((scsi_cmd->bytes_todev<scsi_cmd->trans_len) 
                 && ((scsi_cmd->bytes_todev < 
                      sess->sess_params.first_burst_length)
                     || (!sess->sess_params.first_burst_length)));
        if (scsi_cmd->trans_len-scsi_cmd->bytes_todev) {
            TRACE(TRACE_DEBUG, 0, "REACHED FIRST BURST\n");
        }
        TRACE(TRACE_DEBUG, 0, "successfully sent %u of %u bytes write data\n", 
              scsi_cmd->bytes_todev, scsi_cmd->trans_len); 
    }
    if (scsi_cmd->todev&&(scsi_cmd->trans_len-scsi_cmd->bytes_todev)) {
        TRACE(TRACE_DEBUG, 0, 
              "expecting R2T for remaining %u bytes write data\n", 
              scsi_cmd->trans_len-scsi_cmd->bytes_todev);
    }
#ifndef NO_SG_MALLOC
    if (fragment_flag) iscsi_free_atomic(sg_copy_orig);
#endif
    sess->CmdSN++;

    return 0;

 error: 
#ifndef NO_SG_MALLOC
    if (fragment_flag) iscsi_free_atomic(sg_copy);
#endif
    return -1;
}

static int reject_i(INITIATOR_SESSION_T *sess, unsigned char *header) {
    INITIATOR_CMD_T *cmd = NULL;
    ISCSI_REJECT_T reject;
    unsigned char bad_header[ISCSI_HEADER_LEN];
    unsigned tag;

    // Get & check args
  
    if (iscsi_reject_decap(header, &reject)!=0) {
        TRACE_ERROR("iscsi_reject_decap() failed\n");
        return -1;
    }
    RETURN_NOT_EQUAL("reject.length", reject.length, ISCSI_HEADER_LEN, 
                     NO_CLEANUP, -1);

    // Read bad header, extract tag, and get cmd from hash table

    if (iscsi_sock_msg(sess->sock, 0, ISCSI_HEADER_LEN, bad_header, 0)!=
        ISCSI_HEADER_LEN) {
        TRACE_ERROR("iscsi_sock_msg() failed\n");
        return -1;
    }
    tag = NTOHL(*((unsigned *)(bad_header+16))); 
    TRACE_ERROR("REJECT PDU: tag 0x%x (reason 0x%x)\n", tag, reject.reason);
    if (tag != 0xffffffff) {
        if ((cmd = hash_get(&g_cmd_hash[sess->isid], tag))==NULL) {
            TRACE_ERROR("no cmd associated w/ tag 0x%x\n", tag);
        } else {
            TRACE_ERROR(
                        "cmd %p associated w/ tag 0x%x (waiting for tx_done)\n",
                        cmd, tag);
            ISCSI_WAIT(&sess->rx_worker.wq, cmd->tx_done); 
            TRACE_ERROR("cmd %p associated w/ tag 0x%x (tx_done is %i)\n", 
                        cmd, tag, cmd->tx_done);
        }
    } else {
        TRACE_ERROR("no command associated w/ tag 0x%x\n", tag);
    }

    /* complete command */
    cmd->status = -1;
    if (iscsi_done_i(cmd)!=0) {
        TRACE_ERROR("iscsi_done_i() failed\n");
        return -1;
    }

    return 0;
}

static int async_msg_i(INITIATOR_SESSION_T *sess, unsigned char *header) {
    ISCSI_AMSG_T msg;

    // Get & check args
    if (iscsi_amsg_decap(header, &msg)!=0) {
        TRACE_ERROR("iscsi_amsg_decap() failed\n");
        return -1;
    }

    sess->CmdSN = msg.ExpCmdSN;
    sess->MaxCmdSN = msg.MaxCmdSN;
    sess->ExpStatSN = msg.StatSN + 1;

    // Read Sense Data
    if (msg.length) {
        unsigned char *sense_data = NULL;
        if ((sense_data=iscsi_malloc(msg.length))==NULL) {
            TRACE_ERROR("iscsi_malloc() failed\n"); 
            return -1;
        }
        TRACE(TRACE_DEBUG, 0, "reading %i bytes sense data \n", msg.length);
        if (iscsi_sock_msg(sess->sock, 0, msg.length, sense_data, 0)!=
            msg.length) {
            TRACE_ERROR("iscsi_sock_msg() failed\n");
            if (sense_data != NULL) iscsi_free(sense_data);
            return -1;
        }
        TRACE(TRACE_DEBUG, 0, 
              "read %i bytes sense data ok (currently discarding)\n", 
              msg.length);
        PRINT_BUFF(TRACE_SCSI, 0, sense_data, msg.length, 32);
        if (sense_data != NULL) iscsi_free(sense_data);
    } else {
        TRACE(TRACE_DEBUG, 0, "no sense data available\n");
    }

    switch (msg.AsyncEvent) {
    case 0:
        // Ignore SCSI asyn messages for now
        break;
    case 1:
    case 4:
        // Ignore Parameter Negotiation. Send Logout
        logout_phase_i(sess);
    case 2:
    case 3:
        if (iscsi_sock_shutdown(sess->sock, ISCSI_SOCK_SHUTDOWN_SEND)!=0) {
            TRACE_ERROR("iscsi_sock_shutdown() failed\n");
        }
        return -1;
    case 255:
        break;
    default:
        break;
    }

    return 0;
}

#define NOI_CLEANUP {if (ping_data) iscsi_free_atomic(ping_data);}
#define NOI_ERROR {NOI_CLEANUP; return -1;}

static int nop_in_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd, 
                    unsigned char *header) {
    ISCSI_NOP_OUT_T *nop_out = NULL;
    ISCSI_NOP_IN_T nop_in;
    unsigned char *ping_data = NULL;
    int i;

    /* parse command and read in any data */
    if (iscsi_nop_in_decap(header, &nop_in)!=0) {
        TRACE_ERROR("iscsi_nop_in() failed\n");
        return -1;
    }
    if (nop_in.length) {
        if ((ping_data=iscsi_malloc_atomic(nop_in.length))==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
            return -1;
        }
        if (iscsi_sock_msg(sess->sock, 0, nop_in.length, ping_data, 0)!=
            nop_in.length) {
            TRACE_ERROR("iscsi_sock_msg() failed\n");
            NOI_ERROR;
        }
        TRACE(TRACE_DEBUG, 0, "successfully read %i bytes ping data\n", 
              nop_in.length);
    } 

    if (cmd) {

        /* this nop_in is in response to previous nop_out */
        nop_out = (ISCSI_NOP_OUT_T *) cmd->ptr;
        if (nop_in.length) {
            for (i=0; i<nop_in.length; i++) {
                if (nop_out->data[i]!= ping_data[i]) {
                    TRACE_ERROR(
                                "Bad ping data[%i]. Got 0x%x, expected 0x%x\n",
                                i, ping_data[i], nop_out->data[i]);
                    NOI_ERROR;
                }
            }
        }
        RETURN_NOT_EQUAL("nop_in.length", nop_in.length, nop_out->length, 
                         NOI_ERROR, -1);

    }  else {

        /* this nop_in was unsolicited */
        RETURN_NOT_EQUAL("nop_in.tag", nop_in.tag, 0xffffffff, NOI_ERROR, -1);
        RETURN_NOT_EQUAL("nop_in.length", nop_in.length, 0, NOI_ERROR, -1);
        g_target[sess->isid].nop_in_count++;
        if (nop_in.transfer_tag != 0xffffffff) {
            memset(&g_nop_out, 0, sizeof(ISCSI_NOP_OUT_T));
            g_nop_out[sess->isid].tag = 0xffffffff;
            g_nop_out[sess->isid].immediate = 1;
            g_nop_out[sess->isid].transfer_tag = nop_in.transfer_tag;
            g_nop_out[sess->isid].lun = nop_in.lun;
            g_nop_out[sess->isid].ExpStatSN = sess->ExpStatSN;
            /* should use this to sychronize */
            g_nop_out[sess->isid].CmdSN = nop_in.ExpCmdSN;
            ISCSI_WAKE_ELSE(g_nop_out_waiting[sess->isid] = 1, 
                            &sess->tx_worker.wq, return -1);
        }
    }
        
    NOI_CLEANUP;

    // Check and update numbering 
    WARN_NOT_EQUAL("StatSN", nop_in.StatSN, sess->ExpStatSN);
    //WARN_NOT_EQUAL("ExpCmdSN", nop_in.ExpCmdSN, sess->CmdSN);
    if (cmd) {
        sess->ExpStatSN++;
    }

    /* finish command if this was a response to a ping */
    if (cmd) {
        cmd->status = 0;
        if (iscsi_done_i(cmd)!=0) {
            TRACE_ERROR("iscsi_done_i() failed\n");
            return -1;
        }
    }
    
    return 0;
}

static int scsi_r2t_rx_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd, 
                         unsigned char *header) {

    /* The INITIATOR_CMD_T contains a single r2t structure for storing
       incoming R2T data. If we end up supporting multiple outstanding
       R2T from a target, we'll need to have an array of r2t structures, or
       maybe a queue. */
    if (cmd->r2t_request++ > 1) {
        TRACE_ERROR("multiple outstanding R2Ts not yet implemented\n");
        return -1;
    }
    if (iscsi_r2t_decap(header, &cmd->r2t)!=0) {
        TRACE_ERROR("iscsi_r2t_decap() failed\n");
        return -1;
    }

#if 0
    PRINT("cmd 0x%x: translen=%u bytes_fromdev=%u bytes_todev=%u r2t off %u len %u\n", 
          cmd->tag,
          ((ISCSI_SCSI_CMD_T *)(cmd->ptr))->trans_len,
          ((ISCSI_SCSI_CMD_T *)(cmd->ptr))->bytes_fromdev,
          ((ISCSI_SCSI_CMD_T *)(cmd->ptr))->bytes_todev,
          cmd->r2t.offset,
          cmd->r2t.length);
#endif

    if ((cmd->r2t.offset+cmd->r2t.length) == ((ISCSI_SCSI_CMD_T *)(cmd->ptr))->trans_len) {
        /* We're assuming that the target's R2T request arrive in order. Need to fix this.*/
        HASH_REMOVE_ELSE(&g_cmd_hash[cmd->isid], cmd->tag, return -1);
    }

    /* wake up worker */
    ISCSI_WAKE_ELSE((iscsi_queue_insert(&g_tx_queue[cmd->isid], cmd)==0), 
                    &sess->tx_worker.wq, return -1);

    return 0;
}

#ifdef NO_SG_MALLOC
#define FF_CLEANUP
#else
#define FF_CLEANUP {if (fragment_flag) iscsi_free_atomic(sg_copy_orig);}
#endif

static int scsi_r2t_tx_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd) {
    ISCSI_SCSI_CMD_T *scsi_cmd; 
    ISCSI_WRITE_DATA_T data;
    unsigned bytes_todev;
    unsigned DataSN;
    struct iovec sg_singleton;
    struct iovec *sg, *sg_copy, *sg_copy_orig, *sg_which;
    int sg_len, sg_len_copy, sg_len_which;
    int fragment_flag;
    int rc;
    unsigned char header[ISCSI_HEADER_LEN];

    /* check args and update numbering */
    NOT_EQUAL_ELSE("cmd", (unsigned long) cmd, 0, return -1);
    scsi_cmd = (ISCSI_SCSI_CMD_T *) cmd->ptr;
    NOT_EQUAL_ELSE("scsi_cmd", (unsigned long) scsi_cmd, 0, return -1);
    NOT_EQUAL_ELSE("scsi_cmd->send_data", (unsigned long) scsi_cmd->send_data, 
                   0, return -1);
    EQUAL_ELSE("cmd->tag", cmd->tag, scsi_cmd->tag, return -1);
    NOT_EQUAL_ELSE("cmd->r2t.length", cmd->r2t.length, 0, return -1);
    //WARN_NOT_EQUAL("StatSN", cmd->r2t.StatSN, sess->ExpStatSN); 
    //WARN_NOT_EQUAL("ExpCmdSN", cmd->r2t.ExpCmdSN, sess->CmdSN);
    sess->MaxCmdSN = cmd->r2t.MaxCmdSN;

    /* send back requested data */
    TRACE(TRACE_ISCSI, 0, "sending %i bytes R2T write data (offset %u)\n", 
          cmd->r2t.length, cmd->r2t.offset);
    sg = sg_copy = sg_copy_orig = sg_which = NULL;
    sg_len = sg_len_copy = sg_len_which = 0;
    if (scsi_cmd->send_sg_len) {
        sg = (struct iovec *) scsi_cmd->send_data;
        sg_len = scsi_cmd->send_sg_len;
        if (sg == NULL) {
            TRACE_ERROR("sg is NULL\n");
            return -1;
        }
    } else {
        sg_singleton.iov_base = scsi_cmd->send_data;
        sg_singleton.iov_len = scsi_cmd->trans_len;
        sg = &sg_singleton;
        sg_len = 1;
    }
    fragment_flag = 0;
    bytes_todev = 0;
    DataSN = 0;
    do {
        memset(&data, 0, sizeof(ISCSI_WRITE_DATA_T));
        if (sess->sess_params.max_data_seg_length)  {
            data.length = MIN(sess->sess_params.max_data_seg_length, 
                              cmd->r2t.length-bytes_todev);
        } else {
            data.length = cmd->r2t.length-bytes_todev;
        }
        if (bytes_todev+data.length==cmd->r2t.length) {
            data.final = 1;
        }
        data.tag = cmd->r2t.tag;
        data.transfer_tag = cmd->r2t.transfer_tag;
        data.ExpStatSN = sess->ExpStatSN;
        data.DataSN = DataSN++;
        data.offset = cmd->r2t.offset + bytes_todev;
        data.lun = scsi_cmd->lun;
        if (iscsi_write_data_encap(header, &data)!=0) {
            TRACE_ERROR("iscsi_write_data_encap() failed\n");
            FF_CLEANUP;
            return -1;
        }
        if ((data.length<cmd->r2t.length)||(cmd->r2t.offset)) {
            if (data.length<cmd->r2t.length) {
                TRACE(TRACE_ISCSI, 1, 
                      "R2T data being fragmented: sending %u bytes of %u\n", 
                      data.length, cmd->r2t.length); 
            } else {
                TRACE(TRACE_ISCSI, 1, 
                      "R2T data starts at offset %u, desired length %u\n", 
                      cmd->r2t.offset, cmd->r2t.length); 
            }

            /* Allocate space for a copy of the original iovec */           
            if (!fragment_flag) {
#ifdef NO_SG_MALLOC
                sg_copy_orig = cmd->sg_copy_1;
#else
                if ((sg_copy_orig=
                     iscsi_malloc_atomic(sg_len*sizeof(struct iovec)))==NULL) { /* MALLOC 3 */
                    TRACE_ERROR("iscsi_malloc_atomic() failed\n");
                    return -1;
                }
#endif
                fragment_flag++;
            } 
            
            /* Copy and modify original iovec with new offset and length */

            TRACE(TRACE_ISCSI, 1, "modifying orig iovec w/ off %u length %u\n", 
                  cmd->r2t.offset+bytes_todev, data.length); 
            sg_copy = sg_copy_orig;
            sg_len_copy = sg_len;
	    if (sg_len > CONFIG_INITIATOR_MAX_SG) {
		PANIC("sg_len %i is greater than CONFIG_INITIATOR_MAX_SG %i\n",
		      sg_len, CONFIG_INITIATOR_MAX_SG);
	    }
            memcpy(sg_copy, sg, sizeof(struct iovec)*sg_len);
            if (modify_iov(&sg_copy, &sg_len_copy, cmd->r2t.offset+bytes_todev,
                           data.length)!=0) {
                TRACE_ERROR("modify_iov() failed\n");
                FF_CLEANUP;
                return -1;
            }
            sg_which = sg_copy;
            sg_len_which = sg_len_copy;
        } else {
            TRACE(TRACE_ISCSI, 1, 
                  "using orig iovec for R2T transfer (offset %u, length %u)\n", 
                  cmd->r2t.offset, cmd->r2t.length);
            sg_which = sg;
            sg_len_which = sg_len;
        }
        TRACE(TRACE_ISCSI, 0, 
              "0x%x: sending R2T write data (off %u, len %u, sg_len %u, "
              "data %p)\n", scsi_cmd->tag, data.offset, data.length, 
              sg_len_which, sg_which);
        if ((rc=iscsi_sock_send_header_and_data(sess->sock, header, 
                                                ISCSI_HEADER_LEN, sg_which, 
                                                data.length, sg_len_which))
            !=ISCSI_HEADER_LEN+data.length) {
            TRACE_ERROR("iscsi_sock_send_header_and_data() failed (got %i, "
                        "expected %i)\n", rc, ISCSI_HEADER_LEN+data.length);
            FF_CLEANUP;
            return -1;
        }
        TRACE(TRACE_ISCSI, 0, 
              "0x%x: sent write data PDU OK (offset %u, len %u)\n", 
              scsi_cmd->tag, data.offset, data.length);
        bytes_todev += data.length;
        scsi_cmd->bytes_todev += data.length;
    } while (bytes_todev<cmd->r2t.length);

    FF_CLEANUP;

    return 0;
}

static int scsi_response_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd, 
                           unsigned char *header) {
    ISCSI_SCSI_CMD_T *scsi_cmd;
    ISCSI_SCSI_RSP_T scsi_rsp;
    
    /* check args and update numbering */
    NOT_EQUAL_ELSE("cmd", (unsigned long) cmd, 0, return -1);
    scsi_cmd = (ISCSI_SCSI_CMD_T *) cmd->ptr;
    NOT_EQUAL_ELSE("scsi_cmd", (unsigned long) scsi_cmd, 0, return -1);
    if (iscsi_scsi_rsp_decap(header, &scsi_rsp)!=0) {
        TRACE_ERROR("iscsi_scsi_rsp_decap() failed\n");
        return -1;
    }
    EQUAL_ELSE("o bit (FIX ME)", scsi_rsp.bidi_overflow, 0, goto failed);
    EQUAL_ELSE("u bit (FIX ME)", scsi_rsp.bidi_underflow, 0, goto failed);
    EQUAL_ELSE("O bit (FIX ME)", scsi_rsp.overflow, 0, goto failed);
    EQUAL_ELSE("iSCSI Response (FIX ME)", scsi_rsp.response, 0, goto failed);
    EQUAL_ELSE("Tag", scsi_rsp.tag, scsi_cmd->tag, goto failed);
    EQUAL_ELSE("Bidi Residual Count", scsi_rsp.bidi_res_cnt, 0, goto failed);
    WARN_NOT_EQUAL("StatSN", scsi_rsp.StatSN, sess->ExpStatSN);

    sess->ExpStatSN = scsi_rsp.StatSN + 1;
    if (sess->sess_params.max_data_seg_length)
        LESS_THAN_EQUAL_ELSE("DataSegmentLength (FIX ME)", scsi_rsp.length, 
                             sess->sess_params.max_data_seg_length, 
                             goto failed);   
    if ((scsi_rsp.status==0)&&(scsi_rsp.length!=0)) {
        TRACE_ERROR("Unexpected DataSegmentLength %u with GOOD SCSI status\n", 
                    scsi_rsp.length);
        return -1;
    }
    /* WARN_NOT_EQUAL("ExpCmdSN", scsi_rsp.ExpCmdSN, sess->CmdSN); */
    sess->MaxCmdSN = scsi_rsp.MaxCmdSN;

    /* make sure all data was transferred, else read sense data */
    if (scsi_rsp.status==0) {
        if (scsi_cmd->todev) {
            EQUAL_ELSE("scsi_cmd->bytes_todev", scsi_cmd->bytes_todev, 
                       scsi_cmd->trans_len, goto failed);
            if (scsi_cmd->fromdev) {
                EQUAL_ELSE("scsi_cmd->bytes_fromdev", scsi_cmd->bytes_fromdev, 
                           scsi_cmd->bidi_trans_len, goto failed);
            }
        } 
    } else if (scsi_rsp.length) {
        unsigned char *sense_data = NULL;

        if ((sense_data=iscsi_malloc(scsi_rsp.length))==NULL) {
            TRACE_ERROR("iscsi_malloc() failed\n"); 
            return -1;
        }
        TRACE_ERROR("reading %i bytes sense data (recv_sg_len %u)\n", 
                    scsi_rsp.length, scsi_cmd->recv_sg_len);
        if (iscsi_sock_msg(sess->sock, 0, scsi_rsp.length, sense_data, 0)!=
            scsi_rsp.length) {
            TRACE_ERROR("iscsi_sock_msg() failed\n");
            if (sense_data != NULL) iscsi_free(sense_data);
            return -1;
        }
        TRACE_ERROR("read %i bytes sense data ok (currently discarding)\n", 
                    scsi_rsp.length);
        if (sense_data != NULL) iscsi_free(sense_data);
    }

    /* complete command */

    cmd->status = 0;
    scsi_cmd->status = scsi_rsp.status;
    iscsi_done_i(cmd);
    return 0;

 failed:
    cmd->status = -1;
    iscsi_done_i(cmd);
    return -1;
}

static int scsi_read_data_i(INITIATOR_SESSION_T *sess, INITIATOR_CMD_T *cmd, 
                            unsigned char *header) {
    ISCSI_READ_DATA_T data;
    ISCSI_SCSI_CMD_T *scsi_cmd;
    int rc;

    /* decap and check args */
    NOT_EQUAL_ELSE("cmd", (unsigned long) cmd, 0, goto failed);
    if ((scsi_cmd=(ISCSI_SCSI_CMD_T *) cmd->ptr)==NULL) {
        TRACE_ERROR("no ISCSI_SCSI_CMD_T associated w/ INITIATOR_CMD_T??\n"); 
        goto failed;
    }
    if (iscsi_read_data_decap(header, &data)!=0) {
        TRACE_ERROR("iscsi_scsi_rsp_decap() failed\n");
        goto failed;
    }
    EQUAL_ELSE("cmd->type", cmd->type, ISCSI_SCSI_CMD, goto failed);
#if 0
    EQUAL_ELSE("Overflow bit", data.overflow, 0, goto failed);
#else
    WARN_NOT_EQUAL("Overflow bit", data.overflow, 0);
#endif
    WARN_NOT_EQUAL("Tag", data.task_tag, scsi_cmd->tag);
    if (sess->sess_params.max_data_seg_length)
        LESS_THAN_EQUAL_ELSE("Length", data.length, 
                             sess->sess_params.max_data_seg_length, 
                             goto failed);

    /* update numbering */
    //WARN_NOT_EQUAL("ExpCmdSN", data.ExpCmdSN, sess->CmdSN);
    sess->MaxCmdSN = data.MaxCmdSN;

    /* Need to optimize this section */
    if (scsi_cmd->recv_sg_len) {
        int sg_len = scsi_cmd->recv_sg_len;
        struct iovec *sg;
        struct iovec *sg_orig = NULL;
        unsigned total_len, disp; 
        int i;

        if (data.length!=scsi_cmd->trans_len) {

            /* Make a copy of the iovec */
#ifdef NO_SG_MALLOC
            sg_orig = sg = cmd->sg_copy_1;
#else           
            if ((sg_orig=sg=
                iscsi_malloc_atomic(sizeof(struct iovec)*sg_len))==NULL) { /* MALLOC 4 */
                TRACE_ERROR("iscsi_malloc_atomic() failed\n");
                return -1;

            }
#endif
            memcpy(sg, scsi_cmd->recv_data, sizeof(struct iovec)*sg_len);

            /* Find offset in iovecs */
            total_len = 0;
            disp = data.offset;
            for (i=0; i<sg_len; i++) {
                total_len += sg[i].iov_len;
                if (total_len>data.offset) {
                    break;
                }
                disp -= sg[i].iov_len;
            }
            sg[i].iov_len -= disp;
            sg[i].iov_base += disp;
            sg_len -= i;
            sg = &sg[i];

            /* Find last iovec needed for read */
            total_len = 0;
            for (i=0; i<sg_len; i++) {
                total_len += sg[i].iov_len;
                if (total_len>=data.length) {
                    break;
                }
            }
            sg[i].iov_len -= (total_len - data.length);
            sg_len = i+1;
        } else {
            sg = (struct iovec *) scsi_cmd->recv_data;
            for (i=0; i<sg_len; i++) {
                if (sg[i].iov_base == NULL) {
                    TRACE_ERROR("sg[%i].iov_base is NULL (len %Zu)\n", i, 
                                sg[i].iov_len);
                    return -1;
                }
            }
        }
        if ((rc = iscsi_sock_msg(sess->sock, 0, data.length, 
                                 (unsigned char *)sg, sg_len))!=data.length) {
            TRACE_ERROR("iscsi_sock_msg() failed: got %u, expected %u\n", 
                        rc, data.length);
#ifndef NO_SG_MALLOC
            if (sg_orig) iscsi_free_atomic(sg_orig);
#endif
            return -1;
        }
        scsi_cmd->bytes_fromdev += data.length;
#ifndef NO_SG_MALLOC
        if (sg_orig) iscsi_free_atomic(sg_orig);
#endif
    } else {
        if (data.length) {
            int bytes_read = 0;
            do {
                if ((rc=iscsi_sock_msg(sess->sock, 0, data.length-bytes_read, 
                                       scsi_cmd->recv_data+data.offset+bytes_read, 0))<0){
                    TRACE_ERROR("iscsi_sock_msg() failed (rc %i)\n", rc);
                    return -1;
                }
                if (rc != data.length) {
                    TRACE_WARN("short read: %u of %u bytes\n", rc, data.length-bytes_read);
                }
                bytes_read += data.length;
            } while (bytes_read < data.length);
            scsi_cmd->bytes_fromdev += data.length;
        }
    }

    /* Check for status */
    if (data.S_bit) {
        TRACE(TRACE_DEBUG, 0, "received status with final PDU\n");
        RETURN_NOT_EQUAL("Final Bit", data.final, 1, NO_CLEANUP, -1);
        WARN_NOT_EQUAL("StatSN", data.StatSN, sess->ExpStatSN++);
        scsi_cmd->status = data.status = 0; 
        cmd->status = 0;
        TRACE(TRACE_DEBUG, 0, "scsi op 0x%x done (tag %u, status %i)\n", 
              scsi_cmd->cdb[0], scsi_cmd->tag, scsi_cmd->status);
        if (iscsi_done_i(cmd)!=0) {
            TRACE_ERROR("iscsi_done_i() failed\n");
            return -1;
        }
    } 
    TRACE(TRACE_ISCSI, 1, "read %i data bytes (total offset %u)\n", 
          data.length, data.offset);

    return 0;
 failed:
    cmd->status = -1;
    iscsi_done_i(cmd);
    return -1;
}

int initiator_show_config(int terse, int ssid) {
    INITIATOR_SESSION_T *sess;

    sess = g_target[ssid].sess;
    if (terse) {
        PRINT("%26s %3s %3s %6s ", 
              g_target[sess->isid].TargetName,
              param_val(sess->params, "InitialR2T"),
              param_val(sess->params, "ImmediateData"),
              param_val(sess->params, "MaxRecvDataSegmentLength")); 
    } else {
        PRINT("* %25s:%44s *\n", "InitiatorName", 
              param_val(sess->params, "InitiatorName"));
        PRINT("* %25s:%44s *\n", "TargetName", g_target[sess->isid].TargetName);
        PRINT("* %25s:%44s *\n", "Type", 
              param_val(sess->params, "SessionType"));
        PRINT("* %25s:%44"PRIu64" *\n", "ISID", sess->isid);
        PRINT("* %25s:%44u *\n", "TSIH", sess->tsih);
        PRINT("* %25s:%44s *\n", "InitialR2T",  
              param_val(sess->params, "InitialR2T")); 
        PRINT("* %25s:%44s *\n", "ImmediateData",  
              param_val(sess->params, "ImmediateData"));
        PRINT("* %25s:%44s *\n", "MaxRecvDataSegmentLength",  
              param_val(sess->params, "MaxRecvDataSegmentLength"));
        PRINT("* %25s:%44s *\n", "MaxBurstLength",  
              param_val(sess->params, "MaxBurstLength"));
        PRINT("* %25s:%44s *\n", "FirstBurstLength", 
              param_val(sess->params, "FirstBurstLength"));
    }
    return 0;
}


int initiator_info(char *ptr) {
    int i;
    char state[64];

    ptr[0] = '\0';
    sprintf(ptr+strlen(ptr), "%3s %15s %4s %20s %6s %8s %8s %s\n", 
            "TID", "IP Address", "Port", "State", "Errors", "Nop-in", 
            "Nop-out", "TargetName");
    for(i=0; i<g_num_targets; i++) {
        sprintf(state, "0x%x (", g_target[i].state);
        if (g_target[i].state & SESSION_STATE_ERROR) {
            sprintf(state+strlen(state), "ERROR");
        } else if (g_target[i].state & SESSION_STATE_FULL_FEATURE) {
            sprintf(state+strlen(state), "FULL FEATURE");
        } else if (g_target[i].state & SESSION_STATE_LOGGED_IN) {
            sprintf(state+strlen(state), "LOGGED IN");
        } else if (g_target[i].state & SESSION_STATE_CONNECTED) {
            sprintf(state+strlen(state), "CONNECTED");
        } else if (g_target[i].state & SESSION_STATE_INITIALIZED) {
            sprintf(state+strlen(state), "INITIALIZED");
        } else if (g_target[i].state & SESSION_STATE_STARTED) {
            sprintf(state+strlen(state), "STARTING...");
        } else {
            sprintf(state+strlen(state), "UNKNOWN STATE");
        }
        sprintf(state+strlen(state), ")");
        sprintf(ptr+strlen(ptr), "%3i %15s %4d %20s %6u %8i %8i %s\n", 
                i, g_target[i].ip, g_target[i].port, state,
                g_target[i].errors, g_target[i].nop_in_count,  
                g_target[i].nop_out_count,
                g_target[i].TargetName);
    } 
    return strlen(ptr);
}
