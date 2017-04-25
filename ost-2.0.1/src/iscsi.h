
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

#ifndef ISCSI_H
#define ISCSI_H
#ifndef __KERNEL__
#include <sys/time.h>
#include <unistd.h>
#endif
#include "util.h"
#include "debug.h"

#define ISCSI_VERSION 0

/*
 * Constants
 */

#define CONFIG_ISCSI_MAX_AHS_LEN 128

/*
 * Parameters
 */

#define ISCSI_IMMEDIATE_DATA_DFLT            1
#define ISCSI_INITIAL_R2T_DFLT               1
#define ISCSI_USE_PHASE_COLLAPSED_READ_DFLT  0
#define ISCSI_HEADER_LEN                     48
#define ISCSI_PORT                           3260 // Default port
#define ISCSI_OPCODE(HEADER) (HEADER[0]&0x3f)
#define ISCSI_ITT(HEADER)    (NTOHL(*((unsigned *)(HEADER+16))))
#define ISCSI_TTT(HEADER)    (NTOHL(*((unsigned *)(HEADER+20))))
#define ISCSI_LEN(HEADER)    (NTOHL(*((unsigned *)(HEADER+4)))&0x00ffffff)
#define ISCSI_LUN(HEADER)    (HEADER[9])
#define ISCSI_CMD_SN(HEADER) (NTOHL(*((unsigned *)(HEADER+24))))   


/*
 * Opcodes
 */

#define ISCSI_NOP_OUT       0x00
#define ISCSI_SCSI_CMD      0x01
#define ISCSI_TASK_CMD      0x02
#define ISCSI_LOGIN_CMD     0x03
#define ISCSI_TEXT_CMD      0x04
#define ISCSI_WRITE_DATA    0x05
#define ISCSI_LOGOUT_CMD    0x06
#define ISCSI_NOP_IN        0x20
#define ISCSI_SCSI_RSP      0x21
#define ISCSI_TASK_RSP      0x22
#define ISCSI_LOGIN_RSP     0x23
#define ISCSI_TEXT_RSP      0x24
#define ISCSI_READ_DATA     0x25
#define ISCSI_LOGOUT_RSP    0x26
#define ISCSI_R2T           0x31
#define ISCSI_REJECT        0x3f
#define ISCSI_SNACK         0x10  // not implemented
#define ISCSI_ASYNC         0x32

/*
 * Login Phase
 */


#define ISCSI_LOGIN_STATUS_SUCCESS          0
#define ISCSI_LOGIN_STATUS_REDIRECTION      1
#define ISCSI_LOGIN_STATUS_INITIATOR_ERROR  2
#define ISCSI_LOGIN_STATUS_TARGET_ERROR     3
#define ISCSI_LOGIN_STAGE_SECURITY          0
#define ISCSI_LOGIN_STAGE_NEGOTIATE         1
#define ISCSI_LOGIN_STAGE_FULL_FEATURE      3


/*
 * Logout Phase
 */


#define ISCSI_LOGOUT_CLOSE_SESSION      0
#define ISCSI_LOGOUT_CLOSE_CONNECTION   1
#define ISCSI_LOGOUT_CLOSE_RECOVERY     2
#define ISCSI_LOGOUT_STATUS_SUCCESS     0
#define ISCSI_LOGOUT_STATUS_NO_CID      1
#define ISCSI_LOGOUT_STATUS_NO_RECOVERY 2
#define ISCSI_LOGOUT_STATUS_FAILURE     3


/*
 * Task Command 
 */

#define ISCSI_TASK_CMD_ABORT_TASK           1
#define ISCSI_TASK_CMD_ABORT_TASK_SET       2
#define ISCSI_TASK_CMD_CLEAR_ACA            3
#define ISCSI_TASK_CMD_CLEAR_TASK_SET       4
#define ISCSI_TASK_CMD_LOGICAL_UNIT_RESET   5
#define ISCSI_TASK_CMD_TARGET_WARM_RESET    6
#define ISCSI_TASK_CMD_TARGET_COLD_RESET    7
#define ISCSI_TASK_CMD_TARGET_REASSIGN      8

typedef struct iscsi_task_cmd_t {
  int immediate;
  unsigned char function;
  uint64_t lun;
  unsigned tag;
  unsigned ref_tag;
  unsigned CmdSN;
  unsigned ExpStatSN;
  unsigned RefCmdSN;
  unsigned ExpDataSN;
} ISCSI_TASK_CMD_T;

int iscsi_task_cmd_encap(unsigned char *header, ISCSI_TASK_CMD_T *cmd);
int iscsi_task_cmd_decap(unsigned char *header, ISCSI_TASK_CMD_T *cmd);


/*
 * Task Response
 */

#define ISCSI_TASK_RSP_FUNCTION_COMPLETE  0
#define ISCSI_TASK_RSP_NO_SUCH_TASK       1
#define ISCSI_TASK_RSP_NO_SUCH_LUN        2
#define ISCSI_TASK_RSP_STILL_ALLEGIANT    3
#define ISCSI_TASK_RSP_NO_FAILOVER        4
#define ISCSI_TASK_RSP_NO_SUPPORT         5
#define ISCSI_TASK_RSP_AUTHORIZED_FAILED  6

#define ISCSI_TASK_RSP_REJECTED           255

#define ISCSI_TASK_QUAL_FUNCTION_EXECUTED  0
#define ISCSI_TASK_QUAL_NOT_AUTHORIZED     1

typedef struct iscsi_task_rsp_t {
  unsigned char response;
  unsigned length;
  unsigned tag;
  unsigned StatSN;
  unsigned ExpCmdSN;
  unsigned MaxCmdSN;
} ISCSI_TASK_RSP_T;

int iscsi_task_rsp_encap(unsigned char *header, ISCSI_TASK_RSP_T *rsp);
int iscsi_task_rsp_decap(unsigned char *header, ISCSI_TASK_RSP_T *rsp);


/*
 * NOP-Out
 */


typedef struct iscsi_nop_out_args_t {
  int immediate;
  unsigned length;
  uint64_t lun;
  unsigned tag;
  unsigned transfer_tag;
  unsigned CmdSN;
  unsigned ExpStatSN;
  unsigned char *data;
} ISCSI_NOP_OUT_T;

int iscsi_nop_out_encap(unsigned char *header, ISCSI_NOP_OUT_T *cmd);
int iscsi_nop_out_decap(unsigned char *header, ISCSI_NOP_OUT_T *cmd);


/*
 * NOP-In
 */


typedef struct iscsi_nop_in_args_t {
  unsigned length;
  uint64_t lun;
  unsigned tag;
  unsigned transfer_tag;
  unsigned StatSN;
  unsigned ExpCmdSN;
  unsigned MaxCmdSN;
} ISCSI_NOP_IN_T;

int iscsi_nop_in_encap(unsigned char *header, ISCSI_NOP_IN_T *cmd);
int iscsi_nop_in_decap(unsigned char *header, ISCSI_NOP_IN_T *cmd);


/*
 * Text Command
 */


typedef struct iscsi_text_cmd_args_t {
  int immediate;
  int final;
  int cont;
  unsigned length;
  uint64_t lun;
  unsigned tag;
  unsigned transfer_tag;
  unsigned CmdSN;
  unsigned ExpStatSN;
  char *text;
} ISCSI_TEXT_CMD_T;

int iscsi_text_cmd_encap(unsigned char *header, ISCSI_TEXT_CMD_T *cmd);
int iscsi_text_cmd_decap(unsigned char *header, ISCSI_TEXT_CMD_T *cmd);


/*
 * Text Response
 */


typedef struct iscsi_text_rsp_args_t {
  int final;
  int cont;
  unsigned length;
  uint64_t lun;
  unsigned tag;
  unsigned transfer_tag;
  unsigned StatSN;
  unsigned ExpCmdSN;
  unsigned MaxCmdSN;
} ISCSI_TEXT_RSP_T;

int iscsi_text_rsp_encap(unsigned char *header, ISCSI_TEXT_RSP_T *rsp);
int iscsi_text_rsp_decap(unsigned char *header, ISCSI_TEXT_RSP_T *rsp);


/*
 * Login Command
 */


typedef struct iscsi_login_cmd_args_t {
  int transit;
  int cont;
  unsigned char csg;
  unsigned char nsg;
  char version_max;
  char version_min;
  unsigned char AHSlength;
  unsigned length;
  uint64_t isid;
  unsigned short tsih;
  unsigned tag;
  unsigned short cid;
  unsigned CmdSN;
  unsigned ExpStatSN;
  char *text;
} ISCSI_LOGIN_CMD_T;

int iscsi_login_cmd_encap(unsigned char *header, ISCSI_LOGIN_CMD_T *cmd);
int iscsi_login_cmd_decap(unsigned char *header, ISCSI_LOGIN_CMD_T *cmd);


/*
 * Login Response
 */


typedef struct iscsi_login_rsp_args_t {
  int transit;
  int cont;
  unsigned char csg;
  unsigned char nsg;
  char version_max;
  char version_active;
  unsigned char AHSlength;
  unsigned length;
  uint64_t isid;
  unsigned short tsih;
  unsigned tag;
  unsigned StatSN;
  unsigned ExpCmdSN;
  unsigned MaxCmdSN;
  unsigned char status_class;
  unsigned char status_detail;
} ISCSI_LOGIN_RSP_T;

int iscsi_login_rsp_encap(unsigned char *header, ISCSI_LOGIN_RSP_T *rsp);
int iscsi_login_rsp_decap(unsigned char *header, ISCSI_LOGIN_RSP_T *rsp);


/* 
 * Logout Command
 */


typedef struct iscsi_logout_cmd_args_t {
  int immediate;
  unsigned char reason;
  unsigned tag;
  unsigned short cid;
  unsigned CmdSN;
  unsigned ExpStatSN;
} ISCSI_LOGOUT_CMD_T;

int iscsi_logout_cmd_encap(unsigned char *header, ISCSI_LOGOUT_CMD_T *cmd);
int iscsi_logout_cmd_decap(unsigned char *header, ISCSI_LOGOUT_CMD_T *cmd);


/*
 * Logout Response
 */


typedef struct iscsi_logout_rsp_args_t {
  unsigned char response;
  unsigned length;
  unsigned tag;
  unsigned StatSN;
  unsigned ExpCmdSN;
  unsigned MaxCmdSN;
  unsigned short Time2Wait;
  unsigned short Time2Retain;
} ISCSI_LOGOUT_RSP_T;

int iscsi_logout_rsp_encap(unsigned char *header, ISCSI_LOGOUT_RSP_T *rsp);
int iscsi_logout_rsp_decap(unsigned char *header, ISCSI_LOGOUT_RSP_T *rsp);


/*
 * SCSI Command
 */


typedef struct iscsi_scsi_cmd_args_t {
    int immediate;
    int final;
    int fromdev;
    unsigned bytes_fromdev;
    int todev;
    unsigned bytes_todev;
    unsigned char attr;
    unsigned length;
    uint64_t lun;
    unsigned tag;
    unsigned trans_len;
    unsigned bidi_trans_len;
    unsigned CmdSN;
    unsigned ExpStatSN;
    unsigned char *cdb;
    unsigned char *ext_cdb;
    unsigned char ahs[CONFIG_ISCSI_MAX_AHS_LEN];
    unsigned char ahs_len;
    unsigned char *send_data;
    int send_sg_len;
    unsigned char *recv_data;
    int recv_sg_len;
    unsigned char status;
    struct timeval start_time;
    struct timeval stop_time;
    uint64_t lba, len; /* private data */
} ISCSI_SCSI_CMD_T;

int iscsi_scsi_cmd_encap(unsigned char *header, ISCSI_SCSI_CMD_T *cmd);
int iscsi_scsi_cmd_decap(unsigned char *header, ISCSI_SCSI_CMD_T *cmd);


/*
 * SCSI Response
 */


typedef struct iscsi_scsi_rsp_args_t {
  int bidi_overflow;
  int bidi_underflow;
  int overflow;
  int underflow;

  unsigned char response;
  unsigned char status;
  unsigned ahs_len; 
  unsigned length;
  unsigned tag;
  unsigned StatSN;
  unsigned ExpCmdSN;
  unsigned MaxCmdSN;
  unsigned ExpDataSN;
  unsigned bidi_res_cnt;
  unsigned basic_res_cnt;
} ISCSI_SCSI_RSP_T;

int iscsi_scsi_rsp_encap(unsigned char *header, ISCSI_SCSI_RSP_T *rsp);
int iscsi_scsi_rsp_decap(unsigned char *header, ISCSI_SCSI_RSP_T *rsp);


/*
 * Ready To Transfer (R2T)
 */


typedef struct iscsi_r2t_args_t {
  unsigned AHSlength;
  uint64_t lun;
  unsigned tag;
  unsigned transfer_tag;
  unsigned StatSN;
  unsigned ExpCmdSN;
  unsigned MaxCmdSN;
  unsigned R2TSN;
  unsigned offset;
  unsigned length;
} ISCSI_R2T_T;

int iscsi_r2t_encap(unsigned char *header, ISCSI_R2T_T *cmd);
int iscsi_r2t_decap(unsigned char *header, ISCSI_R2T_T *cmd);


/*
 * SCSI Write Data
 */


typedef struct iscsi_write_data_args_t {
  int final;
  unsigned length;
  uint64_t lun;
  unsigned tag;
  unsigned transfer_tag;
  unsigned ExpStatSN;
  unsigned DataSN;
  unsigned offset;
} ISCSI_WRITE_DATA_T;

int iscsi_write_data_encap(unsigned char *header, ISCSI_WRITE_DATA_T *cmd);
int iscsi_write_data_decap(unsigned char *header, ISCSI_WRITE_DATA_T *cmd);


/*
 * SCSI Read Data
 */


typedef struct iscsi_read_data_args_t {
  int final;
  int ack;
  int overflow;
  int underflow;
  int S_bit;
  unsigned char status;
  unsigned length;
  uint64_t lun;
  unsigned task_tag;
  unsigned transfer_tag;
  unsigned StatSN;
  unsigned ExpCmdSN;
  unsigned MaxCmdSN;
  unsigned DataSN;
  unsigned offset;
  unsigned res_count;
} ISCSI_READ_DATA_T;

int iscsi_read_data_encap(unsigned char *header, ISCSI_READ_DATA_T *cmd);
int iscsi_read_data_decap(unsigned char *header, ISCSI_READ_DATA_T *cmd);


/*
 * Reject
 */

typedef struct iscsi_reject_args_t {
  unsigned char reason;
  unsigned length;
  unsigned StatSN;
  unsigned ExpCmdSN;
  unsigned MaxCmdSN;
  unsigned DataSN;
  char *header;

} ISCSI_REJECT_T;

int iscsi_reject_encap(unsigned char *header, ISCSI_REJECT_T *cmd);
int iscsi_reject_decap(unsigned char *header, ISCSI_REJECT_T *cmd);

/*
 * Async Message
 */

typedef struct iscsi_async_msg_args_t {
  unsigned char AHSlength;
  uint64_t lun;
  unsigned StatSN;
  unsigned ExpCmdSN;
  unsigned MaxCmdSN;
  unsigned length;
  unsigned char AsyncEvent;
  unsigned char AsyncVCode;
} ISCSI_AMSG_T;

int iscsi_amsg_decap(unsigned char *header, ISCSI_AMSG_T *msg);

#endif /* ISCSI_H */

