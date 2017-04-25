
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

#ifndef _TARGET_H_
#define _TARGET_H_
#include "iscsi.h"
#include "util.h"
#include "parameters.h"

#define CONFIG_TARGET_MAX_SESSIONS  9    /* n+1 */
#define CONFIG_TARGET_MAX_QUEUE     65   /* n+1 */
#define CONFIG_TARGET_MAX_IOV_LEN   512
#define CONFIG_TARGET_MAX_IMMEDIATE 65536

/* CHAP settings */
#define CONFIG_TARGET_USER_IN   "foo"      /* initiator user */
#define CONFIG_TARGET_PASS_IN   "bar"      /* initiator pass */
#define CONFIG_TARGET_USER_OUT  "foo"      /* target user */
#define CONFIG_TARGET_PASS_OUT  "bar"      /* target's secret */

typedef struct session_t {
    int id;
    iscsi_socket_t sock;
    unsigned short cid;
    unsigned StatSN, ExpCmdSN, MaxCmdSN; 
    int UsePhaseCollapsedRead;
    int IsFullFeature;
    int IsLoggedIn;
    int LoginStarted;
    int Authenticated;
    uint64_t isid;
    int tsih;
    ISCSI_WORKER_T worker;
    ISCSI_PARAMETER_T *params;
    ISCSI_SESS_PARAM_T sess_params;
    ISCSI_QUEUE_T device_queue;
    ISCSI_SPIN_LOCK_T slock;
    HASH_T cmds;
} TARGET_SESSION_T;

/* memory types for each element in a scatter/gather list */
#define MEM_STACK   1
#define MEM_HEAP    2
#define MEM_MAPPED  3
#define MEM_ALIGNED 4
#define MEM_STAGING 5
#define MEM_CACHE   6

typedef struct target_cmd_t {
    TARGET_SESSION_T *sess;
    ISCSI_SCSI_CMD_T scsi_cmd;
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
    int r2t_flag;
    unsigned char immediate[CONFIG_TARGET_MAX_IMMEDIATE];
    int ready;
    ISCSI_WQ_T ready_queue;
    HASH_ELEM_T hash;
    int done;
    int is_write;
    int worker_id; /* id of worker handling request (to index into fid) */
    uint64_t issue, complete;
} TARGET_CMD_T;

int target_init(int queue_depth);
int target_shutdown(void);
int target_listen(int port, int delay, int family);
int target_transfer_data(TARGET_CMD_T *cmd);
int read_counters(uint64_t lun, char *data);
#endif /* _TARGET_H_ */
