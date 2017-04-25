
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

#ifndef _INITIATOR_H_
#define _INITIATOR_H_
#include "iscsi.h"
#include "parameters.h"

/* This should be at least one less than CONFIG_TARGET_MAX_QUEUE */
#define CONFIG_INITIATOR_QUEUE_DEPTH 64
#define CONFIG_INITIATOR_MAX_TARGETS 8

/***********
 * Private *
 ***********/

#define INITIATOR_STATE_SHUTDOWN 1

typedef struct iscsi_session_t {
    iscsi_socket_t sock;
    unsigned CmdSN, ExpStatSN, MaxCmdSN;
    ISCSI_WORKER_T tx_worker;
    ISCSI_WORKER_T rx_worker;
    uint64_t isid;
    unsigned short tsih, cid;
    ISCSI_PARAMETER_T *params;
    ISCSI_SESS_PARAM_T sess_params;
    HASH_T cmds;
    int redirected;
    int discovery;
} INITIATOR_SESSION_T;

#undef NO_SG_MALLOC
#define CONFIG_INITIATOR_MAX_SG 128

typedef struct initiator_cmd_t {
    void *ptr;
    int type;
    int (*callback)(void *arg);
    void *callback_arg;
    uint64_t isid;
    volatile int tx_done;
    int status;
    unsigned tag;
    int r2t_request;
    int nop_out_request;
    ISCSI_R2T_T r2t;
    HASH_ELEM_T hash;
    int aborted;
    volatile int done;
    ISCSI_WQ_T wq;
#ifdef NO_SG_MALLOC
    struct iovec sg_copy_1[CONFIG_INITIATOR_MAX_SG];
    struct iovec sg_copy_2[CONFIG_INITIATOR_MAX_SG];
#endif
} INITIATOR_CMD_T; 

#define SESSION_STATE_STARTED       0x01
#define SESSION_STATE_INITIALIZED   0x02
#define SESSION_STATE_CONNECTED     0x04
#define SESSION_STATE_LOGGED_IN     0x08
#define SESSION_STATE_FULL_FEATURE  0x10
#define SESSION_STATE_REPORTING     0x20
#define SESSION_STATE_ERROR         0x40
#define SESSION_STATE_DESTROYING    0x80

#define MAX_LEN 256

typedef struct iscsi_target_t {
    INITIATOR_SESSION_T *sess;
    volatile int state;
    int port;
    char ip[MAX_LEN];
    int fix_address;  /* ignore redirection ip:port address if one is given */
    char TargetName[MAX_LEN];
    char InitiatorUser[MAX_LEN];
    char InitiatorPass[MAX_LEN];
    char TargetUser[MAX_LEN];
    char TargetPass[MAX_LEN];
    char ImmediateData[MAX_LEN];
    char InitialR2T[MAX_LEN];
    char MaxRecvDataSegmentLength[MAX_LEN];
    char FirstBurstLength[MAX_LEN];
    char MaxBurstLength[MAX_LEN];
    char which[MAX_LEN];
    unsigned nop_in_count;    /* target initiated */
    unsigned nop_out_count;   /* initiator initiated */
    unsigned errors;
    unsigned attempts;
} INITIATOR_TARGET_T;

/**********
 * Public *
 **********/

#ifdef __cplusplus
#define LNKG extern "C"
#else
#define LNKG
#endif

LNKG int initiator_init(char *config_filename, int verbose, int family);
LNKG int initiator_current_config(void);
LNKG int initiator_info(char *buffer);
LNKG int initiator_show_config(int terse, int ssid);
LNKG int initiator_command(int type, void *ptr, uint64_t tid, 
                           INITIATOR_CMD_T *cmd, int timeout);
LNKG int initiator_enqueue(int type, void *ptr, uint64_t tid, 
                           INITIATOR_CMD_T *cmd, 
                           int (*callback)(void *arg), void *callback_arg);
LNKG int initiator_q_depth(void);
LNKG int initiator_abort(INITIATOR_CMD_T *cmd);
LNKG int initiator_session_reset(uint64_t tid);
LNKG int initiator_shutdown(void);

#endif /* _INITIATOR_H_ */
