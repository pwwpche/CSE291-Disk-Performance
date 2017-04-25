
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

/*
 * Transport-independent Methods
 */


#ifndef _OSD_OPS_H_
#define _OSD_OPS_H_

#include "osd.h"
#include "initiator.h"

typedef struct osd_ops_mem {
  void *send_data;  
  uint64_t send_len; 
  int send_sg;
  void *recv_data; 
  uint64_t recv_len; 
  int recv_sg;
} OSD_OPS_MEM_T;

typedef struct osd_device_t {
    uint64_t tid;
    uint64_t lun;
} OSD_DEVICE_T;

int osd_create_group(void *dev,
                     int (*osd_exec)(void *dev, osd_args_t *args, 
                                     OSD_OPS_MEM_T *mem),
                     uint32_t *gid);
int osd_remove_group(void *dev, uint32_t GroupID,
                     int (*osd_exec)(void *dev, osd_args_t *args,
                                     OSD_OPS_MEM_T *mem));
int osd_create(void *dev, uint32_t GroupID,
               int (*osd_exec)(void *dev, osd_args_t *args, 
                               OSD_OPS_MEM_T *mem),
               uint64_t *UserID);
int osd_create_and_set(void *dev, uint32_t GroupID,
                       int (*osd_exec)(void *dev, osd_args_t *args, 
                                       OSD_OPS_MEM_T *mem),
                       uint64_t *UserID,
                       uint32_t page, uint32_t index, uint32_t len, 
                       void *value);
int osd_remove(void *dev, uint32_t GroupID, uint64_t UserID,
               int (*osd_exec)(void *dev, osd_args_t *args, 
                               OSD_OPS_MEM_T *mem));
int osd_write(void *dev, 
              uint32_t GroupID, uint64_t UserID, uint64_t offset,
              uint64_t len, void *send_data, int send_sg,
              int (*osd_exec)(void *dev, osd_args_t *args, 
                              OSD_OPS_MEM_T *mem));
int osd_read(void *dev,
             uint32_t GroupID, uint64_t UserID, uint64_t offset, 
             uint64_t len, void *recv_data, int recv_sg,
             int (*osd_exec)(void *dev, osd_args_t *args, 
                             OSD_OPS_MEM_T *mem));
int osd_set_one_attr(void *dev, 
                     uint32_t GroupID, uint64_t UserID, uint32_t page,
                     uint32_t index, uint32_t len, void *value,
                     int (*osd_exec)(void *dev, osd_args_t *args, 
                                     OSD_OPS_MEM_T *mem));
int osd_get_one_attr(void *dev, 
                     uint32_t GroupID, uint64_t UserID, uint32_t page, 
                     uint32_t index, uint32_t alloc_len,
                     int (*osd_exec)(void *dev, osd_args_t *args, 
                                     OSD_OPS_MEM_T *mem),
                     uint16_t *len, void *value);

int osd_command(void *dev, osd_args_t *args, OSD_OPS_MEM_T *mem);
int osd_command_async(INITIATOR_CMD_T *cmd, ISCSI_SCSI_CMD_T *scsi_cmd, 
                      unsigned char *cdb,
                      void *dev, osd_args_t *args, OSD_OPS_MEM_T *m, 
                      int (*callback)(void *arg), void *callback_arg);
int osd_command_build(ISCSI_SCSI_CMD_T *scsi_cmd, osd_args_t *args, 
                      unsigned char *cdb, OSD_OPS_MEM_T *m, OSD_DEVICE_T *dev);
#endif
