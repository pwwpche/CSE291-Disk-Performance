
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
#include <endian.h>
#else
#include <stdlib.h>
#include <netinet/tcp.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/wait.h>
#endif
#include <inttypes.h>

#include "target.h"
#include "device.h"
#include "md5.h"
#include "parameters.h"
#include "debug.h"
#include "research.h"

#define BASE_DEVICE_ID 1

/*
 * Constants
 */

#define TARGET_INITIALIZING  1
#define TARGET_INITIALIZED   2
#define TARGET_SHUTTING_DOWN 3
#define TARGET_SHUT_DOWN     4

extern uint32_t g_disk_block_len[];

/*
 * Globals
 */

static void* g_cookie[CONFIG_TARGET_MAX_SESSIONS];
static TARGET_SESSION_T g_session[CONFIG_TARGET_MAX_SESSIONS];
static TARGET_CMD_T *g_cmd[CONFIG_TARGET_MAX_SESSIONS];
static unsigned char g_header[CONFIG_TARGET_MAX_SESSIONS]
                             [CONFIG_TARGET_MAX_QUEUE]
                             [ISCSI_HEADER_LEN];
static ISCSI_QUEUE_T g_session_q;
static ISCSI_QUEUE_T g_cmd_q[CONFIG_TARGET_MAX_SESSIONS];
static ISCSI_QUEUE_T g_header_q[CONFIG_TARGET_MAX_SESSIONS];
static iscsi_socket_t g_sock;
static int g_listener_pid;
volatile int g_listener_listening;
static int g_state = TARGET_SHUT_DOWN;
static char g_TargetAddress[64];  
char g_hostname[64];
#if 0
static TARGET_STATS_T g_stats;
#endif
static int g_silent_login = 1;

/*
 * Internal functions
 */

static int execute_t(TARGET_SESSION_T *sess, unsigned char *header);
static int login_command_t(TARGET_SESSION_T *sess, unsigned char* header);
static int logout_command_t(TARGET_SESSION_T *sess, unsigned char* header);
static int text_command_t(TARGET_SESSION_T *sess, unsigned char* header);
static int nop_out_t(TARGET_SESSION_T *sess, unsigned char* header);
static int task_command_t(TARGET_SESSION_T *sess, unsigned char* header);
static int scsi_command_t(TARGET_SESSION_T *sess, unsigned char* header);
static int reject_t(TARGET_SESSION_T *sess, unsigned char* header, 
                    unsigned char reason);
static int worker_proc_t(void *arg);

/*
 * External Functions (disk.c and osd.c use these)
 */

/*
 * This function reads the output of iscsi-sample-counters through a pipe
 */

static int read_system_counters(char *data) {
    int pfd[2];
    int pid;
    int rc;
    char cmd[256];

    if (pipe(pfd) == -1) {
        perror("pipe failed");
        return -1;
    }
    if ((pid = fork()) < 0) {
        perror("fork failed");
        return -1;
    }
    if (pid == 0) {
        close(pfd[0]); 
        dup2(pfd[1], 1); 
        close(pfd[1]);
        sprintf(cmd, "iscsi-sample-counters");
        if (execlp(cmd, "iscsi-sample-counters", (char *) 0)==-1) {
            TRACE_ERROR("error executing iscsi-sample-counters\n");
            return -1;
        }
    } else {
        close(pfd[1]);
        dup2(pfd[0], 0);
        rc = read(pfd[0], data, 4096);
        close(pfd[0]);
        if (waitpid(pid, &rc, 0)==-1) {
            TRACE_ERROR("waitpid() failed\n");
            return -1;
        }
        if (rc!=0) {
            TRACE_ERROR("iscsi-sample-counters failed\n");
            return -1;
        }
    }
    return 0;
}

/*
 * This function reads all the counters (platform, target, and device)
 */

int read_counters(uint64_t lun, char *data) {
    OST_DEVICE_COUNTERS_T *dstats;

    /* system counters (should read from a pipe) */
    memset(data, 0, 4096);
    if (read_system_counters(data)!=0) {
        TRACE_ERROR("failed to read system counters\n");
        return -1;
    }

    dstats = device_counters_get(lun);

    sprintf(data+strlen(data), "t_OPS %"PRIu64" t_OPS_WR %"PRIu64
            " t_OPS_RD %"PRIu64" t_OPS_BIDI %"PRIu64" t_OPS_OTHER %"PRIu64" ",
            dstats->ops, dstats->ops_wr, dstats->ops_rd, 
            dstats->ops_bidi, dstats->ops_other);
    sprintf(data+strlen(data), "t_DEPTHS %"PRIu64" t_DEPTHS_WR %"PRIu64
            " t_DEPTHS_RD %"PRIu64" t_DEPTHS_BIDI %"PRIu64
            " t_DEPTHS_OTHER %"PRIu64" ",
            dstats->depths, dstats->depths_wr, dstats->depths_rd, 
            dstats->depths_bidi, dstats->depths_other);
    sprintf(data+strlen(data), "t_LAT %"PRIu64" t_LAT_WR %"PRIu64
            " t_LAT_RD %"PRIu64" t_LAT_BIDI %"PRIu64" t_LAT_OTHER %"PRIu64" ",
            dstats->lats, dstats->lats_wr, dstats->lats_rd,
            dstats->lats_bidi, dstats->lats_other);
    sprintf(data+strlen(data), "t_TODEV %"PRIu64" t_FROMDEV %"PRIu64
            " t_JUMPS %"PRIu64" t_JUMPS_WR %"PRIu64" t_JUMPS_RD %"PRIu64" ", 
            dstats->blocks_wr*g_disk_block_len[BASE_DEVICE_ID], 
            dstats->blocks_rd*g_disk_block_len[BASE_DEVICE_ID],
            dstats->jumps, dstats->jumps_wr, dstats->jumps_rd);
    sprintf(data+strlen(data), "\n");

    return 0;
}

int target_init(int queue_depth) {
    int i, j;

    TRACE(TRACE_DEBUG, 0, "initializing target\n");
    if ((g_state==TARGET_INITIALIZING)||(g_state==TARGET_INITIALIZED)) {
        TRACE_ERROR("duplicate target initialization attempted\n");
        return -1;
    }

    /* set target name */
    sprintf(g_hostname, "iqn.2003-08.com.intel:");
#ifdef __KERNEL__
    sprintf(g_hostname+strlen(g_hostname), "%s", system_utsname.nodename);
#else
    gethostname(g_hostname+strlen(g_hostname), 64);
#endif

    g_state = TARGET_INITIALIZING;
    g_listener_listening = 0;
    g_listener_pid = -1;

    /* initialize the device */
    if (device_init()!=0) {
        TRACE_ERROR("device_init() failed\n");
        return -1;
    }

    /* allocated target commands */
    for (i=0; i<CONFIG_TARGET_MAX_SESSIONS; i++) {
        if ((g_cmd[i] = iscsi_malloc(CONFIG_TARGET_MAX_QUEUE*
                                     sizeof(TARGET_CMD_T)))==NULL) {
            TRACE_ERROR("iscsi_malloc() failed\n");
            return -1;
        }
    }

    PRINT("TARGET: allocated %u MB for target command structures\n", 
        (unsigned) (CONFIG_TARGET_MAX_QUEUE*CONFIG_TARGET_MAX_SESSIONS*sizeof(TARGET_CMD_T))>>20);

    /* create queue of free sessions structures */
    if (iscsi_queue_init(&g_session_q, CONFIG_TARGET_MAX_SESSIONS)!=0) {
        TRACE_ERROR("iscsi_queue_init() failed\n");
        return -1;
    }
    for (i=0;i<CONFIG_TARGET_MAX_SESSIONS; i++) {
        ISCSI_SPIN_LOCK_INIT_ELSE(&g_session[i].slock, return -1);
        g_session[i].id = i; /* do we need this ?*/
        if (iscsi_queue_insert(&g_session_q, &g_session[i])!=0) {
            TRACE_ERROR("iscsi_queue_insert() failed\n");
            return -1;
        }
    }

    /* create queue of free iSCSI headers */
    for (i=0;i<CONFIG_TARGET_MAX_SESSIONS; i++) {
        if (iscsi_queue_init(&g_header_q[i], CONFIG_TARGET_MAX_QUEUE)!=0) {
            TRACE_ERROR("iscsi_queue_init() failed\n");
            return -1;
        }
        for (j=0;j<CONFIG_TARGET_MAX_QUEUE; j++) {
            if (iscsi_queue_insert(&g_header_q[i], &g_header[i][j])!=0) {
                TRACE_ERROR("iscsi_queue_insert() failed\n");
                return -1;
            }
        }
    }

    /* create queue of free target command structures */
    for (i=0;i<CONFIG_TARGET_MAX_SESSIONS; i++) {
        if (iscsi_queue_init(&g_cmd_q[i], CONFIG_TARGET_MAX_QUEUE)!=0) {
            TRACE_ERROR("iscsi_queue_init() failed\n");
            return -1;
        }
        for (j=0;j<CONFIG_TARGET_MAX_QUEUE; j++) {
            ISCSI_WQ_INIT(&g_cmd[i][j].ready_queue);
            if (iscsi_queue_insert(&g_cmd_q[i], &g_cmd[i][j])!=0) {
                TRACE_ERROR("iscsi_queue_insert() failed\n");
                return -1;
            }
        }
    }

    /* intialize security cookies */
    memset(g_cookie, 0, sizeof(void *)*CONFIG_TARGET_MAX_SESSIONS);

    g_state = TARGET_INITIALIZED;
    TRACE(TRACE_DEBUG, 0, "target initialized\n");

    return 0;
}

int target_shutdown(void) {
    int i, j;

    TRACE(TRACE_DEBUG, 0, "shutting down target\n");
    if ((g_state==TARGET_SHUTTING_DOWN)||(g_state==TARGET_SHUT_DOWN)) {
        TRACE_ERROR("duplicate target shutdown attempted\n");
        return -1;
    }
    g_state = TARGET_SHUTTING_DOWN;

    /* destroy all sessions */
    for (i=0;i<CONFIG_TARGET_MAX_SESSIONS; i++) {
        if (g_session[i].IsLoggedIn) {
            STOP_WORKER_ELSE(g_session[i].worker, 
                             (iscsi_sock_shutdown(g_session[i].sock, 2)==0),
                             return -1);
        }
    }

    /* shutdown listening thread */
    if (iscsi_sock_shutdown(g_sock, 2)!=0) {
        TRACE_WARN("iscsi_sock_shutdown() failed\n");
    }
    if (g_listener_pid != OST_GETPID) {
        while (g_listener_listening) ISCSI_SPIN;
    }
    if (iscsi_sock_close(g_sock)!=0) {
        TRACE_ERROR("iscsi_sock_close() failed\n");
        return -1;
    }

    /* destroy queue of free target command structures */
    for (i=0;i<CONFIG_TARGET_MAX_SESSIONS; i++) {
        for (j=0; j<CONFIG_TARGET_MAX_QUEUE; j++) {
            ISCSI_WQ_DESTROY(&g_cmd[i][j].ready_queue);
        }
        iscsi_queue_destroy(&g_cmd_q[i]);
    }
 
    /* destroy target commands */
    for (i=0; i<CONFIG_TARGET_MAX_SESSIONS; i++) {
        iscsi_free(g_cmd[i]);
    }

    /* destroy queue of free iSCSI headers */
    for (i=0;i<CONFIG_TARGET_MAX_SESSIONS; i++) {
        iscsi_queue_destroy(&g_header_q[i]);
        ISCSI_SPIN_LOCK_DESTROY_ELSE(&g_session[i].slock, return -1);
    }

    /* destroy queue of free sessions structures */
    iscsi_queue_destroy(&g_session_q);

    /* shutdown the device */
    if (device_shutdown()!=0) {
        TRACE_ERROR("device_shutdown() failed\n");
        return -1;
    }

    g_state = TARGET_SHUT_DOWN;

    return 0;
}

int target_listen(int port, int delay, int family) {
    TARGET_SESSION_T *sess;
    int one = 1;
    socklen_t localAddrLen;
    struct sockaddr_in localAddr;
    socklen_t remoteAddrLen;
    struct sockaddr_in remoteAddr;
    char local[16];
    char remote[16];
    ISCSI_THREAD_START("listen_thread");
    g_listener_pid = OST_GETPID;
    g_listener_listening++;

    // Create/Bind/Listen

    if (iscsi_sock_create(&g_sock, AF_INET)<0) {
        TRACE_ERROR("iscsi_sock_create() failed\n");
        goto done;
    }
    if (iscsi_sock_setsockopt(&g_sock, SOL_SOCKET, SO_REUSEADDR, &one, 
                              sizeof(one))!=0) {
        TRACE_ERROR("iscsi_sock_setsockopt() failed\n");
        goto done;
    }
#if 1
#ifdef __arm__
    if (iscsi_sock_setsockopt(&g_sock, 6, TCP_NODELAY, &one, sizeof(one))!=0) {
        TRACE_ERROR("iscsi_sock_setsockopt() failed\n");
        return -1;
    }
#else
    if (iscsi_sock_setsockopt(&g_sock, SOL_TCP, TCP_NODELAY, &one, 
                              sizeof(one))!=0) {
        TRACE_ERROR("iscsi_sock_setsockopt() failed\n");
        return -1;
    }
#endif
#endif
    if (iscsi_sock_bind(g_sock, port, family)<0) {
        TRACE_ERROR("iscsi_sock_bind() failed\n");
        goto done;
    }
    if (iscsi_sock_listen(g_sock)<0) {
        TRACE_ERROR("iscsi_sock_listen() failed\n");
        goto done;
    }

    // Loop for connections: FIX ME with queue

    while (g_state!=TARGET_SHUT_DOWN) {
        int tmp;

        if ((sess = iscsi_queue_remove(&g_session_q))==NULL) {
            TRACE_ERROR("no free sessions: iscsi_queue_remove() failed\n");
            goto done;
        }

        tmp = sess->id;
        memset(sess, 0, sizeof(TARGET_SESSION_T));
        sess->id = tmp;

        // Accept connection, spawn session thread, and
        // clean up old threads

        if (iscsi_sock_accept(g_sock, &sess->sock)<0) {
            TRACE(TRACE_DEBUG, 0, "iscsi_sock_accept() failed\n");
            goto done;
        }

        localAddrLen = sizeof(localAddr);
        memset((char *)&localAddr, 0, sizeof(localAddr));
        if (iscsi_sock_getsockname(sess->sock, (struct sockaddr *) 
                                   &localAddr, &localAddrLen)<0) {
            TRACE_ERROR("iscsi_sock_getsockname() failed\n");
            goto done;
        }
        remoteAddrLen = sizeof(remoteAddr);
        memset((char *)&remoteAddr, 0, sizeof(remoteAddr));
        if (iscsi_sock_getpeername(sess->sock, 
                                   (struct sockaddr *) &remoteAddr, 
                                   &remoteAddrLen)<0) {
            TRACE_ERROR("iscsi_sock_getsockname() failed\n");
            goto done;
        }

#if (BYTE_ORDER == BIG_ENDIAN)
        sprintf(local, "%u.%u.%u.%u",
                (localAddr.sin_addr.s_addr&0xff000000)>>24,
                (localAddr.sin_addr.s_addr&0x00ff0000)>>16,
                (localAddr.sin_addr.s_addr&0x0000ff00)>>8,
                (localAddr.sin_addr.s_addr&0x000000ff));
        sprintf(remote, "%u.%u.%u.%u",
                (remoteAddr.sin_addr.s_addr&0xff000000)>>24,
                (remoteAddr.sin_addr.s_addr&0x00ff0000)>>16,
                (remoteAddr.sin_addr.s_addr&0x0000ff00)>>8,
                (remoteAddr.sin_addr.s_addr&0x000000ff));
#else
        sprintf(local, "%u.%u.%u.%u",
                (localAddr.sin_addr.s_addr&0x000000ff),
                (localAddr.sin_addr.s_addr&0x0000ff00)>>8,
                (localAddr.sin_addr.s_addr&0x00ff0000)>>16,
                (localAddr.sin_addr.s_addr&0xff000000)>>24);
        sprintf(remote, "%u.%u.%u.%u",
                (remoteAddr.sin_addr.s_addr&0x000000ff),
                (remoteAddr.sin_addr.s_addr&0x0000ff00)>>8,
                (remoteAddr.sin_addr.s_addr&0x00ff0000)>>16,
                (remoteAddr.sin_addr.s_addr&0xff000000)>>24);
#endif

        sprintf(g_TargetAddress ,"%s:%u,1", local, port);
        TRACE_CLEAN(TRACE_ISCSI, 0, 
                    "Connection accepted:  port %i, local %s, remote %s\n", 
                    port, local, remote);
        TRACE(TRACE_ISCSI, 1, "TargetAddress = \"%s\"\n", g_TargetAddress);
        ISCSI_SLEEP(delay);
        START_WORKER_NO_JOIN_ELSE("session worker", (uint64_t) 0, sess->worker, 
                                  worker_proc_t, sess, goto done);      
    }
 done: g_listener_listening--;
    return 0;
}

/*********************
 * Private Functions *
 *********************/

static int reject_t(TARGET_SESSION_T *sess, unsigned char *header, 
                    unsigned char reason) {
    ISCSI_REJECT_T reject;
    unsigned char rsp_header[ISCSI_HEADER_LEN];
    TRACE_ERROR("reject %x\n", reason);
    reject.reason = reason;
    reject.length = ISCSI_HEADER_LEN;
    reject.StatSN = ++(sess->StatSN);
    reject.ExpCmdSN = sess->ExpCmdSN;
    reject.MaxCmdSN = sess->MaxCmdSN;
    reject.DataSN = 0;  // SNACK not yet implemented

    if (iscsi_reject_encap(rsp_header, &reject)!=0) {
        TRACE_ERROR("iscsi_reject_encap() failed\n");
        return -1;
    }
    ISCSI_SPIN_LOCK_ELSE(&sess->slock, return -1);
    if (iscsi_sock_send_header_and_data(sess->sock, rsp_header, 
                                        ISCSI_HEADER_LEN, header, 
                                        ISCSI_HEADER_LEN, 0)!=
        2*ISCSI_HEADER_LEN) {
        TRACE_ERROR("iscsi_sock_send_header_and_data() failed\n");
        ISCSI_SPIN_UNLOCK_ELSE(&sess->slock, return -1);
        return -1;
    }
    ISCSI_SPIN_UNLOCK_ELSE(&sess->slock, return -1);
    return 0;
}

int device_command_done(TARGET_CMD_T *cmd) {
    TARGET_SESSION_T *sess = cmd->sess;
    ISCSI_SCSI_CMD_T *scsi_cmd = &(cmd->scsi_cmd);
    ISCSI_READ_DATA_T data;
    unsigned char rsp_header[ISCSI_HEADER_LEN];
    unsigned DataSN = 0;
    ISCSI_SCSI_RSP_T scsi_rsp;
    struct iovec sg_new[CONFIG_TARGET_MAX_IOV_LEN];
    int rc = -1;
    struct timeval time;
    uint64_t diff;
    OST_DEVICE_COUNTERS_T *dstats;

    assert(cmd!=NULL); assert(scsi_cmd!=NULL);
    assert(scsi_cmd->send_data == NULL);
    assert(scsi_cmd->send_sg_len == 0);
    // assert(scsi_cmd->tag != 0); OPENISCSI

    scsi_cmd->send_data = (void *) cmd->gather_list;
    scsi_cmd->send_sg_len = cmd->gather_len;

    /* Send initiator any input data */
    scsi_cmd->bytes_fromdev = 0;
    if (!scsi_cmd->status&&scsi_cmd->fromdev) {
        struct iovec sg_singleton;
        struct iovec *sg, *sg_orig;
        int sg_len_orig, sg_len;
        unsigned offset, trans_len;
        int fragment_flag = 0;
        int offset_inc;
        if (scsi_cmd->todev) {
            TRACE(TRACE_ISCSI, 0, 
                  "sending %u bytes bi-directional input data\n", 
                  scsi_cmd->bidi_trans_len); 
            trans_len = scsi_cmd->bidi_trans_len;
        } else {
            trans_len = scsi_cmd->trans_len;
        }

        TRACE(TRACE_ISCSI, 1, "sending %i bytes input data as separate PDUs\n",
              trans_len);

        if (scsi_cmd->send_data == NULL) {
            TRACE_ERROR("send_data is NULL\n");
            return -1;
        }
        if (scsi_cmd->send_sg_len) {
            sg_orig = (struct iovec *) scsi_cmd->send_data;
            sg_len_orig = scsi_cmd->send_sg_len;
        } else {
            sg_len_orig = 1;
            sg_singleton.iov_base = scsi_cmd->send_data;
            sg_singleton.iov_len = trans_len;
            sg_orig = &sg_singleton;
        }
        sg = sg_orig; sg_len = sg_len_orig;
        
        if (sess->sess_params.max_data_seg_length) {
            offset_inc = sess->sess_params.max_data_seg_length;
        } else {
            offset_inc = trans_len;
        }

        for (offset=0; offset<trans_len; offset+=offset_inc) {
            memset(&data, 0, sizeof(ISCSI_READ_DATA_T));          
            if (sess->sess_params.max_data_seg_length) {
                data.length = MIN(trans_len-offset, 
                                  sess->sess_params.max_data_seg_length);
            } else {
                data.length = trans_len-offset;
            }
            if (data.length!=trans_len) {
                if (!fragment_flag) {
                    fragment_flag++;
                }
                sg = sg_new; sg_len = sg_len_orig;
                memcpy(sg, sg_orig, sizeof(struct iovec)*sg_len_orig);
                if (modify_iov(&sg, &sg_len, offset, data.length)!=0) {
                    TRACE_ERROR("modify_iov() failed\n");
                    goto error;
                }
            }
            TRACE(TRACE_ISCSI, 1, 
                  "sending read data PDU (offset %u, len %u)\n", 
                  offset, data.length);
            if (offset+data.length==trans_len) {
                data.final = 1;
                if (sess->UsePhaseCollapsedRead) {
                    data.status = 1;
                    data.status = scsi_cmd->status;
                    data.StatSN = ++(sess->StatSN);
                    TRACE(TRACE_ISCSI, 1, 
                          "status 0x%x collapsed into last data PDU\n", 
                          data.status);
                } else {
                    TRACE(TRACE_ISCSI, 1, 
                          "NOT collapsing status with last data PDU\n");
                }
            } else if (offset+data.length>trans_len) {
                TRACE_ERROR("offset+data.length > trans_len??\n");
                goto error;
            }
            data.task_tag = scsi_cmd->tag;
            data.ExpCmdSN = sess->ExpCmdSN;
            data.MaxCmdSN = sess->MaxCmdSN;
            data.DataSN = DataSN++;
            data.offset = offset;
            if (iscsi_read_data_encap(rsp_header, &data)!=0) {
                TRACE_ERROR("iscsi_read_data_encap() failed\n");
                goto error;
            }
            TRACE(TRACE_ISCSI, 1, 
                  "sending response header w/ %u bytes data (sg len %u)\n", 
                  data.length, sg_len);

            ISCSI_SPIN_LOCK_ELSE(&sess->slock, goto error);
            if (iscsi_sock_send_header_and_data(sess->sock, rsp_header, 
                                                ISCSI_HEADER_LEN, sg, 
                                                data.length, sg_len)
                !=ISCSI_HEADER_LEN+data.length) {
                TRACE_ERROR("iscsi_sock_send_header_and_data(%u bytes) failed\n"
                            , ISCSI_HEADER_LEN + data.length);
                goto error;
            }       
            ISCSI_SPIN_UNLOCK_ELSE(&sess->slock, goto error);

            scsi_cmd->bytes_fromdev += data.length;
            TRACE(TRACE_ISCSI, 1, "sent read data PDU ok (offset %u, len %u)\n"
                  , data.offset, data.length);
        }
        TRACE(TRACE_ISCSI, 1, "successfully sent %i bytes read data\n", 
              trans_len);
        
        /* signal the device that we're done with the buffers */

#if 0
        if (device_place_data(cmd, 0, 0) != 0) {
            TRACE_ERROR("device callback failed to ack end of transfer\n");
            exit(1);
        }
#endif

    } 

    /*
     * Send a response PDU if 
     *
     * 1) we're not using phase collapsed input (and status was good)
     * 2) we are using phase collapsed input, but there was no input 
     *    data (e.g., TEST UNIT READY) 
     * 3) command had non-zero status and possible sense data
     */

    if (!sess->UsePhaseCollapsedRead||!scsi_cmd->length||scsi_cmd->status) {
        TRACE(TRACE_ISCSI, 1, "sending SCSI response PDU\n");
        memset(&scsi_rsp, 0, sizeof(ISCSI_SCSI_RSP_T));
        scsi_rsp.length = scsi_cmd->status?scsi_cmd->length:0;
        scsi_rsp.tag = scsi_cmd->tag;
        scsi_rsp.StatSN = sess->StatSN++;
        scsi_rsp.ExpCmdSN = sess->ExpCmdSN;
        scsi_rsp.MaxCmdSN = sess->MaxCmdSN;
        scsi_rsp.ExpDataSN = (!scsi_cmd->status&&scsi_cmd->fromdev)?DataSN:0;
        scsi_rsp.response = 0x00;      // iSCSI response
        scsi_rsp.status = scsi_cmd->status;  // SCSI status
        if (iscsi_scsi_rsp_encap(rsp_header, &scsi_rsp)!=0) {
            TRACE_ERROR("iscsi_scsi_rsp_encap() failed\n");
            goto error;
        }

        ISCSI_SPIN_LOCK_ELSE(&sess->slock, goto error);
        if (iscsi_sock_send_header_and_data(sess->sock, rsp_header, 
                                            ISCSI_HEADER_LEN, 
                                            scsi_cmd->send_data, 
                                            scsi_rsp.length, 
                                            scsi_cmd->send_sg_len) != 
            ISCSI_HEADER_LEN+scsi_rsp.length) {
            TRACE_ERROR("iscsi_sock_send_header_and_data() failed\n");
            goto error;
        }
        ISCSI_SPIN_UNLOCK_ELSE(&sess->slock, goto error);

        /* Make sure all data was transferred */
        if ((scsi_cmd->todev)&&(scsi_cmd->status==0)) {
#if 0
            g_stats.rx_bytes += scsi_cmd->trans_len;    
#endif
            if (scsi_cmd->bytes_todev != scsi_cmd->trans_len) {
                TRACE_ERROR("op 0x%x: expected %u bytes recv, got %u\n", 
                            scsi_cmd->tag, scsi_cmd->trans_len, 
                            scsi_cmd->bytes_todev);
                return -1;
            }
            RETURN_NOT_EQUAL("scsi_cmd->bytes_todev", scsi_cmd->bytes_todev, 
                             scsi_cmd->trans_len, NO_CLEANUP,  -1);
            if (scsi_cmd->fromdev) {
#if 0
                g_stats.tx_bytes += scsi_cmd->bidi_trans_len;   
#endif
                RETURN_NOT_EQUAL("scsi_cmd->bytes_fromdev", 
                                 scsi_cmd->bytes_fromdev, 
                                 scsi_cmd->bidi_trans_len, NO_CLEANUP, -1);
            }
        } else {
            if ((scsi_cmd->fromdev)&&(scsi_cmd->status==0)) {
                RETURN_NOT_EQUAL("scsi_cmd->bytes_fromdev", 
                                 scsi_cmd->bytes_fromdev, scsi_cmd->trans_len, 
                                 NO_CLEANUP, -1);
#if 0
                g_stats.tx_bytes += scsi_cmd->trans_len;
#endif
            }
        }
    } 

    TRACE(TRACE_ISCSI, 1, 
          "session %i: completed iSCSI op 0x%x (tag 0x%x header %p cmd %p)\n", 
          cmd->sess->id, ISCSI_OPCODE(cmd->header), ISCSI_ITT(cmd->header), 
          cmd->header, cmd); 

    /* service time (latency) */
    gettimeofday(&time, NULL);
    cmd->complete = time.tv_sec*1e6+time.tv_usec;
    diff = time.tv_sec*1e6+time.tv_usec - cmd->issue;

    /* update device counters (we have the lun lock???) */
    dstats = device_counters_get(scsi_cmd->lun);
    /* The last argument specifies whether we already have the counters locked. We use
       this lock to keep StatSN in order. Probably makes sense to use a different lock
       instead of overloading the counters lock */
    if (ost_update_counters_post_via_cdb(dstats, scsi_cmd->cdb, diff, 1)!=0) {
        TRACE_ERROR("ost_update_counters_post_via_cdb() failed\n");
        goto error;
    }

    /* remove from outstanding list */
    HASH_REMOVE_ELSE(&cmd->sess->cmds, scsi_cmd->tag, exit(1));

    /* free up iSCSI header */
    assert(cmd->header!=NULL);
    if (iscsi_queue_insert(&g_header_q[sess->id], cmd->header)!=0) {
        TRACE_ERROR("iscsi_queue_insert() failed\n");
        goto error;
    }

    /* free up target command structure */
    if (iscsi_queue_insert(&g_cmd_q[sess->id], cmd)!=0) {
        TRACE_ERROR("iscsi_queue_insert() failed\n");
        goto error;
    }

    rc = 0;

 error:
    return rc;
}

static int scsi_command_t(TARGET_SESSION_T *sess, unsigned char *header) {
    TARGET_CMD_T *cmd;
    ISCSI_SCSI_CMD_T *scsi_cmd = NULL;
    int rc;
    OST_DEVICE_COUNTERS_T *dstats;

    /* get and initialize a target command structure */
    if ((cmd=iscsi_queue_remove(&g_cmd_q[sess->id]))==NULL) {
        TRACE_ERROR("No free target commands\n");
        return -1;
    }
    memset(cmd, 0, sizeof(TARGET_CMD_T));
    cmd->sess = sess; 
    cmd->header = header; /* so we can re-queue it when we're done */
    cmd->callback = cmd->callback_arg = NULL;
    scsi_cmd = &cmd->scsi_cmd;
    memset(scsi_cmd, 0, sizeof(ISCSI_SCSI_CMD_T));
    if (iscsi_scsi_cmd_decap(header, scsi_cmd)!=0) {
        TRACE_ERROR("iscsi_scsi_cmd_decap() failed\n");
        goto error;
    }
    scsi_cmd->tag = ISCSI_ITT(cmd->header);

    TRACE(TRACE_ISCSI, 1, "dequeued 0x%x (lun %"PRIu64", lun %u)\n", 
          scsi_cmd->tag, scsi_cmd->lun, ISCSI_LUN(header));

    /* For Non-immediate commands, the CmdSN should be between ExpCmdSN 
     * and MaxCmdSN, inclusive of both */

    if( (!scsi_cmd->immediate) && ((scsi_cmd->CmdSN < sess->ExpCmdSN) || 
                                   (scsi_cmd->CmdSN > sess->MaxCmdSN)) ) {
        static int flag = 0;

        if (!flag) {
            TRACE_ERROR("CmdSN(%d) of SCSI Command not valid, ExpCmdSN(%d) "
                        "MaxCmdSN(%d)\n", scsi_cmd->CmdSN, sess->ExpCmdSN, 
                        sess->MaxCmdSN);
            //goto error;
            flag = 1;
            TRACE_ERROR("SUPPRESSING FURTHER ERRORS\n");
        }
    }

    /* Arg check */
 
    if (scsi_cmd->CmdSN != sess->ExpCmdSN) {
        TRACE_WARN("Expected CmdSN %i, got %i. (ignoring and resetting)\n", 
            sess->ExpCmdSN, scsi_cmd->CmdSN);
        sess->ExpCmdSN = scsi_cmd->CmdSN;
    }
    if (sess->sess_params.first_burst_length
        && (scsi_cmd->length > sess->sess_params.first_burst_length)) {
        TRACE_ERROR("scsi_cmd->length (%u) > FirstBurstLength (%u)\n",
                    scsi_cmd->length, sess->sess_params.first_burst_length);
        goto error;
    }
    if (sess->sess_params.max_data_seg_length
        && (scsi_cmd->length > sess->sess_params.max_data_seg_length)) {
        TRACE_ERROR("scsi_cmd.length (%u) > MaxRecvDataSegmentLength (%u)\n",
                    scsi_cmd->length, sess->sess_params.max_data_seg_length);
        goto error;
    }

    // Read AHS.  Need to optimize/clean this.  
    // We should not be calling malloc().
    // We need to check for properly formated AHS segments.
  
    if (scsi_cmd->ahs_len) {
        unsigned ahs_len;
        unsigned char *ahs_ptr;
        unsigned char ahs_type;

        if (scsi_cmd->ahs_len > CONFIG_ISCSI_MAX_AHS_LEN) {
            TRACE_ERROR("scsi_cmd->ahs_len (%u) > "
                        "CONFIG_ISCSI_MAX_AHS_LEN (%u)\n",
                        scsi_cmd->ahs_len, CONFIG_ISCSI_MAX_AHS_LEN);
        }
        TRACE(TRACE_ISCSI, 1, "reading %i bytes AHS\n", scsi_cmd->ahs_len);
        if (iscsi_sock_msg(sess->sock, 0, scsi_cmd->ahs_len, scsi_cmd->ahs, 0)
            !=scsi_cmd->ahs_len) {
            TRACE_ERROR("iscsi_sock_msg() failed\n");
            goto error;
        }
        TRACE(TRACE_ISCSI, 2, "read %i bytes AHS ok\n", scsi_cmd->ahs_len);
        ahs_ptr = scsi_cmd->ahs;
        while (ahs_ptr<(scsi_cmd->ahs+scsi_cmd->ahs_len-1)) {
            ahs_len = NTOHS(*((unsigned short *)ahs_ptr));
            ahs_type = *(ahs_ptr+2); 
            switch(ahs_type) {
            case 0x01:
                TRACE(TRACE_ISCSI, 0, 
                      "Got ExtendedCDB AHS (%u bytes extra CDB)\n", ahs_len);
                scsi_cmd->ext_cdb = ahs_ptr+4;
                break;
            case 0x02:
                scsi_cmd->bidi_trans_len = NTOHL(*((unsigned *)(ahs_ptr+4)));
                TRACE(TRACE_ISCSI, 0, 
                      "Got Bidirectional Read AHS (expected read length %u)\n",
                      scsi_cmd->bidi_trans_len);
                break;
            default:
                TRACE_ERROR("unknown AHS type %x\n", ahs_type); 
                goto error;
            }
            ahs_ptr += ahs_len;
        }
        TRACE(TRACE_DEBUG, 0, "done parsing %i bytes AHS\n", scsi_cmd->ahs_len);
    } 

    sess->ExpCmdSN++;
    sess->MaxCmdSN++; 

    /*
     * Execute cdb.  device_command() will set scsi_cmd.input if there is input     
     * data and set the length of the input to either scsi_cmd.trans_len or
     * scsi_cmd.bidi_trans_len, depending on whether scsi_cmd.output was set.
     *
     * When the device has finished executing the CDB (i.e., status has been
     * set), it will call the device_command_done() callback, instructing
     * this target code to send the response PDU.
     */

    /* update device counters */
    dstats = device_counters_get(scsi_cmd->lun);

    /* This assumes that we're not doing a bi-directional transfer */
    if (ost_update_counters_pre_via_cdb(dstats, scsi_cmd->cdb)!=0) {
        TRACE_ERROR("ost_update_counters_pre_via_cdb() failed\n");
    }

    /* setup callback */
    scsi_cmd->fromdev = 0;
    cmd->callback = (void *) device_command_done;
    cmd->callback_arg = cmd;

    /* clear out any immediate data */
    if ((!sess->sess_params.immediate_data) && scsi_cmd->length) {
        TRACE_ERROR("Cannot accept any Immediate data\n");
        return -1;
    }
    if (sess->sess_params.immediate_data && scsi_cmd->length) {
        TRACE(TRACE_ISCSI, 0, 
              "we have %u bytes of immediate write data (ptr for is %p)\n", 
              scsi_cmd->length, cmd->immediate);        
        if (sess->sess_params.max_data_seg_length) {
            RETURN_GREATER("args->length", 
                           ISCSI_LEN(cmd->header), 
                           sess->sess_params.max_data_seg_length, 
                           NO_CLEANUP, -1);
        }
        if ((rc=iscsi_sock_msg(sess->sock, 0, scsi_cmd->length, 
                               cmd->immediate, 0)) != scsi_cmd->length) {
            TRACE_ERROR("iscsi_sock_msg() failed (rc %i)\n", rc);
            exit(1);   
        }
        TRACE(TRACE_ISCSI, 0, "cleared %i bytes of immediate write data\n", 
              scsi_cmd->length);        
    } else {
        TRACE(TRACE_ISCSI, 0, "no immediate write data\n");        
    }

    /* add to list of outstanding commands */
    if (hash_get(&cmd->sess->cmds, scsi_cmd->tag)) {
        TRACE_WARN("not rehashing cmd 0x%x\n", scsi_cmd->tag);
    } else {
        HASH_INSERT_ELSE(&cmd->sess->cmds, cmd, scsi_cmd->tag, return -1);
    }

    /* we should really protect this with a lock ! */
    {
        static uint64_t last, diff;
        struct timeval time;
        static int flag = 0;

        gettimeofday(&time, NULL);
        cmd->issue = time.tv_sec*1e6+time.tv_usec;
        if (flag) {     
            diff = cmd->issue - last;
        } else {
            diff = 0;
            flag = 1;
        }
        last = cmd->issue;
    }

    /* queue to the device server */
    if (device_enqueue(cmd)!=0)  {
        TRACE_ERROR("device_enqueue() failed\n");
        goto error;
    }
    TRACE(TRACE_ISCSI, 1, "queued cmd 0x%x to device\n", scsi_cmd->tag);

    return 0;

 error: 

    scsi_cmd->status = 0x02; scsi_cmd->length = 0;
    return device_command_done(cmd);
}

static int task_command_t(TARGET_SESSION_T *sess, unsigned char *header) {
    ISCSI_TASK_CMD_T cmd;
    ISCSI_TASK_RSP_T rsp;
    unsigned char rsp_header[ISCSI_HEADER_LEN];

    // Get & check args

    if (iscsi_task_cmd_decap(header, &cmd) !=0) {
        TRACE_ERROR("iscsi_task_cmd_decap() failed\n");
        return -1;
    }
    if (cmd.CmdSN != sess->ExpCmdSN) {
        TRACE_WARN("Expected CmdSN %i, got %i. (ignoring and resetting)\n", 
            cmd.CmdSN, sess->ExpCmdSN);
        sess->ExpCmdSN = cmd.CmdSN;
    }

    sess->MaxCmdSN++; 

    memset(&rsp, 0, sizeof(ISCSI_TASK_RSP_T));
    rsp.response = ISCSI_TASK_RSP_FUNCTION_COMPLETE;

    switch(cmd.function) {
    case (ISCSI_TASK_CMD_ABORT_TASK):
        PRINT("ISCSI_TASK_CMD_ABORT_TASK\n");
        break;
    case (ISCSI_TASK_CMD_ABORT_TASK_SET):
        PRINT("ISCSI_TASK_CMD_ABORT_TASK_SET\n");
        break;
    case (ISCSI_TASK_CMD_CLEAR_ACA):
        PRINT("ISCSI_TASK_CMD_CLEAR_ACA\n");
        break;
    case (ISCSI_TASK_CMD_CLEAR_TASK_SET):
        PRINT("ISCSI_TASK_CMD_CLEAR_TASK_SET\n");
        break;
    case (ISCSI_TASK_CMD_LOGICAL_UNIT_RESET):
        PRINT("ISCSI_TASK_CMD_LOGICAL_UNIT_RESET\n");
        break;
    case (ISCSI_TASK_CMD_TARGET_WARM_RESET):
        PRINT("ISCSI_TASK_CMD_TARGET_WARM_RESET\n");
        break;
    case (ISCSI_TASK_CMD_TARGET_COLD_RESET):
        PRINT("ISCSI_TASK_CMD_TARGET_COLD_RESET\n");
        break;
    case (ISCSI_TASK_CMD_TARGET_REASSIGN):
        PRINT("ISCSI_TASK_CMD_TARGET_REASSIGN\n");
        break;
    default:
        TRACE_ERROR("Unknown task function %i\n", cmd.function);
        rsp.response = ISCSI_TASK_RSP_REJECTED;
    }

    rsp.tag = cmd.tag;
    rsp.StatSN = ++(sess->StatSN);
    rsp.ExpCmdSN = sess->ExpCmdSN;
    rsp.MaxCmdSN = sess->MaxCmdSN;

    if (iscsi_task_rsp_encap(rsp_header, &rsp) !=0) {
        TRACE_ERROR("iscsi_task_cmd_decap() failed\n");
        return -1;
    }
    if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, rsp_header, 0) != 
        ISCSI_HEADER_LEN) {
        TRACE_ERROR("iscsi_sock_msg() failed\n");
        return -1;

    }
    return 0;
}

static int nop_out_t(TARGET_SESSION_T *sess, unsigned char *header) {
    ISCSI_NOP_OUT_T nop_out;
    char *ping_data = NULL;

    if (iscsi_nop_out_decap(header, &nop_out) !=0) {
        TRACE_ERROR("iscsi_nop_out_decap() failed\n");
        return -1;
    }

    if (nop_out.CmdSN != sess->ExpCmdSN) {
        static int flag = 0;

        if (!flag) {
            TRACE_ERROR("Got CmdSN %i, expected %i. (ignoring and resetting)\n", 
                nop_out.CmdSN, sess->ExpCmdSN);
            sess->ExpCmdSN = nop_out.CmdSN;
            flag = 1;
            TRACE_ERROR("SUPPRESSING FURTHER ERRORS\n");
        }
    }

    if (nop_out.length) {
        if ((ping_data=iscsi_malloc(nop_out.length))==NULL) {
            TRACE_ERROR("iscsi_malloc() failed\n");
            return -1;
        } 
        if (iscsi_sock_msg(sess->sock, 0, nop_out.length, ping_data, 0) != 
            nop_out.length) {
            TRACE_ERROR("iscsi_sock_msg() failed\n");
            if (ping_data) iscsi_free(ping_data);
            return -1;
        }
        TRACE(TRACE_ISCSI, 2, "received %i bytes ping paylosd\n", 
              nop_out.length);
    } else {
        TRACE(TRACE_ISCSI, 2, "ping has no payload\n");
    }
    if (nop_out.tag != 0xffffffff) {
        ISCSI_NOP_IN_T nop_in;
        unsigned char rsp_header[ISCSI_HEADER_LEN];

        /* build response to initiator */
        memset(&nop_in, 0, sizeof(ISCSI_NOP_IN_T));
        nop_in.length = nop_out.length;
        nop_in.lun = nop_out.lun;
        nop_in.tag = nop_out.tag;
        nop_in.transfer_tag = 0xffffffff;
        nop_in.StatSN = sess->StatSN++;

        nop_in.ExpCmdSN = ++sess->ExpCmdSN;
        nop_in.MaxCmdSN = ++sess->MaxCmdSN;

        if (iscsi_nop_in_encap(rsp_header, &nop_in)!=0) {
            TRACE_ERROR("iscsi_nop_in_encap() failed\n");
            if (ping_data) iscsi_free(ping_data);
            return -1;
        }
        ISCSI_SPIN_LOCK_ELSE(&sess->slock, return -1);
        if (iscsi_sock_send_header_and_data(sess->sock, rsp_header, 
                                            ISCSI_HEADER_LEN, ping_data, 
                                            nop_in.length, 0) != 
            ISCSI_HEADER_LEN + nop_in.length) {
            TRACE_ERROR("iscsi_sock_send_header_and_data() failed\n");
            if (ping_data) iscsi_free(ping_data);
            ISCSI_SPIN_UNLOCK_ELSE(&sess->slock, return -1);
            return -1;
        }
        ISCSI_SPIN_UNLOCK_ELSE(&sess->slock, return -1);
        TRACE(TRACE_ISCSI, 2, "sent %i bytes ping response\n", nop_out.length);
    } else {
        TRACE(TRACE_ISCSI, 2, "not sending a ping response\n");
    }

    if (ping_data) iscsi_free(ping_data);
    return 0; 
}

/*
 * text_command_t
 */

#define TC_CLEANUP { if (text_in != NULL) iscsi_free_atomic(text_in);\
        if (text_out != NULL) iscsi_free_atomic(text_out);}
#define TC_ERROR {TC_CLEANUP; return -1;}

static int text_command_t(TARGET_SESSION_T *sess, unsigned char *header) {
    ISCSI_TEXT_CMD_T text_cmd;
    ISCSI_TEXT_RSP_T text_rsp;
    unsigned char rsp_header[ISCSI_HEADER_LEN];
    char *text_in = NULL;
    char *text_out = NULL; 
    int len_in;
    int len_out = 0;

    // Get text args
    if (iscsi_text_cmd_decap(header, &text_cmd) !=0) {
        TRACE_ERROR("iscsi_text_cmd_decap() failed\n");
        return -1;
    }

    // Check args & update numbering
    RETURN_NOT_EQUAL("Continue", text_cmd.cont, 0, NO_CLEANUP, -1);
    RETURN_NOT_EQUAL("CmdSN", text_cmd.CmdSN, sess->ExpCmdSN, NO_CLEANUP, -1);

    sess->ExpCmdSN++;
    sess->MaxCmdSN++; 

    if ((text_out=iscsi_malloc_atomic(ISCSI_PARAM_MAX_TEXT_LEN))==NULL) {
        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        return -1;
    }
    // Read text parameters
  
    len_in = text_cmd.length;
    if (len_in) {
        ISCSI_PARAMETER_T *ptr;

        if ((text_in=iscsi_malloc_atomic(len_in+1))==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
            TC_CLEANUP;
            return -1;
        }
        TRACE(TRACE_DEBUG, 0, "reading %i bytes text parameters\n", len_in);
        if (iscsi_sock_msg(sess->sock, 0, len_in, text_in, 0)!=len_in) {
            TRACE_ERROR("iscsi_sock_msg() failed\n");
            TC_CLEANUP;
            return -1;
        }
        text_in[len_in] = '\0';

        PARAM_TEXT_PARSE_ELSE(sess->params, text_in, len_in, text_out, 
                              &len_out, 0, &g_cookie[sess->id], 
                              param_val(sess->params, "CHAP_N"), 
                              CONFIG_TARGET_PASS_IN, 
                              CONFIG_TARGET_USER_OUT, CONFIG_TARGET_PASS_OUT,
                              TC_ERROR);

        // Exceptional cases not covered by parameters.c (e.g., SendTargets)

        if ((ptr=param_get(sess->params, "SendTargets"))==NULL) {
            TRACE_ERROR("param_get() failed\n");
            TC_CLEANUP;
            return -1;
        }
        if (ptr->rx_offer) {
            if (ptr->offer_rx && !strcmp(ptr->offer_rx, "All") && 
                (param_equiv(sess->params, "SessionType", "Discovery"))) {
                TRACE(TRACE_DEBUG, 0, 
                      "Rejecting SendTargets=All in a non Discovery session\n");
                PARAM_TEXT_ADD_ELSE(sess->params, "SendTargets", "Reject", 
                                    text_out, &len_out, 0, TC_ERROR);
            } else {
                PARAM_TEXT_ADD_ELSE(sess->params, "TargetName",    
                                    g_hostname, text_out, &len_out, 0, 
                                    TC_ERROR);
                PARAM_TEXT_ADD_ELSE(sess->params, "TargetAddress", 
                                    g_TargetAddress,  text_out, &len_out, 0, 
                                    TC_ERROR);
            }
            ptr->rx_offer = 0;
        }

        // Parse outgoing offer

        if (len_out) {
            PARAM_TEXT_PARSE_ELSE(sess->params, text_out, len_out, NULL, NULL, 
                                  1, &g_cookie[sess->id], 
                                  param_val(sess->params, "CHAP_N"), 
                                  CONFIG_TARGET_PASS_IN, 
                                  CONFIG_TARGET_USER_OUT, 
                                  CONFIG_TARGET_PASS_OUT, TC_ERROR);
        }
    }

    if (sess->IsFullFeature) {
        set_session_parameters( sess->params, &sess->sess_params );
    }

    // Send response

    text_rsp.final = text_cmd.final;
    text_rsp.cont = 0;
    text_rsp.length = len_out;
    text_rsp.lun = text_cmd.lun;
    text_rsp.tag = text_cmd.tag;
    if (text_rsp.final) 
        text_rsp.transfer_tag = 0xffffffff;
    else
        text_rsp.transfer_tag = 0x1234;
    text_rsp.StatSN = sess->StatSN++;
    text_rsp.ExpCmdSN = sess->ExpCmdSN;
    text_rsp.MaxCmdSN = sess->MaxCmdSN;
    if (iscsi_text_rsp_encap(rsp_header, &text_rsp)!=0) {
        TRACE_ERROR("iscsi_text_rsp_encap() failed\n");
        TC_CLEANUP;
        return -1;
    }
    if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, rsp_header, 0) != 
        ISCSI_HEADER_LEN) {
        TRACE_ERROR("iscsi_sock_msg() failed\n");
        TC_CLEANUP;
        return -1;
    }
    if (len_out) {
        if (iscsi_sock_msg(sess->sock, 1, len_out, text_out, 0) != len_out) {

            TRACE_ERROR("iscsi_sock_msg() failed\n");
            TC_CLEANUP;
            return -1;
        }
    } 

    TC_CLEANUP;
    return 0;
}

/*
 * login_command_t() handles login requests and replies.
 */

#define LC_CLEANUP { if (text_in != NULL) iscsi_free_atomic(text_in); \
        if (text_out != NULL) iscsi_free_atomic(text_out);}
#define LC_ERROR {TC_CLEANUP; return -1;}

static int login_command_t(TARGET_SESSION_T *sess, unsigned char *header) {
    ISCSI_LOGIN_CMD_T cmd;
    ISCSI_LOGIN_RSP_T rsp;
    unsigned char rsp_header[ISCSI_HEADER_LEN];
    char *text_in = NULL;
    char *text_out = NULL; 
    int len_in = 0;
    int len_out = 0;
    int status = 0;
    //void *cookie;

    /* Initialize response */
    memset(&rsp, 0, sizeof(ISCSI_LOGIN_RSP_T));
    rsp.status_class = ISCSI_LOGIN_STATUS_INITIATOR_ERROR;

    /* Get login args & check preconditions */
    if (iscsi_login_cmd_decap(header, &cmd) !=0) {
        TRACE_ERROR("iscsi_login_cmd_decap() failed\n");
        goto response;
    }   

#if 0
    if (sess->IsLoggedIn) {
        TRACE_ERROR("duplicate login attempt on sess %i\n", sess->id);
        goto response;
    }
#endif 

    if ((cmd.cont != 0) && (cmd.transit != 0)) {
        TRACE_ERROR("Bad cmd.continue.  Expected 0.\n");
        goto response;
    } else  if ((cmd.version_max < ISCSI_VERSION) || 
                (cmd.version_min > ISCSI_VERSION)) { 
        TRACE_ERROR("Target iscsi version (%u) not supported by initiator "
                    "[Max Ver (%u) and Min Ver (%u)]\n", 
                    ISCSI_VERSION, cmd.version_max, cmd.version_min);
        rsp.status_class = ISCSI_LOGIN_STATUS_INITIATOR_ERROR;
        rsp.status_detail = 0x05; // Version not supported
        rsp.version_max = ISCSI_VERSION;
        rsp.version_active = ISCSI_VERSION;
        goto response;
    } else if (cmd.tsih != 0) {
        TRACE_ERROR("Bad cmd.tsih (%u). Expected 0.\n", cmd.tsih);
        goto response;
    }

    /* Parse text parameters and build response */
    if ((text_out=iscsi_malloc_atomic(ISCSI_PARAM_MAX_TEXT_LEN))==NULL) {
        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        return -1;
    }
    if ((len_in=cmd.length)) {
        if ((text_in=iscsi_malloc_atomic(len_in+1))==NULL) {
            TRACE_ERROR("iscsi_malloc() failed\n");
            LC_CLEANUP;
            return -1;
        }
        if (iscsi_sock_msg(sess->sock, 0, len_in, text_in, 0)!=len_in) {
            TRACE_ERROR("iscsi_sock_msg() failed\n");
            LC_CLEANUP;
            return -1;
        }
        text_in[len_in] = '\0';

        /* Parse incoming parameters (text_out will contain the response
           we need to send back to the initiator */

        if ((status = param_text_parse(sess->params, text_in, len_in, 
                                       text_out, &len_out, 0, 
                                       &g_cookie[sess->id],
                                       CONFIG_TARGET_USER_IN, 
                                       CONFIG_TARGET_PASS_IN,
                                       CONFIG_TARGET_USER_OUT, 
                                       CONFIG_TARGET_PASS_OUT)) != 0) {
            switch (status) {
            case ISCSI_PARAM_STATUS_FAILED:
                TRACE_ERROR("*** ISCSI_PARAM_STATUS_FAILED ***\n");
                rsp.status_detail = 0;
                break;
            case ISCSI_PARAM_STATUS_AUTH_FAILED:
                TRACE_ERROR("*** ISCSI_PARAM_STATUS_AUTH_FAILED ***\n");
                rsp.status_detail = 1; //Initiator Authentication faiture
                break;
            default:
                /*
                 * Need to set the detail field (status_detail field)
                 */
                PRINT("status = %i\n", status);
                break;
            }
            goto response;
        }

        /* Parse the outgoing offer */
        if(!sess->LoginStarted) {
            PARAM_TEXT_ADD_ELSE(sess->params, "TargetPortalGroupTag", "1",  
                                text_out, &len_out, 0, LC_ERROR);
        }
        if (len_out) {
            PARAM_TEXT_PARSE_ELSE(sess->params, text_out, len_out, NULL, NULL, 
                                  1, &g_cookie[sess->id], 
                                  param_val(sess->params, "CHAP_N"),
                                  CONFIG_TARGET_PASS_IN,
                                  CONFIG_TARGET_USER_OUT,
                                  CONFIG_TARGET_PASS_OUT,
                                  LC_ERROR;);
        }
    }

    if(!sess->LoginStarted) {
        sess->LoginStarted = 1;
    }

    /* For now, we accept whatever the initiator's current and next states 
       are, and we are always ready to transition to that state. */

    rsp.csg = cmd.csg;
    rsp.nsg = cmd.nsg;
    rsp.transit = cmd.transit;

    if (cmd.csg == ISCSI_LOGIN_STAGE_SECURITY) {
        if ((0==param_equiv(sess->params, "AuthResult", "No")))
            rsp.transit = 0; 
        else if ((0==param_equiv(sess->params, "AuthResult", "Fail"))) {
            rsp.status_class = rsp.status_detail = 1;
            goto response;
        } else if ((0==param_equiv(sess->params, "AuthResult", "Yes"))) {
            sess->Authenticated = 1;
        }
    }

    if (cmd.transit&&(cmd.nsg==ISCSI_LOGIN_STAGE_FULL_FEATURE)) {

        // Check post conditions

        if (!param_equiv(sess->params, "InitiatorName", "")) {
            TRACE_ERROR("InitiatorName not specified\n");
            goto response;
        }
        if (!param_equiv(sess->params, "SessionType", "Normal")) {
            if (!param_equiv(sess->params, "TargetName", "")) {
                TRACE_ERROR("TargetName not specified\n");
                goto response;
            } else if (param_equiv(sess->params, "TargetName", g_hostname)) {
                TRACE_ERROR("Bad TargetName \"%s\" (expected \"%s\")\n", 
                            param_val(sess->params, "TargetName"), g_hostname);
                goto response;
            }
        }
        if (!param_equiv(sess->params, "SessionType", "")) {
            TRACE_ERROR("SessionType not specified\n");
            goto response;
        }
        sess->ExpCmdSN = sess->MaxCmdSN = cmd.CmdSN;
        sess->cid = cmd.cid;
        sess->isid = cmd.isid;
        sess->tsih = sess->id+1;
        sess->IsFullFeature = 1;

        sess->IsLoggedIn = 1;
        if (!param_equiv(sess->params, "SessionType", "Discovery")) {
            strcpy(param_val(sess->params, "MaxConnections"),  "1");
        }
        set_session_parameters( sess->params, &sess->sess_params );
    } 

    // No errors

    rsp.status_class = rsp.status_detail = 0;
    rsp.length = len_out;

    // Send login response

 response: 
    sess->ExpCmdSN = sess->MaxCmdSN = cmd.CmdSN;
    rsp.isid = cmd.isid;
    rsp.StatSN = cmd.ExpStatSN; // debug 
    rsp.tag = cmd.tag;
    rsp.cont = cmd.cont;
    rsp.ExpCmdSN = sess->ExpCmdSN;
    rsp.MaxCmdSN = sess->MaxCmdSN; 
    if (!rsp.status_class) {
        if (rsp.transit&&(rsp.nsg==ISCSI_LOGIN_STAGE_FULL_FEATURE)) {
            rsp.version_max = ISCSI_VERSION;
            rsp.version_active = ISCSI_VERSION;
            rsp.StatSN = ++(sess->StatSN);
            rsp.tsih = sess->tsih;
        }
    }
    if (iscsi_login_rsp_encap(rsp_header, &rsp)!=0) {
        TRACE_ERROR("iscsi_login_rsp_encap() failed\n");
        LC_CLEANUP;
        return -1;
    }

    ISCSI_SPIN_LOCK_ELSE(&sess->slock, return -1);
    if (iscsi_sock_send_header_and_data(sess->sock, rsp_header, 
                                        ISCSI_HEADER_LEN, 
                                        text_out, rsp.length, 0) != 
        ISCSI_HEADER_LEN+rsp.length) {
        TRACE_ERROR("iscsi_sock_send_header_and_data() failed\n");
        LC_CLEANUP;
        ISCSI_SPIN_UNLOCK_ELSE(&sess->slock, return -1);
        return -1; 
    } 
    ISCSI_SPIN_UNLOCK_ELSE(&sess->slock, return -1);

    if (rsp.status_class!=0) {
        TRACE_ERROR("bad status class (0x%x)\n", rsp.status_class);
        LC_CLEANUP;
        return -1;
    } 

    if (cmd.transit&&(cmd.nsg==ISCSI_LOGIN_STAGE_FULL_FEATURE)) {
        if (!strcmp(param_val(sess->params, "AuthMethod"), "CHAP")) {
            if (!sess->Authenticated) {
                TRACE_ERROR("INITIATOR FAILED TO AUTHENTICATE (IGNORING)\n");
            }
        }

        if (1) {
            char TargetName[256];
            char InitiatorName[256];
            
            /* just to keep the output tidy */
            strncpy(InitiatorName, 
                    param_val(sess->params, "InitiatorName"), 50);
            InitiatorName[50] = '\0';
            strncpy(TargetName, g_hostname, 50);
            TargetName[50] = '\0';
            
	    if (!g_silent_login) {
		PRINT("*****************************************************"
		      "***************************\n");
		PRINT("*                                LOGIN SUCCESSFUL    "
		      "                          *\n");
		PRINT("*                                                    "
		      "                          *\n");
		PRINT("* %25s:%50s *\n", "InitiatorName", InitiatorName);
		if (strlen(param_val(sess->params, "InitiatorName"))>50) {
		    PRINT("* %25s:%50s *\n", "InitiatorName (cont)", 
			  param_val(sess->params, "InitiatorName")+50);
		}
		PRINT("* %25s:%50s *\n", "TargetName", TargetName);
		if (strlen(g_hostname)>50) {
		    PRINT("* %25s:%50s *\n", "TargetName (cont)", 
			  param_val(sess->params, "TargetName")+50);
		}
		PRINT("* %25s:%50s *\n", "Type", 
		      param_equiv(sess->params, "SessionType", 
				  "Discovery")?"Normal":"Discovery");
		PRINT("* %25s:%50s *\n", "AuthMethod", 
		      param_val(sess->params, "AuthMethod"));  
		if (param_equiv(sess->params, "AuthMethod", "CHAP")==0) {
		    PRINT("* %25s:%50s *\n", "InitiatorUser", 
			  CONFIG_TARGET_USER_IN);
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
		PRINT("*****************************************************"
		      "***************************\n");
	    }
        }
        sess->StatSN++;
    }
    
    LC_CLEANUP;
    return 0;
}

static int logout_command_t(TARGET_SESSION_T *sess, unsigned char *header) {
    ISCSI_LOGOUT_CMD_T cmd;
    ISCSI_LOGOUT_RSP_T rsp;
    unsigned char rsp_header[ISCSI_HEADER_LEN];

    memset(&rsp, 0, sizeof(ISCSI_LOGOUT_RSP_T));
    if (iscsi_logout_cmd_decap(header, &cmd)!=0) {
        TRACE_ERROR("iscsi_logout_cmd_decap() failed\n");
        return -1;
    }
    sess->StatSN = cmd.ExpStatSN;
    if ((cmd.reason == ISCSI_LOGOUT_CLOSE_RECOVERY) && 
        (!param_equiv(sess->params, "ErrorRecoveryLevel", "0"))) {
        rsp.response = ISCSI_LOGOUT_STATUS_NO_RECOVERY;
    }
    WARN_NOT_EQUAL("CmdSN", cmd.CmdSN, sess->ExpCmdSN);
    RETURN_NOT_EQUAL("ExpStatSN", cmd.ExpStatSN, sess->StatSN, NO_CLEANUP, -1);

    rsp.tag = cmd.tag;
    rsp.StatSN = sess->StatSN;
    rsp.ExpCmdSN = ++sess->ExpCmdSN;
    rsp.MaxCmdSN = sess->MaxCmdSN;
    if (iscsi_logout_rsp_encap(rsp_header, &rsp)!=0) {
        TRACE_ERROR("iscsi_logout_rsp_encap() failed\n");
        return -1;
    }
    if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, rsp_header, 0) != 
        ISCSI_HEADER_LEN) {
        TRACE_ERROR("iscsi_sock_msg() failed\n");
        return -1;
    }
    TRACE(TRACE_DEBUG, 0, "sent logout response OK\n");

    if (!g_silent_login) {
	PRINT("****************************************************************"
	      "****************\n");
	PRINT("*                              LOGOUT SUCCESSFUL                "
	      "               *\n");
	PRINT("*                                                               "
	      "               *\n");
	PRINT("* %25s:%50s *\n", "Type", param_equiv(sess->params, "SessionType", 
						     "Discovery")?"Normal":
	      "Discovery");
	PRINT("* %25s:%50"PRIu64" *\n", "ISID", sess->isid);      
	PRINT("* %25s:%50u *\n", "TSIH", sess->tsih);      
	PRINT("****************************************************************"
	      "****************\n");
    }

    sess->IsLoggedIn = 0;

    return 0;
}

static int verify_cmd_t(TARGET_SESSION_T *sess, unsigned char *header) {
    int op = ISCSI_OPCODE(header);

    if ((!sess->LoginStarted) && (op!=ISCSI_LOGIN_CMD)) {
        // Terminate the connection
        TRACE_ERROR("session %i: iSCSI op 0x%x attempted before LOGIN PHASE\n",
                    sess->id, op);
        return -1;
    }

    if (!sess->IsFullFeature&&((op!=ISCSI_LOGIN_CMD)&&(op!=ISCSI_LOGOUT_CMD))) {
        ISCSI_LOGIN_RSP_T rsp;
        unsigned char rsp_header[ISCSI_HEADER_LEN];

        TRACE_ERROR("session %i: iSCSI op 0x%x attempted before FULL FEATURE\n",
                    sess->id, op);

        // Create Login Reject response
        memset(&rsp, 0, sizeof(ISCSI_LOGIN_RSP_T));
        rsp.status_class = ISCSI_LOGIN_STATUS_INITIATOR_ERROR;
        rsp.status_detail = 0x0b;
        rsp.version_max = ISCSI_VERSION;
        rsp.version_active = ISCSI_VERSION;

        if (iscsi_login_rsp_encap(rsp_header, &rsp)!=0) {
            TRACE_ERROR("iscsi_login_rsp_encap() failed\n");
            return -1;
        }
        ISCSI_SPIN_LOCK_ELSE(&sess->slock, return -1);
        if (iscsi_sock_send_header_and_data(sess->sock, rsp_header, 
                                            ISCSI_HEADER_LEN, NULL, 0, 0) != 
            ISCSI_HEADER_LEN+rsp.length) {
            TRACE_ERROR("iscsi_sock_send_header_and_data() failed\n");
            ISCSI_SPIN_UNLOCK_ELSE(&sess->slock, return -1);
            return -1;
        }    
        ISCSI_SPIN_UNLOCK_ELSE(&sess->slock, return -1);
        return -1;
    }
    return 0;
}

static int execute_t(TARGET_SESSION_T *sess, unsigned char *header) {
    int op = ISCSI_OPCODE(header);
    int queued = 0;

    if (verify_cmd_t(sess, header) != 0) {
        return -1;
    }

    switch(op) {

    case(ISCSI_TASK_CMD):

        TRACE(TRACE_ISCSI, 0, "ISCSI_TASK_CMD(sess %i)\n", sess->id);
        if (task_command_t(sess, header)!=0) {
            TRACE_ERROR("task_command_t() failed\n");
            return -1;
        }
        break;

    case(ISCSI_NOP_OUT):

        TRACE_CLEAN(TRACE_ISCSI, 0, "NOP_OUT(sess %i, tag 0x%x)\n", 
                    sess->id, ISCSI_ITT(header)); 
        if (nop_out_t(sess, header)!=0) {
            TRACE_ERROR("nop_out_t() failed\n");
            return -1;
        }

        break;

    case(ISCSI_LOGIN_CMD):

        TRACE_CLEAN(TRACE_ISCSI, 0, "ISCSI_LOGIN_CMD(sess %i)\n", sess->id);
        if (login_command_t(sess, header)!=0) {
            TRACE_ERROR("login_command_t() failed\n");
            return -1;
        }
        break;

    case(ISCSI_TEXT_CMD):

        TRACE_CLEAN(TRACE_ISCSI, 0, "ISCSI_TEXT_CMD(sess %i)\n", sess->id);
        if (text_command_t(sess, header)!=0) {
            TRACE_ERROR("text_command_t() failed\n");
            return -1;
        }
        break;

    case (ISCSI_LOGOUT_CMD):

        TRACE_CLEAN(TRACE_ISCSI, 0, "ISCSI_LOGOUT_CMD(sess %i)\n", sess->id);
        if (logout_command_t(sess, header)!=0) {
            TRACE_ERROR("logout_command_t() failed\n");
            return -1;
        }
        break;

    case (ISCSI_SCSI_CMD):

        TRACE_CLEAN(TRACE_ISCSI, 0, 
                    "ISCSI_SCSI_CMD(sess %i tag 0x%x, op 0x%x, lun %u)\n", 
                    sess->id, ISCSI_ITT(header), header[32], ISCSI_LUN(header));
        if (scsi_command_t(sess, header)!=0) {
            TRACE_ERROR("scsi_command_t() failed\n");
            return -1;
        }
        queued = 1;
        break;

    case (ISCSI_WRITE_DATA): {
        ISCSI_WRITE_DATA_T data;
        TARGET_CMD_T *cmd;
        ISCSI_SCSI_CMD_T *args;

        if (iscsi_write_data_decap(header, &data)!=0) {
            TRACE_ERROR("iscsi_write_data_decap() failed\n");
            return -1;
        }

        HASH_GET_ELSE(&g_session[sess->id].cmds, data.tag, cmd, exit(1));

        assert(sess == cmd->sess);
        args = &(cmd->scsi_cmd);

        TRACE_CLEAN(TRACE_ISCSI, 0, 
                    "ISCSI_WRITE_DATA(sess %i tag 0x%x t_tag 0x%x, op 0x%x, "
                    "off %u, len %u, DataSN %u)\n", 
                    sess->id, data.tag, data.transfer_tag, args->cdb[0], 
                    data.offset, data.length, data.DataSN);

        /* wait for command to be ready */
        ISCSI_WAIT(&cmd->ready_queue, cmd->ready);

        /* check args */
        if (sess->sess_params.max_data_seg_length) {
            if (data.length > sess->sess_params.max_data_seg_length ) {
                args->status = 0x02;
                return -1;
            }
        }
        if ((args->bytes_todev + data.length) > args->trans_len) {
            args->status = 0x02;
            return -1;
        }
        if (data.tag != args->tag) {
            TRACE(TRACE_DEBUG, 0, 
                  "Data ITT (%d) does not match with command ITT (%d)\n", 
                  data.tag, args->tag);
            if (data.final) {
                args->status = 0x02;
                return -1;
            } else {
                TRACE(TRACE_DEBUG, 0, "Sending Reject PDU\n");
                if (reject_t(sess, header, 0x09)!=0) { // Invalid PDU Field
                    TRACE(TRACE_DEBUG, 0, "Sending Reject PDU failed\n");
                    return 1;
                }
            }
        }
        if (!(args->trans_len-(args->bytes_todev+data.length))) {
            if (!data.final) {
                PRINT("SORRY: bytes_todev = %u\n", args->bytes_todev); 
                sleep(60);
            }
            RETURN_NOT_EQUAL("Final bit", data.final, 1, NO_CLEANUP, -1);
        }

        /* let the device place the data */
        if (device_place_data(cmd, data.offset, data.length) != 
            data.length) {
            TRACE_ERROR("device_place_data() failed\n");
            exit(1);
        }
        cmd->scsi_cmd.bytes_todev += data.length;
        TRACE(TRACE_ISCSI, 1, 
              "session %i: placed %u bytes of data (%u of %u completed)\n", 
              sess->id, data.length, cmd->scsi_cmd.bytes_todev, 
              cmd->scsi_cmd.trans_len); 

        if (!(cmd->scsi_cmd.trans_len-cmd->scsi_cmd.bytes_todev)) {
            TRACE(TRACE_ISCSI, 0, 
                  "session %i: data transfer complete\n", 
                  cmd->sess->id);
            if (device_place_data(cmd, 0, 0) != 0) {
                TRACE_ERROR("device callback failed to ack end transfer\n");
                return -1;
            }
        } else {

            /* If we're not in R2T mode and reach the first burst, then
               we still need to send an R2T.  If we are in R2T mode, we
               would have already sent the R2T for all the data in
               target_transfer_data */

            if (!cmd->r2t_flag && (!sess->sess_params.initial_r2t &&
                                   (sess->sess_params.first_burst_length && 
                                    args->bytes_todev >= 
                                    sess->sess_params.first_burst_length))) {
                ISCSI_R2T_T r2t;
                int desired_xfer_len;
                unsigned char header[ISCSI_HEADER_LEN];

                if (sess->sess_params.first_burst_length && 
                    args->bytes_todev >= 
                    sess->sess_params.first_burst_length) {
                    TRACE(TRACE_ISCSI, 1, 
                          "reached first burst of %u bytes (%u left)\n", 
                          sess->sess_params.first_burst_length, 
                          args->trans_len - args->bytes_todev);
                }

                desired_xfer_len = args->trans_len - args->bytes_todev;
#if 0
                RETURN_GREATER("Bad xfer len", desired_xfer_len, 
                               sess->sess_params.max_burst_length, 
                               NO_CLEANUP, -1);
#else
                WARN_GREATER("Bad xfer len", desired_xfer_len, 
                               sess->sess_params.max_burst_length);
#endif
                memset(&r2t, 0, sizeof(ISCSI_R2T_T));
                r2t.tag = args->tag;
                r2t.transfer_tag = 0x1234;  
                r2t.ExpCmdSN = sess->ExpCmdSN;
                r2t.MaxCmdSN = sess->MaxCmdSN;
                r2t.StatSN = sess->StatSN;
                r2t.length = desired_xfer_len;
                r2t.offset = args->bytes_todev;
                if (iscsi_r2t_encap(header, &r2t) != 0) {
                    TRACE_ERROR("r2t_encap() failed\n");
                    return -1;
                }
                TRACE(TRACE_ISCSI, 1, 
                      "sending R2T tag 0x%x transfer tag 0x%x len "
                      "%u offset %u\n",
                      r2t.tag, r2t.transfer_tag, r2t.length, r2t.offset);

                ISCSI_SPIN_LOCK_ELSE(&sess->slock, return -1);
                if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, 
                                   header, 0) != ISCSI_HEADER_LEN) {
                    TRACE_ERROR("iscsi_sock_msg() failed\n");
                    return -1;
                }
                ISCSI_SPIN_UNLOCK_ELSE(&sess->slock, return -1);
                cmd->r2t_flag = 1;
            }
        }    
    }
        break;
        
    default:

        TRACE_ERROR("Unknown Opcode 0x%x\n", ISCSI_OPCODE(header));
        if (reject_t(sess, header, 0x04)!=0) {
            TRACE_ERROR("reject_t() failed\n");
            return -1;
        }
        break;
    }
    return queued;
}

/*
 * Currently one thread per session, used for both Rx and Tx.
 */

#define WORKER_FAILURE {sess->IsLoggedIn = 0; EXIT(me, -1)}

int worker_proc_t(void *arg) {
    ISCSI_WORKER_T *me = (ISCSI_WORKER_T *) arg;
    TARGET_SESSION_T *sess = (TARGET_SESSION_T *) me->ptr;
    unsigned char *header;
    ISCSI_PARAMETER_T **l = &sess->params;
    int queued;

    //TRACE_ERROR("session worker started (pid %i, tid %li)\n", 
    //            ISCSI_GETPID, ISCSI_GETTID);
    START_ELSE(me, 1, WORKER_FAILURE);

    /*
     * ISCSI_PARAM_TYPE_LIST foyrmat: <type> <key> <dflt> <valid list values>
     * ISCSI_PARAM_TYPE_BINARY format: <type> <key> <dflt> <valid binary values>
     * ISCSI_PARAM_TYPE_NUMERICAL format: <type> <key> <dflt> <max>
     * ISCSI_PARAM_TYPE_DECLARATIVE format: <type> <key> <dflt> ""
     */

    sess->params = NULL; l = &sess->params;

    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_LIST,                
                        "AuthMethod", "CHAP", "CHAP,None", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_LIST,
                        "CHAP_A", "5", "5", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,
                        "CHAP_N", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,
                        "CHAP_R", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,
                        "CHAP_I", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,
                        "CHAP_C", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,
                        "TargetPortalGroupTag", "1", "1", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_LIST,
                        "HeaderDigest", "None", "None", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_LIST,
                        "DataDigest", "None", "None", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL,
                        "MaxConnections", "1", "1", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,
                        "SendTargets", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,
                        "TargetName", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,
                        "InitiatorName", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,
                        "TargetAlias", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,
                        "InitiatorAlias", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_DECLARATIVE,
                        "TargetAddress", "", "", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_BINARY_OR,
                        "InitialR2T", "Yes", "Yes,No", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_BINARY_AND,
                        "OFMarker", "No", "Yes,No", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_BINARY_AND,
                        "IFMarker", "No", "Yes,No", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL_Z,
                        "OFMarkInt", "1", "65536", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL_Z, 
                        "IFMarkInt", "1", "65536", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_BINARY_AND,
                        "ImmediateData", "Yes", "Yes,No", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL_Z,
                        "MaxRecvDataSegmentLength", "8192", "1048576", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL_Z,
                        "FirstBurstLength", "65536", "1048576", return -1);
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_NUMERICAL_Z,         
                        "MaxBurstLength", "262144", "1048576", return -1);
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

    /* Auth Result is not in specs, we use this key to pass 
       authentication result */
    PARAM_LIST_ADD_ELSE(l, ISCSI_PARAM_TYPE_LIST,
                        "AuthResult", "No", "Yes,No,Fail", return -1);

    /* Set remaining session parameters  */
    sess->UsePhaseCollapsedRead = ISCSI_USE_PHASE_COLLAPSED_READ_DFLT;

    /* hash of outstanding commands */
    if (hash_init(&sess->cmds, CONFIG_TARGET_MAX_QUEUE)!=0) {
        TRACE_ERROR("hash_init() failed\n"); 
        return -1;
    }   

    /* Loop for commands or data */

    while (!WORKER_STOPPING(me)) {
        
        if ((header=iscsi_queue_remove(&g_header_q[sess->id]))==NULL) {
            TRACE_ERROR("worker %i: No free iSCSI headers\n", me->id);
            WORKER_FAILURE;
        } else {
            TRACE(TRACE_ISCSI, 1, "session %i: reading into header %p\n", 
                  sess->id, header); 
            if (iscsi_sock_msg(sess->sock, 0, ISCSI_HEADER_LEN, header, 0)!=
                ISCSI_HEADER_LEN) {
                TRACE(TRACE_ISCSI, 0, "session %i: iscsi_sock_msg() failed "
                      "(ending session)\n", sess->id);
                if (iscsi_queue_insert(&g_header_q[sess->id], header)!=0) {
                    TRACE_ERROR("iscsi_queue_insert() failed\n");
                    break;
                }
                break;
            }
            TRACE(TRACE_ISCSI, 1, 
                  "session %i: received iSCSI op 0x%x (lun %u, tag 0x%x, "
                  "header %p, len %u, CmdSN %u)\n", 
                  sess->id, ISCSI_OPCODE(header), ISCSI_LUN(header), 
                  ISCSI_ITT(header), header, 
                  ISCSI_LEN(header), ISCSI_CMD_SN(header));
#if 0
            if (ISCSI_OPCODE(header)!=ISCSI_SCSI_CMD) {
                TRACE_ERROR( 
                      "session %i: received iSCSI op 0x%x (lun %u, tag 0x%x, "
                      "header %p, len %u, CmdSN %u)\n", 
                      sess->id, ISCSI_OPCODE(header), ISCSI_LUN(header), 
                      ISCSI_ITT(header), header, 
                      ISCSI_LEN(header), ISCSI_CMD_SN(header));
            }
#endif
            if ((queued=execute_t(sess, header))<0) {
                TRACE_ERROR("execute_t() failed\n");
                break;
            }      
            if (!queued) {
                TRACE(TRACE_ISCSI, 1, 
                      "session %i: sync. completed iSCSI op 0x%x (tag 0x%x, "
                      "header %p)\n", 
                      sess->id, ISCSI_OPCODE(header), ISCSI_ITT(header), 
                      header);
                if (iscsi_queue_insert(&g_header_q[sess->id], header)!=0) {
                    TRACE_ERROR("iscsi_queue_insert() failed\n"); sleep(60);
                    break;
                }
            } else {
                TRACE(TRACE_ISCSI, 1, "session %i: queued op 0x%x ok\n", 
                      sess->id, ISCSI_OPCODE(header));
            }
            if (ISCSI_OPCODE(header) == ISCSI_LOGOUT_CMD) {
                TRACE(TRACE_ISCSI, 1, 
                      "session %i: logout, ending session\n", sess->id);
                break;
            }
        }
    }

    /* terminate connection */
    if (iscsi_sock_close(sess->sock)!=0) {
        TRACE_ERROR("iscsi_sock_close() failed\n");
        WORKER_FAILURE;
    }

    /* clean up any outstanding commands */
    if (hash_count(&sess->cmds)!=0) {
        TARGET_CMD_T *ptr;

        TRACE_ERROR("session %i has %i outstanding commands\n", 
                    sess->id, hash_count(&sess->cmds));

        while((ptr=hash_remove_head(&sess->cmds))!=NULL) {
            PRINT("0x%x (op 0x%x, r2t_flag %i, ready %i)\n", 
                  ptr->scsi_cmd.tag, ptr->scsi_cmd.cdb[0], 
                  ptr->r2t_flag, ptr->ready);
        }

#if 0
        int (*callback)(void *arg);
        void *callback_arg;
        struct iovec scatter_list[CONFIG_TARGET_MAX_IOV_LEN];
        int scatter_type[CONFIG_TARGET_MAX_IOV_LEN];
        int scatter_align[CONFIG_TARGET_MAX_IOV_LEN];
        int scatter_len;
        struct iovec gather_list[CONFIG_TARGET_MAX_IOV_LEN];
        int gather_type[CONFIG_TARGET_MAX_IOV_LEN];
        int gather_align[CONFIG_TARGET_MAX_IOV_LEN];
        int gather_len;
        struct target_cmd_t *next;
        unsigned char *header;
        unsigned char immediate[CONFIG_TARGET_MAX_IMMEDIATE];
        ISCSI_WQ_T ready_queue;
        HASH_ELEM_T hash;
#endif

        hash_print(&sess->cmds);
    }
    if (hash_destroy(&sess->cmds)!=0) {
        TRACE_ERROR("hash_destroy() failed\n");
        return -1;
    }

    /* clean up parameter list */ 
    if (param_list_destroy(sess->params)!=0) {
        TRACE_ERROR("param_list_destroy() failed\n");
        WORKER_FAILURE;
    }

    /* free security cookie */

    if (g_cookie[sess->id]) {
        iscsi_free_atomic(g_cookie[sess->id]);
        g_cookie[sess->id] = NULL;
    }
    
    /* make session available */
    if (iscsi_queue_insert(&g_session_q, sess)!=0) {
        TRACE_ERROR("iscsi_queue_insert() failed\n");
        WORKER_FAILURE;
    }
    
    sess->IsLoggedIn = 0;

    EXIT(me, 0);
}

int target_transfer_data(TARGET_CMD_T *cmd) {
    TARGET_SESSION_T *sess = cmd->sess;
    ISCSI_SCSI_CMD_T *args = &(cmd->scsi_cmd);
  
    if (!args->todev||!(args->trans_len-args->bytes_todev)) {
        TRACE_ERROR("nothing to transfer for tag 0x%x\n", args->tag);
    }
    TRACE(TRACE_ISCSI, 0, "session %i: transferring %u bytes of write data\n",  
          cmd->sess->id, args->trans_len-args->bytes_todev);

    /* if transfer is done, then signal the device */
    if (!(args->trans_len-args->bytes_todev)) {
        PRINT("session %i: data transfer COMPLETE for 0x%x (notifying "
              "device)\n", 
              cmd->sess->id, args->tag);
        TRACE(TRACE_ISCSI, 1, 
              "session %i: data transfer COMPLETE for 0x%x (notifying "
              "device)\n", 
              cmd->sess->id, args->tag);
        RETURN_NOT_EQUAL("Final bit", args->final, 1, NO_CLEANUP, -1);
        if (device_place_data(cmd, 0, 0) != 0) {
            TRACE_ERROR("device callback failed to ack end of transfer\n");
            exit(1);
        }
        return 0;
    } else {
        TRACE(TRACE_ISCSI, 1, 
              "data transfer is NOT complete for 0x%x (%u of %u remaining)\n", 
              args->tag, args->trans_len-args->bytes_todev, args->trans_len);
    }

    /* Otherwise keep going. If we're running in R2T mode, or we've
     * reached the first burst of unsolicted data, then we must send
     * and R2T for the rest of the remaining data. */

    if (sess->sess_params.initial_r2t ||
        (sess->sess_params.first_burst_length && args->bytes_todev >= 
         sess->sess_params.first_burst_length)) {
        ISCSI_R2T_T r2t;
        int desired_xfer_len, transferred, remaining;
        unsigned char header[ISCSI_HEADER_LEN];
        int r2tcount=0;
        
        transferred = 0;
        remaining = desired_xfer_len = args->trans_len - args->bytes_todev;
#if 0
        RETURN_GREATER("Bad xfer len", desired_xfer_len, 
                       sess->sess_params.max_burst_length, NO_CLEANUP, -1);
#endif
	while (remaining) {
            desired_xfer_len = remaining;
            if (desired_xfer_len > sess->sess_params.max_burst_length) {
                desired_xfer_len = sess->sess_params.max_burst_length;
            }
            memset(&r2t, 0, sizeof(ISCSI_R2T_T));
            r2t.tag = args->tag;
            r2t.transfer_tag = 0x1234;  
            r2t.ExpCmdSN = sess->ExpCmdSN;
            r2t.MaxCmdSN = sess->MaxCmdSN;
            r2t.StatSN = sess->StatSN;
            r2t.length = desired_xfer_len;
            r2t.offset = args->bytes_todev + transferred;
            if (iscsi_r2t_encap(header, &r2t) != 0) {
                TRACE_ERROR("r2t_encap() failed\n");
                return -1;
            }

            TRACE(TRACE_ISCSI, 0, 
                  "session %i: sending R2T (tag 0x%x transfer tag 0x%x len %u "
                  "offset %u)\n", sess->id, r2t.tag, r2t.transfer_tag, 
                  r2t.length, r2t.offset);
            
            ISCSI_SPIN_LOCK_ELSE(&sess->slock, return -1);
            if (iscsi_sock_msg(sess->sock, 1, ISCSI_HEADER_LEN, header, 0) != 
                ISCSI_HEADER_LEN) {
                TRACE_ERROR("iscsi_sock_msg() failed\n");
                return -1;
            }
            ISCSI_SPIN_UNLOCK_ELSE(&sess->slock, return -1);
            cmd->r2t_flag = 1;
            transferred += desired_xfer_len;
            remaining -= desired_xfer_len;
            r2tcount++;
        }
    }     

    ISCSI_WAKE_ELSE(cmd->ready = 1, &cmd->ready_queue, return -1);

    return 0;
}
