
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

#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_

typedef struct MD5Context MD5Context_t;

#define ISCSI_PARAM_KEY_LEN         64   // max <key> size (bytes)
#define ISCSI_PARAM_MAX_LEN       4096   // max <value> size (bytes)
#define ISCSI_PARAM_MAX_TEXT_LEN  4096   // max data for text command (bytes)

// Parameter Type

#define ISCSI_PARAM_TYPE_DECLARATIVE      1
#define ISCSI_PARAM_TYPE_DECLARE_MULTI    2  // TargetName and TargetAddress
#define ISCSI_PARAM_TYPE_NUMERICAL        3
#define ISCSI_PARAM_TYPE_NUMERICAL_Z      4  // zero represents no limit
#define ISCSI_PARAM_TYPE_BINARY_OR        5
#define ISCSI_PARAM_TYPE_BINARY_AND       6
#define ISCSI_PARAM_TYPE_LIST             7

#define ISCSI_CHAP_C_MAX_DATA_LENGTH  1024
#define ISCSI_CHAP_C_DATA_LENGTH_I    16  /* target authentication */
#define ISCSI_CHAP_C_DATA_LENGTH_T    16  /* initiator authentication */
#define ISCSI_CHAP_R_DATA_LENGTH      16

/* 2 bytes for 0x, and 2 for each data byte, 1 byte for null */
#define ISCSI_CHAP_STRING_LENGTH     (ISCSI_CHAP_C_MAX_DATA_LENGTH*2+2+1) 

#define ISCSI_PARAM_STATUS_AUTH_FAILED   -2
#define ISCSI_PARAM_STATUS_FAILED        -1
#define ISCSI_PARAM_STATUS_AUTH_SUCCEEDED 1

typedef struct iscsi_parameter_item_t {
  char value[ISCSI_PARAM_MAX_LEN];
  struct iscsi_parameter_item_t *next;
} ISCSI_PARAMETER_VALUE_T;

/* Structure for storing negotiated parameters that are
 * frequently accessed on an active session
 */
typedef struct iscsi_sess_param_t {
  __u32 max_burst_length;
  __u32 first_burst_length;
  __u32 max_data_seg_length;
  __u8 initial_r2t;
  __u8 immediate_data;
} ISCSI_SESS_PARAM_T;

typedef struct iscsi_parameter_t {
  char key[ISCSI_PARAM_KEY_LEN];                         // key
  int  type;                            // type of parameter
  char valid[ISCSI_PARAM_MAX_LEN];      // list of valid values
  char dflt[ISCSI_PARAM_MAX_LEN];       // default value
  ISCSI_PARAMETER_VALUE_T *value_l;     // value list
  char offer_rx[ISCSI_PARAM_MAX_LEN];   // outgoing offer
  char offer_tx[ISCSI_PARAM_MAX_LEN];   // incoming offer
  char answer_tx[ISCSI_PARAM_MAX_LEN];  // outgoing answer
  char answer_rx[ISCSI_PARAM_MAX_LEN];  // incoming answer
  char negotiated[ISCSI_PARAM_MAX_LEN]; // negotiated value
  int tx_offer;                         // sent offer 
  int rx_offer;                         // received offer 
  int tx_answer;                        // sent answer
  int rx_answer;                        // received answer
  int reset;                            // reset value_l
  struct iscsi_parameter_t* next;
} ISCSI_PARAMETER_T;

int param_list_add(ISCSI_PARAMETER_T **head, int type, char *key, 
                   char *dflt, char *valid);
int param_list_print(ISCSI_PARAMETER_T *head);
int param_list_destroy(ISCSI_PARAMETER_T *head);
int param_text_add(ISCSI_PARAMETER_T *head, char *key, char *value, 
                   char *text, int *len, int offer);
int param_text_parse(ISCSI_PARAMETER_T *head, char *text, int text_len, 
                     char *text_out, int *text_len_out, int offer, 
                     void **cookie, 
                     char *InitiatorUser, char *InitiatorPass, 
                     char *TargetUser, char *TargetPass);
int param_text_print(ISCSI_PARAMETER_T *head, char *text, unsigned text_len);
int param_num_vals(ISCSI_PARAMETER_T *head, char* key);
int param_val_reset(ISCSI_PARAMETER_T *head, char* key);
char* param_val(ISCSI_PARAMETER_T *head, char* key);
char* param_val_which(ISCSI_PARAMETER_T *head, char* key, int which);
char* param_offer(ISCSI_PARAMETER_T *head, char* key);
char* param_answer(ISCSI_PARAMETER_T *head, char* key);
ISCSI_PARAMETER_T *param_get(ISCSI_PARAMETER_T *head, char* key);
int param_atoi(ISCSI_PARAMETER_T *head, char* key);
int param_equiv(ISCSI_PARAMETER_T *head, char* key, char *val);
int param_commit(ISCSI_PARAMETER_T *head);
void set_session_parameters( ISCSI_PARAMETER_T *head, 
                             ISCSI_SESS_PARAM_T *sess_params );

/*
 * Macros
 */

#define PARAM_LIST_PRINT_ELSE(LIST, ELSE)                      \
if (param_list_print(LIST)!=0) {                               \
  TRACE_ERROR("param_list_print() failed\n");                  \
  ELSE;                                                        \
}

#define PARAM_TEXT_PRINT_ELSE(TEXT, LEN, ELSE)                 \
if (param_text_print(TEXT, LEN)!=0) {                          \
  TRACE_ERROR("param_text_print() failed\n");                  \
  ELSE;                                                        \
}

#define PARAM_LIST_INIT_ELSE(LIST, ELSE)                       \
if (param_list_init(LIST)!=0) {                                \
  TRACE_ERROR("param_list_init() failed\n");                   \
  ELSE;                                                        \
}

#define PARAM_LIST_DESTROY_ELSE(LIST, ELSE)                    \
if (param_list_destroy(LIST)!=0) {                             \
  TRACE_ERROR("param_list_destroy() failed\n");                \
  ELSE;                                                        \
}

#define PARAM_LIST_ADD_ELSE(LIST, TYPE, KEY, DFLT, VALID, ELSE)   \
if (param_list_add(LIST, TYPE, KEY, DFLT, VALID)!=0) {            \
  TRACE_ERROR("param_list_add() failed\n");                       \
  ELSE;                                                           \
}

#define PARAM_TEXT_ADD_ELSE(LIST, KEY, VAL, TEXT, LEN, OFFER, ELSE )  \
if (param_text_add(LIST, KEY, VAL, TEXT, LEN, OFFER)!=0) {            \
  TRACE_ERROR("param_text_add() failed\n");                           \
  ELSE;                                                               \
}

#define PARAM_TEXT_PARSE_ELSE(LIST, TEXT, LEN, TEXT_OUT, LEN_OUT, OFFER, \
                              COOKIE, IU, IP, TU, TP, ELSE )             \
    if (param_text_parse(LIST, TEXT, LEN, TEXT_OUT, LEN_OUT, OFFER,      \
                         COOKIE, IU, IP, TU, TP)!=0) {                   \
        TRACE_ERROR("param_text_parse_offer() failed\n"); ELSE;          \
}
#endif
