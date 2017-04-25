
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

#include <inttypes.h>
#include "util.h"
#include "debug.h"
#include "osd.h"
#include "osd_ops.h"

int extract_attribute(uint32_t page, uint32_t index, uint16_t len,
                      uint8_t *data, unsigned length, void *val) {
    int i = 0;

    for (i=0; i<length;) {
        if (NTOHL(*(uint32_t *)(data+i))!=page) {
            TRACE_ERROR("page mismatch: got 0x%x, expected 0x%x\n", 
                        NTOHL(*(uint32_t *)(data+i)), page);
            return -1;
        }
        i+=4;
        if (NTOHL(*(uint32_t *)(data+i))!=index) {
            TRACE_ERROR("index mismatch\n");
            return -1;
        }
        i+=4;
        if (NTOHS(*(uint16_t *)(data+i))!=len) {
            TRACE_ERROR("len mismatch\n");
            return -1;
        }
        i+=2;
        TRACE(TRACE_DEBUG, 0, "page 0x%x, index %u, len %u\n", page, 
              index, len);
        memcpy(val, data+i, len);
        i+=len;
    }
    TRACE(TRACE_DEBUG, 0, "parsed %i bytes\n", i);
    return i;
}

int osd_create_and_set(void *dev, uint32_t GroupID,
                       int (*osd_exec)(void *dev, osd_args_t *args, 
                                       OSD_OPS_MEM_T *mem),
                       uint64_t *UserID,
                       uint32_t page, uint32_t index, uint32_t len, 
                       void *value) {
    osd_args_t args;
    uint8_t buffer[1024];
    OSD_OPS_MEM_T mem;

    /* build OSD command */

    memset(&args, 0, sizeof(osd_args_t));
    memset(buffer, 0, 1024);
    args.opcode = 0x7F;
    args.service_action = OSD_CREATE;
    args.GroupID = GroupID;
    args.UserID = *UserID;
    args.get_attributes_page = 0x000000001;
    args.get_attributes_allocation_length = 18;
    mem.recv_data = buffer; 
    mem.recv_len = 18; 
    mem.recv_sg = 0;

    /* optionally set some attributes */

    if (value) {
        *((uint32_t *)(buffer+0)) = HTONL(page);
        *((uint32_t *)(buffer+4)) = HTONL(index);
        *((uint16_t *)(buffer+8)) = HTONS(len);
        memcpy(buffer+10, value, len);
        mem.send_data = buffer; 
        mem.send_len = 10+len; 
        mem.send_sg = 0;
        args.set_attributes_list_length = 10+len;
    } else {
        mem.send_data = NULL; 
        mem.send_len = 0; 
        mem.send_sg = 0;
    }

    if (osd_exec(dev, &args, &mem)!=0) {
        TRACE_ERROR("osd_exec() failed\n");
        return -1;
    }
    if (extract_attribute(0x00000001, 0x2, 8, buffer, 18, UserID)==-1) {
        TRACE_ERROR("extract_attributes() failed\n");
        return -1;
    }
    *UserID = NTOHLL(*UserID);
    TRACE(TRACE_DEBUG, 0, 
          "osd_create(GroupID 0x%x) OK --> UserID 0x%"PRIx64"\n", 
          GroupID, *UserID);

    return 0;
}

int osd_create_group(void *dev, 
                     int (*osd_exec)(void *dev, osd_args_t *args, 
                                     OSD_OPS_MEM_T *mem),
                     uint32_t *GroupID) {
    osd_args_t args;
#if 0
    uint8_t get_list[8];
#endif
    uint8_t get_data[14];
    OSD_OPS_MEM_T mem;

    mem.recv_data = get_data; mem.recv_len = 14; mem.recv_sg = 0;

    memset(&args, 0, sizeof(osd_args_t));
    args.opcode = 0x7F; 
    args.GroupID = *GroupID;
    args.service_action = OSD_CREATE_GROUP;  

#if 0
    args.length = 8;
    args.get_attributes_list_length = 8;
    *((unsigned long*)get_list) = HTONL(0x40000001);
    *((unsigned long*)(get_list+4)) = HTONL(0x1);
    mem.send_data = get_list; mem.send_len = 8; mem.send_sg = 0;
#else
    args.get_attributes_page = 0x40000001;
    mem.send_data = NULL; mem.send_len = 0; mem.send_sg = 0;
#endif

    args.get_attributes_allocation_length = 14;
    if (osd_exec(dev, &args, &mem)!=0) {
        TRACE_ERROR("osd_exec() failed\n");
        return -1;
    }
    if (extract_attribute(0x40000001, 0x1, 4, get_data, 14, GroupID)==-1) {
        TRACE_ERROR("extract_attributes() failed\n");
        return -1;
    }
    *GroupID = NTOHL(*GroupID);
    TRACE(TRACE_DEBUG, 0, "osd_create_group() OK --> GroupID 0x%x\n", *GroupID);

    return 0;
}

int osd_create(void *dev, uint32_t GroupID,
               int (*osd_exec)(void *dev, osd_args_t *args, OSD_OPS_MEM_T *mem),
               uint64_t *UserID) {
    osd_args_t args;
#if 0
    uint8_t get_list[8];
#endif
    uint8_t get_data[18];
    OSD_OPS_MEM_T mem;

    mem.recv_data = get_data; mem.recv_len = 18; mem.recv_sg = 0;

    memset(&args, 0, sizeof(osd_args_t));
    args.opcode = 0x7F;
    args.service_action = OSD_CREATE;
    args.GroupID = GroupID;
    args.UserID = *UserID;

#if 0
    args.length = 8;
    args.get_attributes_list_length = 8;
    *((unsigned long*)get_list) = HTONL(0x00000001);
    *((unsigned long*)(get_list+4)) = HTONL(0x2);
    mem.send_data = get_list; mem.send_len = 8; mem.send_sg = 0;
#else
    args.get_attributes_page = 0x000000001;
    mem.send_data = NULL; mem.send_len = 0; mem.send_sg = 0;
#endif

    args.get_attributes_allocation_length = 18;
    if (osd_exec(dev, &args, &mem)!=0) {
        TRACE_ERROR("osd_exec() failed\n");
        return -1;
    }
    if (extract_attribute(0x00000001, 0x2, 8, get_data, 18, UserID)==-1) {
        TRACE_ERROR("extract_attributes() failed\n");
        return -1;
    }
    *UserID = NTOHLL(*UserID);
    TRACE(TRACE_DEBUG, 0, 
          "osd_create(GroupID 0x%x) OK --> UserID 0x%"PRIx64"\n", 
          GroupID, *UserID);

    return 0;
}

int osd_remove_group(void *dev, uint32_t GroupID,
                     int (*osd_exec)(void *dev, 
                                     osd_args_t *args, OSD_OPS_MEM_T *mem)) {
    osd_args_t args;
    OSD_OPS_MEM_T mem;

    mem.send_data = NULL; mem.send_len = 0; mem.send_sg = 0;
    mem.recv_data = NULL; mem.recv_len = 0; mem.recv_sg = 0;

    memset(&args, 0, sizeof(osd_args_t));
    args.opcode = 0x7F;
    args.service_action = OSD_REMOVE_GROUP;
    args.GroupID = GroupID;
    if (osd_exec(dev, &args, &mem)!=0) {
        TRACE_ERROR("osd_exec() failed\n");
        return -1;
    }
    TRACE(TRACE_DEBUG, 0, "osd_remove_group(Group ID 0x%x) OK\n", GroupID);

    return 0;
}

int osd_remove(void *dev, uint32_t GroupID, uint64_t UserID,
               int (*osd_exec)(void *dev, osd_args_t *args, 
                               OSD_OPS_MEM_T *mem)) {
    osd_args_t args;
    OSD_OPS_MEM_T mem;

    mem.send_data = NULL; mem.send_len = 0; mem.send_sg = 0;
    mem.recv_data = NULL; mem.recv_len = 0; mem.recv_sg = 0;

    memset(&args, 0, sizeof(osd_args_t));
    args.opcode = 0x7F;
    args.service_action = OSD_REMOVE;
    args.UserID = UserID; 
    args.GroupID = GroupID;
    if (osd_exec(dev, &args, &mem)!=0) {
        TRACE_ERROR("osd_exec() failed\n");
        return -1;
    }
    TRACE(TRACE_DEBUG, 0, 
          "osd_remove(GroupID 0x%x, UserID 0x%"PRIx64") OK\n", GroupID, UserID);

    return 0;
}

int osd_write(void *dev, uint32_t GroupID, uint64_t UserID, uint64_t offset, 
              uint64_t len, void *send_data, int sg_len,
              int (*osd_exec)(void *dev, osd_args_t *args, 
                              OSD_OPS_MEM_T *mem)) {
    osd_args_t args;
    OSD_OPS_MEM_T mem;

    TRACE(TRACE_DEBUG, 0, 
          "osd_write(GroupID 0x%x, UserID 0x%"PRIx64", Offset %"PRIu64
          ", Len %"PRIu64")\n", 
          GroupID, UserID, offset, len);
    mem.send_data = send_data; mem.send_len = len; mem.send_sg = sg_len;
    mem.recv_data = NULL; mem.recv_len = 0; mem.recv_sg = 0;
    memset(&args, 0, sizeof(osd_args_t));
    args.opcode = 0x7F;
    args.service_action = OSD_WRITE;
    args.GroupID = GroupID;
    args.UserID = UserID;
    args.offset = offset;
    args.length = len;
    if (osd_exec(dev, &args, &mem)!=0) {
        TRACE_ERROR("osd_exec() failed\n");
        return -1;
    }
    return 0;
}

int osd_read(void *dev, uint32_t GroupID, uint64_t UserID, uint64_t offset, 
             uint64_t len, void *recv_data, int sg_len,
             int (*osd_exec)(void *dev, osd_args_t *args, 
                             OSD_OPS_MEM_T *mem)) {
    osd_args_t args;
    OSD_OPS_MEM_T mem;

    TRACE(TRACE_DEBUG, 0, 
          "osd_read(GroupID 0x%x, UserID 0x%"PRIx64", Offset %"PRIu64
          ", ""Len %"PRIu64")\n", GroupID, UserID, offset, len);
    mem.send_data = NULL; mem.send_len = 0; mem.send_sg = 0;
    mem.recv_data = recv_data; mem.recv_len = len; mem.recv_sg = sg_len;
    memset(&args, 0, sizeof(osd_args_t));
    args.opcode = 0x7F;
    args.service_action = OSD_READ;
    args.GroupID = GroupID;
    args.UserID = UserID;
    args.offset = offset;
    args.length = len;
    if (osd_exec(dev, &args, &mem)!=0) {
        TRACE_ERROR("osd_exec() failed\n");
        return -1;
    }
    return 0;
}

int osd_set_one_attr(void *dev, uint32_t GroupID, uint64_t UserID, 
                     uint32_t page, uint32_t index, uint32_t len, void *value,
                     int (*osd_exec)(void *dev, osd_args_t *args, 
                                     OSD_OPS_MEM_T *mem)) {
    osd_args_t args;
    OSD_OPS_MEM_T mem;
    uint8_t list[10];
#if 0
    struct iovec sg[2];
#else
    uint8_t *buffer = NULL;
#endif

    TRACE(TRACE_DEBUG, 0, "osd_set_one_attr(GroupID 0x%x, UserID 0x%"PRIx64", "
          "Page 0x%x, Index %u, Len %u)\n", GroupID, UserID, page, index, len);
    memset(&args, 0, sizeof(osd_args_t));
    args.opcode = 0x7F;
    args.service_action = OSD_SET_ATTR;
    args.GroupID = GroupID;
    args.UserID = UserID;
    args.length = 10+len;
    args.set_attributes_list_length = 10+len;
    *((uint32_t *)  (list+0)) = HTONL(page);
    *((uint32_t *)  (list+4)) = HTONL(index);
    *((uint16_t *)  (list+8)) = HTONS(len);
#if 0
    sg[0].iov_base = list; 
    sg[0].iov_len = 10;
    sg[1].iov_base = value; 
    sg[1].iov_len = len;
    mem.send_data = sg; mem.send_len = 10+len; mem.send_sg = 2;
#else
    if ((buffer=iscsi_malloc_atomic(10+len))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        return -1; 
    }
    memcpy(buffer, list, 10); 
    memcpy(buffer+10, value, len);
    mem.send_data = buffer; mem.send_len = 10+len; mem.send_sg = 0;
#endif
    mem.recv_data = NULL; mem.recv_len = 0; mem.recv_sg = 0;

    if (osd_exec(dev, &args, &mem)!=0) {
        TRACE_ERROR("osd_exec() failed\n");
        return -1;
    }
    if (buffer) iscsi_free_atomic(buffer);

    return 0;
}

int osd_get_one_attr(void *dev, uint32_t GroupID, uint64_t UserID, 
                     uint32_t page, uint32_t index, uint32_t alloc_len,
                     int (*osd_exec)(void *dev, osd_args_t *args, 
                                     OSD_OPS_MEM_T *mem),
                     uint16_t *len, void *value) {
    osd_args_t args;
    OSD_OPS_MEM_T mem;
    uint8_t list_out[8];
#if 0
    uint8_t list_in[10];
    struct iovec sg[2];
#else
    uint8_t *buffer;
#endif

    TRACE(TRACE_DEBUG, 0, "osd_get_one_attr(GroupID 0x%x, UserID 0x%"PRIx64", "
          "Page 0x%x, Index %u, Alloc Len %u)\n", GroupID, UserID, page, 
          index, alloc_len);
    memset(&args, 0, sizeof(osd_args_t));
    args.opcode = 0x7F;
    args.service_action = OSD_GET_ATTR;
    args.GroupID = GroupID;
    args.UserID = UserID;

    //PRINT("GroupID = 0x%x UserID=0x%"PRIx64"\n", GroupID, UserID);

    if (index) {
        args.length = 8;
        *((uint32_t *)(list_out+0)) = HTONL(page);
        *((uint32_t *)(list_out+4)) = HTONL(index);
        args.get_attributes_list_length = 8;
        mem.send_data = list_out; mem.send_len = 8; mem.send_sg = 0;
    } else {
        TRACE(TRACE_DEBUG, 0, "requesting entire page or reference page\n");
        mem.send_data = NULL; mem.send_len = 0; mem.send_sg = 0;
        args.get_attributes_page = page;
    }
#if 0
    sg[0].iov_base = list_in; sg[0].iov_len = 10;
    sg[1].iov_base = value; sg[1].iov_len = alloc_len;
    mem.recv_data = sg; mem.recv_len = 10+alloc_len; mem.recv_sg = 2;
#else
    if ((buffer=iscsi_malloc_atomic(10+alloc_len))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        return -1; 
    }
    mem.recv_data = buffer; mem.recv_len = 10+alloc_len; mem.recv_sg = 0;
#endif
    args.get_attributes_allocation_length = 10+alloc_len;
    if (osd_exec(dev, &args, &mem)!=0) {
        TRACE_ERROR("osd_exec() failed\n");
        return -1;
    }
    memcpy(value, buffer+10, alloc_len);
    if (buffer) iscsi_free_atomic(buffer);
    return 0;
}

int osd_command_build(ISCSI_SCSI_CMD_T *scsi_cmd, osd_args_t *args, 
                      unsigned char *cdb, OSD_OPS_MEM_T *m, OSD_DEVICE_T *dev) {
    unsigned char *ahs_ptr = scsi_cmd->ahs;
    int ahs_len = 0;

    /* build CDB */

    memset(cdb, 0, CONFIG_OSD_CDB_LEN);
    OSD_ENCAP_CDB(args, cdb);
    //OSD_PRINT_CDB(cdb, cdb+16);

    /* AHS for expected bidi read length */

    memset(scsi_cmd, 0, sizeof(ISCSI_SCSI_CMD_T));
    if (m->send_len && m->recv_len) {
        memset(ahs_ptr, 0, 8);
        // AHS length (not v20 compliant, should be 0x0005)
        *((unsigned  *)ahs_ptr) = HTONS(8);
        // Type        
        ahs_ptr[2] = 0x02;     
        // Expected Read length          
        *((unsigned *)(ahs_ptr+4)) = HTONL(m->recv_len);  
        ahs_ptr += 8; ahs_len += 8;
    }

    /* AHS for extended CDB */

    memset(ahs_ptr, 0, 4);
    *((uint16_t *)ahs_ptr) = HTONS(CONFIG_OSD_CDB_LEN-15); // AHS length
    ahs_ptr[2] = 0x01;                                     // Type
    memcpy(ahs_ptr+4, cdb+16, CONFIG_OSD_CDB_LEN-16);      // Copy remaining CDB
    ahs_ptr += CONFIG_OSD_CDB_LEN-15; 
    ahs_len += CONFIG_OSD_CDB_LEN-15; 

    /* iSCSI SCSI command */

    scsi_cmd->cdb = cdb;
    scsi_cmd->ahs_len = ahs_len;
    if (m->send_len && m->recv_len) {
        scsi_cmd->fromdev = 1;
        scsi_cmd->todev = 1;
        scsi_cmd->length = m->send_len;
        scsi_cmd->trans_len = m->send_len;
        scsi_cmd->bidi_trans_len = m->recv_len;
        scsi_cmd->send_data = m->send_data;
        scsi_cmd->send_sg_len = m->send_sg;
        scsi_cmd->recv_data = m->recv_data;
        scsi_cmd->recv_sg_len = m->recv_sg;
    } else if (m->send_len) {
        scsi_cmd->todev = 1;
        scsi_cmd->length = m->send_len;
        scsi_cmd->trans_len = m->send_len;
        scsi_cmd->send_data = m->send_data;
        scsi_cmd->send_sg_len = m->send_sg;
    } else if (m->recv_len) {
        scsi_cmd->fromdev = 1;
        scsi_cmd->trans_len = m->recv_len;
        scsi_cmd->recv_data = m->recv_data;
        scsi_cmd->recv_sg_len = m->recv_sg;
    }

    /* set lun */

    scsi_cmd->lun = dev->lun;

    return 0;
}

int osd_command(void *dev, osd_args_t *args, OSD_OPS_MEM_T *m) {
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T scsi_cmd;
    unsigned char cdb[CONFIG_OSD_CDB_LEN];

    osd_command_build( &scsi_cmd, args, cdb, m, dev);
    if (initiator_command(ISCSI_SCSI_CMD, &scsi_cmd, 
                          ((OSD_DEVICE_T *)dev)->tid, &cmd, -1)!=0) {
        TRACE_ERROR("initiator_command() failed\n"); 
        return -1;
    }
    if (scsi_cmd.status!=0) {
        TRACE_ERROR("SCSI command failed\n"); 
        return -1;
    }
    return 0;
}

int osd_command_async(INITIATOR_CMD_T *cmd, ISCSI_SCSI_CMD_T *scsi_cmd, 
                      unsigned char *cdb,
                      void *dev, osd_args_t *args, OSD_OPS_MEM_T *m, 
                      int (*callback)(void *arg), void *callback_arg) {
    osd_command_build(scsi_cmd, args, cdb, m, dev);
    if (initiator_enqueue(ISCSI_SCSI_CMD, scsi_cmd, 
                          ((OSD_DEVICE_T *)dev)->tid, cmd, callback, 
                          callback_arg)!=0) {
        TRACE_ERROR("initiator_enqueue() failed\n"); 
        return -1;
    }
    return 0;
}
