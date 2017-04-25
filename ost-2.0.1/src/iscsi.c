
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
#define PRIu64 "llu"
#define PRIx64 "llx"
#else
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <string.h>
#include <inttypes.h>
#endif
#include "iscsi.h"
#include "util.h"

/*
 * Task Command
 */

int iscsi_task_cmd_encap(unsigned char *header, ISCSI_TASK_CMD_T *cmd) {
    TRACE(TRACE_RPC, 0, "Immediate: %i\n",   cmd->immediate);
    TRACE(TRACE_RPC, 0, "Function:  %u\n",   cmd->function);
    TRACE(TRACE_RPC, 0, "LUN:       %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Tag:       0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Ref Tag:   0x%x\n", cmd->ref_tag);
    TRACE(TRACE_RPC, 0, "CmdSN:     %u\n",   cmd->CmdSN);
    TRACE(TRACE_RPC, 0, "ExpStatSN: %u\n",   cmd->ExpStatSN);
    TRACE(TRACE_RPC, 0, "RefCmdSN:  %u\n",   cmd->RefCmdSN);
    TRACE(TRACE_RPC, 0, "ExpDataSN: %u\n",   cmd->ExpDataSN);

    memset(header, 0, ISCSI_HEADER_LEN);
 
    header[0] |= ISCSI_TASK_CMD;                             // Opcode
    if (cmd->immediate) header[0] |= 0x40;                   // Immediate bit 
    header[1] = cmd->function&0x80;                          // Function 
    header[9] = cmd->lun&0xff;                               // LUN
    *((unsigned *)(header+16)) = HTONL(cmd->tag);            // Tag
    *((unsigned *)(header+20)) = HTONL(cmd->ref_tag);        // Reference Tag
    *((unsigned *)(header+24)) = HTONL(cmd->CmdSN);          // CmdSN
    *((unsigned *)(header+28)) = HTONL(cmd->ExpStatSN);      // ExpStatSN
    *((unsigned *)(header+32)) = HTONL(cmd->RefCmdSN);       // RefCmdSN
    *((unsigned *)(header+36)) = HTONL(cmd->ExpDataSN);      // ExpDataSN

    return 0;
}

int iscsi_task_cmd_decap(unsigned char *header, ISCSI_TASK_CMD_T *cmd) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_TASK_CMD, 
                     NO_CLEANUP, -1);

    cmd->immediate = ((header[0]&0x40)==0x40);              // Immediate bit 
    cmd->function = header[1]&0x80;                         // Function 
    cmd->lun = header[9];                                   // LUN
    cmd->tag = NTOHL(*((unsigned *)(header+16)));           // Tag
    cmd->ref_tag = NTOHL(*((unsigned *)(header+20)));       // Reference Tag
    cmd->CmdSN =  ISCSI_CMD_SN(header);                     // CmdSN
    cmd->ExpStatSN = NTOHL(*((unsigned *)(header+28)));     // ExpStatSN
    cmd->RefCmdSN = NTOHL(*((unsigned *)(header+32)));      // RefCmdSN
    cmd->ExpDataSN = NTOHL(*((unsigned *)(header+36)));     // ExpDataSN

    RETURN_NOT_EQUAL("Byte 1, bit 0", header[1]&0x80, 0x80, NO_CLEANUP, 1); 
    RETURN_NOT_EQUAL("Byte 2", header[2], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 3", header[3], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 4-7", *((unsigned *)(header+4)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 8", header[8], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 40-43", *((unsigned *)(header+40)), 
                     0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 44-47", *((unsigned *)(header+44)), 
                     0, NO_CLEANUP, 1);

    TRACE(TRACE_RPC, 0, "Immediate: %i\n",   cmd->immediate);
    TRACE(TRACE_RPC, 0, "Function:  %u\n",   cmd->function);
    TRACE(TRACE_RPC, 0, "LUN:       %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Tag:       0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Ref Tag:   0x%x\n", cmd->ref_tag);
    TRACE(TRACE_RPC, 0, "CmdSN:     %u\n",   cmd->CmdSN);
    TRACE(TRACE_RPC, 0, "ExpStatSN: %u\n",   cmd->ExpStatSN);
    TRACE(TRACE_RPC, 0, "RefCmdSN:  %u\n",   cmd->RefCmdSN);
    TRACE(TRACE_RPC, 0, "ExpDataSN: %u\n",   cmd->ExpDataSN);

    return 0;
}

/*
 * Task Response
 */

int iscsi_task_rsp_encap(unsigned char *header, ISCSI_TASK_RSP_T *rsp) {

    TRACE(TRACE_RPC, 0, "Response:  %u\n",   rsp->response);
    TRACE(TRACE_RPC, 0, "Length:    %u\n",   rsp->length);
    TRACE(TRACE_RPC, 0, "Tag:       0x%x\n", rsp->tag); 
    TRACE(TRACE_RPC, 0, "StatSN:    %u\n",   rsp->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:  %u\n",   rsp->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:  %u\n",   rsp->MaxCmdSN);

    memset(header, 0, ISCSI_HEADER_LEN);
  
    header[0] |= ISCSI_TASK_RSP;                               // Opcode
    header[1] |= 0x80;                                         // Byte 1 bit 0 
    header[2] = rsp->response;                                 // Response
    *((unsigned *)(header+4)) = HTONL(rsp->length&0x00ffffff); // Length
    *((unsigned *)(header+16)) = HTONL(rsp->tag);              // Tag
    *((unsigned *)(header+24)) = HTONL(rsp->StatSN);           // StatSN
    *((unsigned *)(header+28)) = HTONL(rsp->ExpCmdSN);         // ExpCmdSN
    *((unsigned *)(header+32)) = HTONL(rsp->MaxCmdSN);         // MaxCmdSN

    return 0;
}

int iscsi_task_rsp_decap(unsigned char *header, ISCSI_TASK_RSP_T *rsp) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_TASK_RSP, 
                     NO_CLEANUP, 1);

    rsp->response = header[2];                                 // Response
    rsp->tag = NTOHL(*((unsigned *)(header+16)));              // Tag
    rsp->StatSN = NTOHL(*((unsigned *)(header+24)));           // StatSN
    rsp->ExpCmdSN = NTOHL(*((unsigned *)(header+28)));         // ExpCmdSN
    rsp->MaxCmdSN = NTOHL(*((unsigned *)(header+32)));         // MaxCmdSN

    RETURN_NOT_EQUAL("Byte 0, bits 0-1", header[0]&0x00, 0x00, NO_CLEANUP, 1); 
    RETURN_NOT_EQUAL("Byte 1, bit 0", header[1]&0x80, 0x80, NO_CLEANUP, 1); 
    RETURN_NOT_EQUAL("Byte 3", header[3], 0, NO_CLEANUP, 1); 
    RETURN_NOT_EQUAL("Bytes 4-7", *((unsigned *)(header+4)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 8-11", *((unsigned *)(header+8)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 12-15", *((unsigned *)(header+12)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 20-23", *((unsigned *)(header+23)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 36-39", *((unsigned *)(header+36)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 40-43", *((unsigned *)(header+40)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 44-47", *((unsigned *)(header+44)), 0, 
                     NO_CLEANUP, 1);

    TRACE(TRACE_RPC, 0, "Response:  %u\n",   rsp->response);
    TRACE(TRACE_RPC, 0, "Tag:       0x%x\n", rsp->tag);
    TRACE(TRACE_RPC, 0, "StatSN:    %u\n",   rsp->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:  %u\n",   rsp->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:  %u\n",   rsp->MaxCmdSN);

    return 0;
}

/*
 * NOP-Out 
 */

int iscsi_nop_out_encap(unsigned char *header, ISCSI_NOP_OUT_T *cmd) {

    TRACE(TRACE_RPC, 0, "Immediate:    %i\n", cmd->immediate);
    TRACE(TRACE_RPC, 0, "Length:       %u\n", cmd->length);
    TRACE(TRACE_RPC, 0, "LUN:          %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Tag:          0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag: 0x%x\n", cmd->transfer_tag);
    TRACE(TRACE_RPC, 0, "CmdSN:        %u\n", cmd->CmdSN);
    TRACE(TRACE_RPC, 0, "ExpStatSN:    %u\n", cmd->ExpStatSN);

    memset(header, 0, ISCSI_HEADER_LEN);

    header[0] = ISCSI_NOP_OUT;                                 // Opcode
    if (cmd->immediate) header[0] |= 0x40;                     // Immediate bit
    header[1] |= 0x80;                                         // B1 b, Rsrv
    *((unsigned *)(header+4)) = HTONL(cmd->length&0x00ffffff); // Length 
    header[9] = cmd->lun&0xff;                                 // LUN
    *((unsigned *)(header+16)) = HTONL(cmd->tag);              // Tag
    *((unsigned *)(header+20)) = HTONL(cmd->transfer_tag);     // Target Tag 
    *((unsigned *)(header+24)) = HTONL(cmd->CmdSN);            // CmdSN
    *((unsigned *)(header+28)) = HTONL(cmd->ExpStatSN);        // ExpStatSN

    return 0;
}

int iscsi_nop_out_decap(unsigned char *header, ISCSI_NOP_OUT_T *cmd) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_NOP_OUT, 
                     NO_CLEANUP, 1);

    cmd->immediate = ((header[0]&0x40)==0x40);              // Immediate bit 
    cmd->length = NTOHL(*((unsigned *)(header+4)));         // Length
    cmd->lun = header[9];                                   // LUN
    cmd->tag = NTOHL(*((unsigned *)(header+16)));           // Tag
    cmd->transfer_tag = NTOHL(*((unsigned *)(header+20)));  // Target xfer Tag
    cmd->CmdSN = ISCSI_CMD_SN(header);                      // CmdSN
    cmd->ExpStatSN = NTOHL(*((unsigned *)(header+28)));     // ExpStatSN

    RETURN_NOT_EQUAL("Byte 1", header[1], 0x80, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 2", header[2], 0, NO_CLEANUP, 1); 
    RETURN_NOT_EQUAL("Byte 3", header[3], 0, NO_CLEANUP, 1); 
    RETURN_NOT_EQUAL("Byte 4", header[4], 0, NO_CLEANUP, 1); 
    RETURN_NOT_EQUAL("Byte 8", header[8], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 32-35", *((unsigned *)(header+32)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 36-39", *((unsigned *)(header+36)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 40-43", *((unsigned *)(header+40)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 44-47", *((unsigned *)(header+44)), 0, 
                     NO_CLEANUP, 1);

    TRACE(TRACE_RPC, 0, "Immediate:    %i\n", cmd->immediate);
    TRACE(TRACE_RPC, 0, "Length:       %u\n", cmd->length);
    TRACE(TRACE_RPC, 0, "LUN:          %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Tag:          0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag: 0x%x\n", cmd->transfer_tag);
    TRACE(TRACE_RPC, 0, "CmdSN:        %u\n", cmd->CmdSN);
    TRACE(TRACE_RPC, 0, "ExpStatSN:    %u\n", cmd->ExpStatSN);

    return 0;
}

/*
 * NOP-In 
 */

int iscsi_nop_in_encap(unsigned char *header, ISCSI_NOP_IN_T *cmd) {

    TRACE(TRACE_RPC, 0, "Length:       %u\n", cmd->length);
    TRACE(TRACE_RPC, 0, "LUN:          %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Tag:          0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag: 0x%x\n", cmd->transfer_tag);
    TRACE(TRACE_RPC, 0, "StatSN:       %u\n", cmd->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:     %u\n", cmd->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:     %u\n", cmd->MaxCmdSN);
  
    memset(header, 0, ISCSI_HEADER_LEN);
  
    header[0] = 0x00|ISCSI_NOP_IN;                             // Opcode 
    header[1] |= 0x80;                                         // Reserved
    *((unsigned *)(header+4)) = HTONL(cmd->length&0x00ffffff); // Length
    header[9] = cmd->lun&0xff;                                 // LUN
    *((unsigned *)(header+16)) = HTONL(cmd->tag);              // Tag
    *((unsigned *)(header+20)) = HTONL(cmd->transfer_tag);     // Target Tag
    *((unsigned *)(header+24)) = HTONL(cmd->StatSN);           // StatSN
    *((unsigned *)(header+28)) = HTONL(cmd->ExpCmdSN);         // ExpCmdSN
    *((unsigned *)(header+32)) = HTONL(cmd->MaxCmdSN);         // MaxCmdSN
  
    return 0;
}

int iscsi_nop_in_decap(unsigned char *header, ISCSI_NOP_IN_T *cmd) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_NOP_IN, 
                     NO_CLEANUP, 1);



    cmd->length = NTOHL(*((unsigned *)(header+4)));            // Length
    cmd->lun = header[9];                                      // LUN
    cmd->tag = NTOHL(*((unsigned *)(header+16)));              // Tag
    cmd->transfer_tag = NTOHL(*((unsigned *)(header+20)));     // Target Tag
    cmd->StatSN = NTOHL(*((unsigned *)(header+24)));           // StatSN 
    cmd->ExpCmdSN = NTOHL(*((unsigned *)(header+28)));         // ExpCmdSN
    cmd->MaxCmdSN = NTOHL(*((unsigned *)(header+32)));         // MaxCmdSN

    RETURN_NOT_EQUAL("Byte 0, bits 0-1", header[0]&0xc0, 0x00, NO_CLEANUP, 1); 
    RETURN_NOT_EQUAL("Byte 1", header[1], 0x80, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 2", header[2], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 3", header[3], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 4", header[4], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 8", header[8], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 36-39", *((unsigned *)(header+36)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 40-43", *((unsigned *)(header+40)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 44-47", *((unsigned *)(header+44)), 0, 
                     NO_CLEANUP, 1);

    TRACE(TRACE_RPC, 0, "Length:       %u\n", cmd->length);
    TRACE(TRACE_RPC, 0, "LUN:          %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Tag:          0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag: 0x%x\n", cmd->transfer_tag);
    TRACE(TRACE_RPC, 0, "StatSN:       %u\n", cmd->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:     %u\n", cmd->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:     %u\n", cmd->MaxCmdSN);
    return 0;
}

/*
 * Text Command
 */

int iscsi_text_cmd_encap(unsigned char *header, ISCSI_TEXT_CMD_T *cmd) {

    TRACE(TRACE_RPC, 0, "Immediate:    %i\n",   cmd->immediate);
    TRACE(TRACE_RPC, 0, "Final:        %i\n",   cmd->final);
    TRACE(TRACE_RPC, 0, "Continue:     %i\n",   cmd->cont);
    TRACE(TRACE_RPC, 0, "Length:       %u\n",   cmd->length);
    TRACE(TRACE_RPC, 0, "LUN:          %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Tag:          0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag: 0x%x\n", cmd->transfer_tag);
    TRACE(TRACE_RPC, 0, "CmdSN:        %u\n",   cmd->CmdSN);
    TRACE(TRACE_RPC, 0, "ExpStatSN:    %u\n",   cmd->ExpStatSN);

    memset(header, 0, ISCSI_HEADER_LEN);
  
    header[0] |= ISCSI_TEXT_CMD;                               // Opcode
    if (cmd->immediate) header[0] |= 0x40;                     // Immediate bit
    if (cmd->final) header[1] |= 0x80;                         // Final bit
    if (cmd->cont) header[1] |= 0x40;                          // Continue bit
    *((unsigned *)(header+4)) = HTONL(cmd->length&0x00ffffff); // Length
    header[9] = cmd->lun&0xff;                                 // LUN
    *((unsigned *)(header+16)) = HTONL(cmd->tag);              // Tag
    *((unsigned *)(header+20)) = HTONL(cmd->transfer_tag);     // Transfer Tag
    *((unsigned *)(header+24)) = HTONL(cmd->CmdSN);            // CmdSN
    *((unsigned *)(header+28)) = HTONL(cmd->ExpStatSN);        // ExpStatSN

    return 0;
}

int iscsi_text_cmd_decap(unsigned char *header, ISCSI_TEXT_CMD_T *cmd) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_TEXT_CMD, 
                     NO_CLEANUP, 1);

    cmd->immediate = ((header[0]&0x40)==0x40);                 // Immediate bit 
    cmd->final = ((header[1]&0x80)==0x80);                     // Final bit
    cmd->cont = ((header[1]&0x40)==0x40);                      // Continue bit
    cmd->length = NTOHL(*((unsigned *)(header+4)));            // Length
    cmd->lun = header[9];                                      // LUN
    cmd->tag = NTOHL(*((unsigned *)(header+16)));              // Tag
    cmd->transfer_tag = NTOHL(*((unsigned *)(header+20)));     // Transfer Tag
    cmd->CmdSN = ISCSI_CMD_SN(header);                         // CmdSN
    cmd->ExpStatSN = NTOHL(*((unsigned *)(header+28)));        // ExpStatSN

    RETURN_NOT_EQUAL("Byte 1, Bits 2-7", header[1]&0x00, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 2", header[2], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 3", header[3], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 4", header[4], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 8-11", *((unsigned *)(header+8)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 12-15", *((unsigned *)(header+12)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 32-35", *((unsigned *)(header+32)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 36-39", *((unsigned *)(header+36)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 40-43", *((unsigned *)(header+40)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 44-47", *((unsigned *)(header+44)), 0, 
                     NO_CLEANUP, 1);

    TRACE(TRACE_RPC, 0, "Immediate:    %i\n",   cmd->immediate);
    TRACE(TRACE_RPC, 0, "Final:        %i\n",   cmd->final);
    TRACE(TRACE_RPC, 0, "Continue:     %i\n",   cmd->cont);
    TRACE(TRACE_RPC, 0, "Length:       %u\n",   cmd->length);
    TRACE(TRACE_RPC, 0, "LUN:          %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Tag:          0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag: 0x%x\n", cmd->transfer_tag);
    TRACE(TRACE_RPC, 0, "CmdSN:        %u\n",   cmd->CmdSN);
    TRACE(TRACE_RPC, 0, "ExpStatSN:    %u\n",   cmd->ExpStatSN);

    return 0;
}

/*
 * Text Response
 */

int iscsi_text_rsp_encap(unsigned char *header, ISCSI_TEXT_RSP_T *rsp) {

    TRACE(TRACE_RPC, 0, "Final:        %i\n",   rsp->final);
    TRACE(TRACE_RPC, 0, "Continue:     %i\n",   rsp->cont);
    TRACE(TRACE_RPC, 0, "Length:       %u\n",   rsp->length);
    TRACE(TRACE_RPC, 0, "LUN:          %"PRIu64"\n",   rsp->lun);
    TRACE(TRACE_RPC, 0, "Tag:          0x%x\n", rsp->tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag: 0x%x\n", rsp->transfer_tag);
    TRACE(TRACE_RPC, 0, "StatSN:       %u\n",   rsp->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:     %u\n",   rsp->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:     %u\n",   rsp->MaxCmdSN);

    memset(header, 0, ISCSI_HEADER_LEN);
    header[0] |= 0x00|ISCSI_TEXT_RSP;                          // Opcode
    if (rsp->final) header[1] |= 0x80;                         // Final bit
    if (rsp->cont) header[1] |= 0x40;                          // Continue
    *((unsigned *)(header+4)) = HTONL(rsp->length&0x00ffffff); // Length
    header[9] = rsp->lun&0xff;                                 // LUN
    *((unsigned *)(header+16)) = HTONL(rsp->tag);              // Tag
    *((unsigned *)(header+20)) = HTONL(rsp->transfer_tag);     // Transfer Tag
    *((unsigned *)(header+24)) = HTONL(rsp->StatSN);           // StatSN
    *((unsigned *)(header+28)) = HTONL(rsp->ExpCmdSN);         // ExpCmdSN
    *((unsigned *)(header+32)) = HTONL(rsp->MaxCmdSN);         // MaxCmdSN

    return 0;
}

int iscsi_text_rsp_decap(unsigned char *header, ISCSI_TEXT_RSP_T *rsp) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_TEXT_RSP, 
                     NO_CLEANUP, 1);

    rsp->final = ((header[1]&0x80)==0x80);                     // Final bit 
    rsp->cont = ((header[1]&0x40)==0x40);                      // Continue bit
    rsp->length = NTOHL(*((unsigned *)(header+4)));            // Length
    rsp->lun = header[9];                                      // LUN
    rsp->tag = NTOHL(*((unsigned *)(header+16)));              // Tag
    rsp->transfer_tag = NTOHL(*((unsigned *)(header+20)));     // Transfer Tag
    rsp->StatSN = NTOHL(*((unsigned *)(header+24)));           // StatSN
    rsp->ExpCmdSN = NTOHL(*((unsigned *)(header+28)));         // ExpCmdSN
    rsp->MaxCmdSN = NTOHL(*((unsigned *)(header+32)));         // MaxCmdSN

    RETURN_NOT_EQUAL("Byte 1, Bits 2-7", header[1]&0x3f, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 2", header[2], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 3", header[3], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 4", header[4], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 8-11", *((unsigned *)(header+8)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 12-15", *((unsigned *)(header+12)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 36-39", *((unsigned *)(header+36)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 40-43", *((unsigned *)(header+40)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 44-47", *((unsigned *)(header+44)), 0, 
                     NO_CLEANUP, 1);

    TRACE(TRACE_RPC, 0, "Final:        %i\n",   rsp->final);
    TRACE(TRACE_RPC, 0, "Continue:     %i\n",   rsp->cont);
    TRACE(TRACE_RPC, 0, "Length:       %u\n",   rsp->length);
    TRACE(TRACE_RPC, 0, "LUN:          %"PRIu64"\n", rsp->lun);
    TRACE(TRACE_RPC, 0, "Tag:          0x%x\n", rsp->tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag: 0x%x\n", rsp->transfer_tag);
    TRACE(TRACE_RPC, 0, "StatSN:       %u\n",   rsp->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:     %u\n",   rsp->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:     %u\n",   rsp->MaxCmdSN);

    return 0;
}

/*
 * Login Command
 */

int iscsi_login_cmd_encap(unsigned char *header, ISCSI_LOGIN_CMD_T *cmd) {

    cmd->isid++;

    TRACE(TRACE_RPC, 0, "Transit:           %i\n",   cmd->transit);
    TRACE(TRACE_RPC, 0, "Continue:          %i\n",   cmd->cont);
    TRACE(TRACE_RPC, 0, "CSG:               %u\n",   cmd->csg);
    TRACE(TRACE_RPC, 0, "NSG:               %u\n",   cmd->nsg);
    TRACE(TRACE_RPC, 0, "Version_min:       %u\n",   cmd->version_min);
    TRACE(TRACE_RPC, 0, "Version_max:       %u\n",   cmd->version_max);
    TRACE(TRACE_RPC, 0, "TotalAHSLength:    %u\n",   cmd->AHSlength);
    TRACE(TRACE_RPC, 0, "DataSegmentLength: %u\n",   cmd->length);
    TRACE(TRACE_RPC, 0, "ISID:              %"PRIx64"\n", cmd->isid);
    TRACE(TRACE_RPC, 0, "TSIH:              %x\n",   cmd->tsih);
    TRACE(TRACE_RPC, 0, "Task Tag:          0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "CID:               %hu\n",  cmd->cid);
    TRACE(TRACE_RPC, 0, "CmdSN:             %u\n",   cmd->CmdSN);
    TRACE(TRACE_RPC, 0, "ExpStatSN:         %u\n",   cmd->ExpStatSN);

    memset(header, 0, ISCSI_HEADER_LEN);
  
    header[0] |= 0x40|ISCSI_LOGIN_CMD;                         // Opcode 
    if (cmd->transit) header[1]|=0x80;                         // Transit
    if (cmd->cont) header[1]|=0x40;                            // Continue
    header[1] |= ((cmd->csg)<<2)&0x0c;                         // CSG
    header[1] |= (cmd->nsg)&0x03;                              // NSG
    header[2] = cmd->version_max;                              // Version-Max 
    header[3] = cmd->version_min;                              // Version-Min 
    *((uint64_t*)(header+6)) = HTONLL(cmd->isid);              // ISID
    *((unsigned *)(header+4)) = HTONL(cmd->length&0x00ffffff); // Length
    header[4] = cmd->AHSlength;                                // TotalAHSLength
    *((unsigned short *)(header+14)) = HTONS(cmd->tsih);       // TSIH 
    *((unsigned *)(header+16)) = HTONL(cmd->tag);              // Task Tag
    *((unsigned short *)(header+20)) = HTONS(cmd->cid);        // CID
    *((unsigned *)(header+24)) = HTONL(cmd->CmdSN);            // CmdSN
    *((unsigned *)(header+28)) = HTONL(cmd->ExpStatSN);        // ExpStatSN

    cmd->isid--;

    return 0;
}

int iscsi_login_cmd_decap(unsigned char *header, ISCSI_LOGIN_CMD_T *cmd) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_LOGIN_CMD, 
                     NO_CLEANUP, 1);

    cmd->transit = (header[1]&0x80)?1:0;                         // Transit
    cmd->cont = (header[1]&0x40)?1:0;                            // Continue
    cmd->csg = (header[1]&0x0c)>>2;                              // CSG
    cmd->nsg = header[1]&0x03;                                   // NSG
    cmd->version_max = header[2];                                // Version-Max 
    cmd->version_min = header[3];                                // Version-Min 
    cmd->AHSlength = header[4];                                  // TotalAHSLen
    cmd->length = NTOHL(*((unsigned *)(header+4)))&0x00ffffffff; // Length
    cmd->isid = NTOHLL(*((uint64_t *)(header+8)))>>16;           // ISID
    cmd->tsih = NTOHS(*((unsigned short *)(header+14)));         // TSIH
    cmd->tag = NTOHL(*((unsigned *)(header+16)));                // Task Tag
    cmd->cid = NTOHS(*((unsigned short *)(header+20)));          // CID
    cmd->CmdSN = ISCSI_CMD_SN(header);                           // CmdSN
    cmd->ExpStatSN = NTOHL(*((unsigned *)(header+28)));          // ExpStatSN

    TRACE(TRACE_RPC, 0, "Transit:           %i\n",   cmd->transit);
    TRACE(TRACE_RPC, 0, "Continue:          %i\n",   cmd->cont);
    TRACE(TRACE_RPC, 0, "CSG:               %u\n",   cmd->csg);
    TRACE(TRACE_RPC, 0, "NSG:               %u\n",   cmd->nsg);
    TRACE(TRACE_RPC, 0, "Version_min:       %u\n",   cmd->version_min);
    TRACE(TRACE_RPC, 0, "Version_max:       %u\n",   cmd->version_max);
    TRACE(TRACE_RPC, 0, "TotalAHSLength:    %u\n",   cmd->AHSlength);
    TRACE(TRACE_RPC, 0, "DataSegmentLength: %u\n",   cmd->length);
    TRACE(TRACE_RPC, 0, "ISID:              %"PRIx64"\n", cmd->isid);
    TRACE(TRACE_RPC, 0, "TSIH:              %x\n",   cmd->tsih);
    TRACE(TRACE_RPC, 0, "Task Tag:          0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "CID:               %hu\n",  cmd->cid);
    TRACE(TRACE_RPC, 0, "CmdSN:             %u\n",   cmd->CmdSN);
    TRACE(TRACE_RPC, 0, "ExpStatSN:         %u\n",   cmd->ExpStatSN);

    RETURN_NOT_EQUAL("Byte 1, bits 2-3", (header[1]&0x30)>>4, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 22-23", *((unsigned short *)(header+22)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 32-35", *((unsigned *)(header+32)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 36-39", *((unsigned *)(header+36)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 40-43", *((unsigned *)(header+40)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 44-47", *((unsigned *)(header+44)), 0,
                     NO_CLEANUP, 1);

    if (cmd->transit) {
        if (cmd->nsg <= cmd->csg) return -1;
        if ((cmd->nsg != 0) && (cmd->nsg != 1) && (cmd->nsg != 3)) return -1;
    }

    return 0;
}

/*
 * Login Response
 */

int iscsi_login_rsp_encap(unsigned char *header, ISCSI_LOGIN_RSP_T *rsp) {
    TRACE(TRACE_RPC, 0, "Transit:           %i\n",   rsp->transit);
    TRACE(TRACE_RPC, 0, "Continue:          %i\n",   rsp->cont);
    TRACE(TRACE_RPC, 0, "CSG:               %u\n",   rsp->csg);
    TRACE(TRACE_RPC, 0, "NSG:               %u\n",   rsp->nsg);
    TRACE(TRACE_RPC, 0, "Version_max:       %u\n",   rsp->version_max);
    TRACE(TRACE_RPC, 0, "Version_active:    %u\n",   rsp->version_active);
    TRACE(TRACE_RPC, 0, "TotalAHSLength:    %u\n",   rsp->AHSlength);
    TRACE(TRACE_RPC, 0, "DataSegmentLength: %u\n",   rsp->length);
    TRACE(TRACE_RPC, 0, "ISID:              0x%"PRIx64"\n", rsp->isid);
    TRACE(TRACE_RPC, 0, "TSIH:              0x%x\n",   rsp->tsih);
    TRACE(TRACE_RPC, 0, "Task Tag:          0x%x\n", rsp->tag);
    TRACE(TRACE_RPC, 0, "StatSN:            %u\n",   rsp->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:          %u\n",   rsp->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:          %u\n",   rsp->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "Status-Class:      %u\n",   rsp->status_class);
    TRACE(TRACE_RPC, 0, "Status-Detail:     %u\n",   rsp->status_detail);

    memset(header, 0, ISCSI_HEADER_LEN); 

    header[0] |= 0x00|ISCSI_LOGIN_RSP;                         // Opcode 
    if (rsp->transit) header[1] |= 0x80;                       // Transit 
    if (rsp->cont) header[1] |= 0x40;                          // Continue
    header[1] |= ((rsp->csg)<<2)&0x0c;                         // CSG
    if (rsp->transit) header[1] |= (rsp->nsg)&0x03;            // NSG
    header[2] = rsp->version_max;                              // Version-max
    header[3] = rsp->version_active;                           // Version-active

    /* some platforms must be word aligned, so we can't just HTONLL */
#if (__BYTE_ORDER == __BIG_ENDIAN)
    memcpy(header+6, &rsp->isid, sizeof(uint64_t));
#else
    header[6] = 0;
    header[7] = 0;
    header[8] = ((unsigned char *)&(rsp->isid))[5];
    header[9] = ((unsigned char *)&(rsp->isid))[4];
    header[10] = ((unsigned char *)&(rsp->isid))[3];
    header[11] = ((unsigned char *)&(rsp->isid))[2];
    header[12] = ((unsigned char *)&(rsp->isid))[1];
    header[13] = ((unsigned char *)&(rsp->isid))[0];
#endif

    *((unsigned *)(header+4)) = HTONL(rsp->length&0x00ffffff); // Length
    header[4] = rsp->AHSlength;                                // Totalahslength
    *((unsigned short *)(header+14)) = HTONS(rsp->tsih);       // TSIH
    *((unsigned *)(header+16)) = HTONL(rsp->tag);              // Tag 
    *((unsigned *)(header+24)) = HTONL(rsp->StatSN);           // StatRn
    *((unsigned *)(header+28)) = HTONL(rsp->ExpCmdSN);         // ExpCmdSN
    *((unsigned *)(header+32)) = HTONL(rsp->MaxCmdSN);         // MaxCmdSN
    header[36] = rsp->status_class;                            // Status-Class
    header[37] = rsp->status_detail;                           // Status-Detail

    return 0;
}

int iscsi_login_rsp_decap(unsigned char *header, ISCSI_LOGIN_RSP_T *rsp) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_LOGIN_RSP, 
                     NO_CLEANUP, 1);

    rsp->transit = (header[1]&0x80)>>7;                          // Transit 
    rsp->cont = (header[1]&0x40)>>6;                             // Continue
    rsp->csg = (header[1]&0x0c)>>2;                              // CSG 
    rsp->nsg = header[1]&0x03;                                   // NSG 
    rsp->version_max = header[2];                                // Version-max
    rsp->version_active = header[3];                             // Version-actv
    rsp->AHSlength = header[4];                                  // TotalAHSLen
    rsp->length = NTOHL(*((unsigned *)(header+4)))&0x00ffffffff; // Length
    rsp->isid = NTOHLL(*((uint64_t*)(header+8)))>>16;            // ISID
    rsp->tsih = NTOHS(*((unsigned short *)(header+14)));         // TSIH
    rsp->tag = NTOHL(*((unsigned *)(header+16)));                // Tag
    rsp->StatSN = NTOHL(*((unsigned *)(header+24)));             // StatSN
    rsp->ExpCmdSN = NTOHL(*((unsigned *)(header+28)));           // ExpCmdSN
    rsp->MaxCmdSN = NTOHL(*((unsigned *)(header+32)));           // MaxCmdSN 
    rsp->status_class = header[36];                              // Stat. Class
    rsp->status_detail = header[37];                             // Stat. Detail

    rsp->isid--;

    TRACE(TRACE_RPC, 0, "Transit:           %i\n",   rsp->transit);
    TRACE(TRACE_RPC, 0, "Continue:          %i\n",   rsp->cont);
    TRACE(TRACE_RPC, 0, "CSG:               %u\n",   rsp->csg);
    TRACE(TRACE_RPC, 0, "NSG:               %u\n",   rsp->nsg);
    TRACE(TRACE_RPC, 0, "Version_max:       %u\n",   rsp->version_max);
    TRACE(TRACE_RPC, 0, "Version_active:    %u\n",   rsp->version_active);
    TRACE(TRACE_RPC, 0, "TotalAHSLength:    %u\n",   rsp->AHSlength);
    TRACE(TRACE_RPC, 0, "DataSegmentLength: %u\n",   rsp->length);
    TRACE(TRACE_RPC, 0, "ISID:              0x%"PRIx64"\n", rsp->isid);
    TRACE(TRACE_RPC, 0, "TSIH:              0x%x\n",   rsp->tsih);
    TRACE(TRACE_RPC, 0, "Task Tag:          0x%x\n", rsp->tag);
    TRACE(TRACE_RPC, 0, "StatSN:            %u\n",   rsp->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:          %u\n",   rsp->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:          %u\n",   rsp->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "Status-Class:      %u\n",   rsp->status_class);
    TRACE(TRACE_RPC, 0, "Status-Detail:     %u\n",   rsp->status_detail);

    RETURN_NOT_EQUAL("Byte 1, bits 2-3", (header[1]&0x30)>>4, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 20-23", *((unsigned *)(header+20)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 38", header[38], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 39", header[39], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 40-43", *((unsigned *)(header+40)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 44-47", *((unsigned *)(header+44)), 0, 
                     NO_CLEANUP, 1);
  
    return 0;
}

/*
 * Logout Command
 */

int iscsi_logout_cmd_encap(unsigned char *header, ISCSI_LOGOUT_CMD_T *cmd) {

    TRACE(TRACE_RPC, 0, "Immediate: %i\n",    cmd->immediate);
    TRACE(TRACE_RPC, 0, "Reason:    %u\n",    cmd->reason);
    TRACE(TRACE_RPC, 0, "Task Tag:  0x%x\n",  cmd->tag);
    TRACE(TRACE_RPC, 0, "CID:       %hu\n",   cmd->cid);
    TRACE(TRACE_RPC, 0, "CmdSN:     %u\n",    cmd->CmdSN);
    TRACE(TRACE_RPC, 0, "ExpStatSN: %u\n",    cmd->ExpStatSN);

    memset(header, 0, ISCSI_HEADER_LEN);
  
    header[0] = ISCSI_LOGOUT_CMD;                        // Opcode
    if (cmd->immediate) header[0] |= 0x40;               // Immediate
    header[1] = cmd->reason|0x80;                        // Reason 
    *((unsigned *)(header+16)) = HTONL(cmd->tag);        // Tag
    *((unsigned short *)(header+20)) = HTONS(cmd->cid);  // CID
    *((unsigned *) (header+24)) = HTONL(cmd->CmdSN);     // CmdSN
    *((unsigned *) (header+28)) = HTONL(cmd->ExpStatSN); // ExpStatSN 

    return 0;
}

int iscsi_logout_cmd_decap(unsigned char *header, ISCSI_LOGOUT_CMD_T *cmd) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_LOGOUT_CMD, 
                     NO_CLEANUP, 1);

    cmd->immediate = (header[0]&0x40)?1:0;               // Immediate
    cmd->reason = header[1]&0x7f;                        // Reason
    cmd->tag = NTOHL(*((unsigned *)(header+16)));        // Tag
    cmd->cid = NTOHS(*((unsigned short *)(header+20)));  // CID
    cmd->CmdSN = ISCSI_CMD_SN(header);                   // CmdSN
    cmd->ExpStatSN = NTOHL(*((unsigned *)(header+28)));  // ExpStatSN

    TRACE(TRACE_RPC, 0, "Immediate: %i\n",   cmd->immediate);
    TRACE(TRACE_RPC, 0, "Reason:    %u\n",   cmd->reason);
    TRACE(TRACE_RPC, 0, "Task Tag:  0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "CID:       %hu\n",  cmd->cid);
    TRACE(TRACE_RPC, 0, "CmdSN:     %u\n",   cmd->CmdSN);
    TRACE(TRACE_RPC, 0, "ExpStatSN: %u\n",   cmd->ExpStatSN);

    RETURN_NOT_EQUAL("Byte 0 bit 0", header[0]>>7, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 1 bit 0", header[1]>>7, 1, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 2", header[2], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 3", header[3], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 4-7", *((unsigned *)(header+4)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 8-11", *((unsigned *)(header+8)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 12-15", *((unsigned *)(header+12)), 0, 
                     NO_CLEANUP, 1);

    if (*((unsigned *)(header+22))) {
        TRACE_WARN("***Bytes 22-23 are not ZERO***\n");
    }

    /* RETURN_NOT_EQUAL("Bytes 22-23", *((unsigned *)(header+22)), 0, 
       NO_CLEANUP, 1); */
    RETURN_NOT_EQUAL("Bytes 32-35", *((unsigned *)(header+32)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 36-39", *((unsigned *)(header+36)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 40-43", *((unsigned *)(header+40)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 44-47", *((unsigned *)(header+44)), 0, 
                     NO_CLEANUP, 1);

    return 0;
}

/*
 * Logout Response
 */

int iscsi_logout_rsp_encap(unsigned char *header, ISCSI_LOGOUT_RSP_T *rsp) {

    TRACE(TRACE_RPC, 0, "Response:    %u\n",    rsp->response);
    TRACE(TRACE_RPC, 0, "Length:      %u\n",    rsp->length);
    TRACE(TRACE_RPC, 0, "Task Tag:    0x%x\n",  rsp->tag);
    TRACE(TRACE_RPC, 0, "StatSN:      %u\n",    rsp->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:    %u\n",    rsp->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:    %u\n",    rsp->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "Time2Wait:   %hu\n",   rsp->Time2Wait);
    TRACE(TRACE_RPC, 0, "Time2Retain: %hu\n",   rsp->Time2Retain);

    memset(header, 0, ISCSI_HEADER_LEN);
  
    header[0] |= 0x00|ISCSI_LOGOUT_RSP;                    // Opcode 
    header[1] |= 0x80;                                     // Reserved 
    header[2] = rsp->response;                             // Response
    *((unsigned *)(header+4)) = HTONL(rsp->length);        // Length
    *((unsigned *)(header+16)) = HTONL(rsp->tag);          // Tag
    *((unsigned *)(header+24)) = HTONL(rsp->StatSN);       // StatSN
    *((unsigned *)(header+28)) = HTONL(rsp->ExpCmdSN);     // ExpCmdSN
    *((unsigned *)(header+32)) = HTONL(rsp->MaxCmdSN);     // MaxCmdSN
    *((unsigned *)(header+40)) = HTONS(rsp->Time2Wait);    // Time2Wait
    *((unsigned *)(header+42)) = HTONS(rsp->Time2Retain);  // Time2Retain

    return 0;
}

int iscsi_logout_rsp_decap(unsigned char *header, ISCSI_LOGOUT_RSP_T *rsp) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_LOGOUT_RSP, 
                     NO_CLEANUP, 1);
  
    rsp->response = header[2];                             // Response
    rsp->length = NTOHL(*((unsigned *)(header+4)));        // Length
    rsp->tag = NTOHL(*((unsigned *)(header+16)));          // Tag
    rsp->StatSN = NTOHL(*((unsigned *)(header+24)));       // StatSN
    rsp->ExpCmdSN = NTOHL(*((unsigned *)(header+28)));     // ExpCmdSN
    rsp->MaxCmdSN = NTOHL(*((unsigned *)(header+32)));     // MaxCmdSN 
    rsp->Time2Wait = NTOHS(*((unsigned *)(header+40)));    // Time2Wait
    rsp->Time2Retain = NTOHS(*((unsigned *)(header+42)));  // Time2Retain

    RETURN_NOT_EQUAL("Byte 0 bits 0-1", (header[0]&0x20), 0x20, 
                     NO_CLEANUP, -1); 
    RETURN_NOT_EQUAL("Byte 1 bit 0",  header[1]&0x80, 0x80, NO_CLEANUP, -1); 
    RETURN_NOT_EQUAL("Byte 3",  header[3], 0, NO_CLEANUP, -1);
    RETURN_NOT_EQUAL("Bytes 4-7", *((unsigned *)(header+4)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 8-11", *((unsigned *)(header+8)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 12-15", *((unsigned *)(header+12)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 20-23", *((unsigned *)(header+20)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 36-39", *((unsigned *)(header+36)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 44-47", *((unsigned *)(header+44)), 0, 
                     NO_CLEANUP, 1);

    TRACE(TRACE_RPC, 0, "Response:    %u\n",   rsp->response);
    TRACE(TRACE_RPC, 0, "Length:      %u\n",   rsp->length);
    TRACE(TRACE_RPC, 0, "Task Tag:    0x%x\n", rsp->tag);
    TRACE(TRACE_RPC, 0, "StatSN:      %u\n",   rsp->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:    %u\n",   rsp->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:    %u\n",   rsp->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "Time2Wait:   %hu\n",  rsp->Time2Wait);
    TRACE(TRACE_RPC, 0, "Time2Retain: %hu\n",  rsp->Time2Retain);

    return 0;
}

/*
 * SCSI Command
 */

int iscsi_scsi_cmd_encap(unsigned char *header, ISCSI_SCSI_CMD_T *cmd) {

    TRACE(TRACE_RPC, 0, "Immediate:            %i\n",   cmd->immediate);
    TRACE(TRACE_RPC, 0, "Final:                %i\n",   cmd->final);
    TRACE(TRACE_RPC, 0, "Input:                %i\n",   cmd->fromdev);
    TRACE(TRACE_RPC, 0, "Output:               %i\n",   cmd->todev);
    TRACE(TRACE_RPC, 0, "ATTR:                 %i\n",   cmd->attr);
    TRACE(TRACE_RPC, 0, "TotalAHSLength:       %u\n",   cmd->ahs_len);
    TRACE(TRACE_RPC, 0, "DataSegmentLength:    %u\n",   cmd->length);
    TRACE(TRACE_RPC, 0, "LUN:                  %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Task Tag:             0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Transfer Length:      %u\n",   cmd->trans_len);
    TRACE(TRACE_RPC, 0, "Bidi transfer Length: %u\n",   cmd->bidi_trans_len);
    TRACE(TRACE_RPC, 0, "CmdSN:                %u\n",   cmd->CmdSN);
    TRACE(TRACE_RPC, 0, "ExpStatSN:            %u\n",   cmd->ExpStatSN);
    TRACE(TRACE_RPC, 0, "CDB:                  0x%x\n", cmd->cdb[0]);

    memset(header, 0, ISCSI_HEADER_LEN);

    header[0] |= ISCSI_SCSI_CMD;                        // Opcode
    if (cmd->immediate) header[0] |= 0x40;              // Immediate
    if (cmd->final) header[1] |= 0x80;                  // Final
    if (cmd->fromdev) header[1] |= 0x40;                // Input bit
    if (cmd->todev) header[1] |= 0x20;                  // Output bit
    header[1] |= cmd->attr&0x07;                        // ATTR 
    *((unsigned *)(header+4)) = HTONL(cmd->length);     // DataSegmentLength
    header[4] = cmd->ahs_len;                           // TotalAHSLength 
    header[9] = cmd->lun&0xff;                          // LUN
    *((unsigned *)(header+16)) = HTONL(cmd->tag);       // Task Tag 
    *((unsigned *)(header+20)) = HTONL(cmd->trans_len); // Expected xfer Length
    *((unsigned *)(header+24)) = HTONL(cmd->CmdSN);     // CmdSN
    *((unsigned *)(header+28)) = HTONL(cmd->ExpStatSN); // ExpStatSN
    memcpy(header+32, cmd->cdb, 16);                    // CDB

    return 0;
}

int iscsi_scsi_cmd_decap(unsigned char *header, ISCSI_SCSI_CMD_T *cmd) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_SCSI_CMD, 
                     NO_CLEANUP, 1);

    cmd->immediate = (header[0]&0x40)?1:0;                     // Immediate
    cmd->final = (header[1]&0x80)?1:0;                         // Final
    cmd->fromdev = (header[1]&0x40)?1:0;                       // Input
    cmd->todev = (header[1]&0x20)?1:0;                         // Output
    cmd->attr = header[1]&0x07;                                // ATTR 
    cmd->ahs_len = header[4];                                  // TotalAHSLength
    cmd->length = NTOHL(*((unsigned *)(header+4)))&0x00ffffff; // DataSegmentLen
    cmd->lun = header[9];                                      // LUN
    cmd->tag = NTOHL(*((unsigned *)(header+16)));              // Task Tag
    cmd->trans_len = NTOHL(*((unsigned *)(header+20)));        // Exp Trans Len
    cmd->CmdSN = ISCSI_CMD_SN(header);                         // CmdSN
    cmd->ExpStatSN = NTOHL(*((unsigned *)(header+28)));        // ExpStatSN
    cmd->cdb = header+32;                                      // CDB

    RETURN_NOT_EQUAL("Byte 1, Bits 3-4", header[1]&0x18, 0,NO_CLEANUP, -1);
    RETURN_NOT_EQUAL("Byte 2", header[2], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 3", header[3], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 8", header[8], 0, NO_CLEANUP, 1);
 
    TRACE(TRACE_RPC, 0, "Immediate:         %i\n",   cmd->immediate);
    TRACE(TRACE_RPC, 0, "Final:             %i\n",   cmd->final);
    TRACE(TRACE_RPC, 0, "Input:             %i\n",   cmd->fromdev);
    TRACE(TRACE_RPC, 0, "Output:            %i\n",   cmd->todev);
    TRACE(TRACE_RPC, 0, "ATTR:              %i\n",   cmd->attr);
    TRACE(TRACE_RPC, 0, "TotalAHSLength:    %u\n",   cmd->ahs_len);
    TRACE(TRACE_RPC, 0, "DataSegmentLength: %u\n",   cmd->length);
    TRACE(TRACE_RPC, 0, "LUN:               %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Task Tag:          0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Transfer Length:   %u\n",   cmd->trans_len);
    TRACE(TRACE_RPC, 0, "CmdSN:             %u\n",   cmd->CmdSN);
    TRACE(TRACE_RPC, 0, "ExpStatSN:         %u\n",   cmd->ExpStatSN);
    TRACE(TRACE_RPC, 0, "CDB:               0x%x\n", cmd->cdb[0]);

    return 0;
}

/*
 * SCSI Response
 */

int iscsi_scsi_rsp_encap(unsigned char *header, ISCSI_SCSI_RSP_T *rsp) {

    TRACE(TRACE_RPC, 0, "Bidi Overflow:       %i\n",   rsp->bidi_overflow);
    TRACE(TRACE_RPC, 0, "Bidi Underflow:      %i\n",   rsp->bidi_underflow);
    TRACE(TRACE_RPC, 0, "Overflow:            %i\n",   rsp->overflow);
    TRACE(TRACE_RPC, 0, "Underflow:           %i\n",   rsp->underflow);
    TRACE(TRACE_RPC, 0, "iSCSI Response:      %u\n",   rsp->response);
    TRACE(TRACE_RPC, 0, "SCSI Status:         %u\n",   rsp->status);
    TRACE(TRACE_RPC, 0, "DataSegmentLength:   %u\n",   rsp->length);
    TRACE(TRACE_RPC, 0, "Task Tag:            0x%x\n", rsp->tag);
    TRACE(TRACE_RPC, 0, "StatSN:              %u\n",   rsp->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:            %u\n",   rsp->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:            %u\n",   rsp->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "ExpDataSN:           %u\n",   rsp->ExpDataSN);
    TRACE(TRACE_RPC, 0, "Bidi Residual Count: %u\n",   rsp->bidi_res_cnt);
    TRACE(TRACE_RPC, 0, "Residual Count:      %u\n",   rsp->basic_res_cnt);

    memset(header, 0, ISCSI_HEADER_LEN);

    header[0] |= 0x00|ISCSI_SCSI_RSP;                       // Opcode 
    header[1] |= 0x80;                                      // Byte 1 bit 7
    if (rsp->bidi_overflow) header[1] |= 0x10;              // Bidi overflow
    if (rsp->bidi_underflow) header[1] |= 0x08;             // Bidi underflow
    if (rsp->overflow) header[1] |= 0x04;                   // Overflow
    if (rsp->underflow) header[1] |= 0x02;                  // Underflow 
    header[2] = rsp->response;                              // iSCSI Response
    header[3] = rsp->status;                                // SCSI Status
    *((unsigned *)(header+4)) = HTONL(rsp->length);         // DataSegmentLength
    header[4] = rsp->ahs_len;                               // TotalAHSLength
    *((unsigned *)(header+16)) = HTONL(rsp->tag);           // Task Tag
    *((unsigned *)(header+24)) = HTONL(rsp->StatSN);        // StatSN
    *((unsigned *)(header+28)) = HTONL(rsp->ExpCmdSN);      // ExpCmdSN
    *((unsigned *)(header+32)) = HTONL(rsp->MaxCmdSN);      // MaxCmdSN
    *((unsigned *)(header+36)) = HTONL(rsp->ExpDataSN);     // ExpDataSN 
    *((unsigned *)(header+40)) = HTONL(rsp->bidi_res_cnt);  // Bidi Res Count
    *((unsigned *)(header+44)) = HTONL(rsp->basic_res_cnt); // Residual Count

    return 0;
}

int iscsi_scsi_rsp_decap(unsigned char *header, ISCSI_SCSI_RSP_T *rsp) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_SCSI_RSP, 
                     NO_CLEANUP, 1);

    rsp->bidi_overflow  = (header[1]&0x10)?1:0;             // Bidi overflow
    rsp->bidi_underflow = (header[1]&0x08)?1:0;             // Bidi underflow
    rsp->overflow       = (header[1]&0x04)?1:0;             // Overflow
    rsp->underflow      = (header[1]&0x02)?1:0;             // Underflow
    rsp->response = header[2];                              // iSCSI Response
    rsp->status  = header[3];                               // SCSI Status
    rsp->length = NTOHL(*((unsigned *)(header+4)));         // DataSegmentLeng
    rsp->ahs_len =  header[4];                              // TotalAHSLength
    rsp->tag = NTOHL(*((unsigned *)(header+16)));           // Task Tag 
    rsp->StatSN = NTOHL(*((unsigned *)(header+24)));        // StatSN 
    rsp->ExpCmdSN = NTOHL(*((unsigned *)(header+28)));      // ExpCmdSN 
    rsp->MaxCmdSN = NTOHL(*((unsigned *)(header+32)));      // MaxCmdSN 
    rsp->ExpDataSN = NTOHL(*((unsigned *)(header+36)));     // ExpDataSN 
    rsp->bidi_res_cnt = NTOHL(*((unsigned *)(header+40)));  // Bidi Res Count 
    rsp->basic_res_cnt = NTOHL(*((unsigned *)(header+44))); // Residual Count
  
    RETURN_NOT_EQUAL("Byte 0 bits 0-1", header[0]&0x00, 0x00, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 1 bit 0", header[1]&0x80, 0x80, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("bidi_res_cnt", rsp->bidi_res_cnt, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("bidi_overflow", rsp->bidi_overflow, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("bidi_underflow", rsp->bidi_underflow, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("overflow", rsp->overflow, 0, NO_CLEANUP, 1);

    TRACE(TRACE_RPC, 0, "Bidi Overflow:       %i\n",   rsp->bidi_overflow);
    TRACE(TRACE_RPC, 0, "Bidi Underflow:      %i\n",   rsp->bidi_underflow);
    TRACE(TRACE_RPC, 0, "Overflow:            %i\n",   rsp->overflow);
    TRACE(TRACE_RPC, 0, "Underflow:           %i\n",   rsp->underflow);
    TRACE(TRACE_RPC, 0, "iSCSI Response:      %u\n",   rsp->response);
    TRACE(TRACE_RPC, 0, "SCSI Status:         %u\n",   rsp->status);
    TRACE(TRACE_RPC, 0, "DataSegmentLength:   %u\n",   rsp->length);
    TRACE(TRACE_RPC, 0, "Task Tag:            0x%x\n", rsp->tag);
    TRACE(TRACE_RPC, 0, "Residual Count:      %u\n",   rsp->basic_res_cnt);
    TRACE(TRACE_RPC, 0, "StatSN:              %u\n",   rsp->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:            %u\n",   rsp->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:            %u\n",   rsp->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "ExpDataSN:           %u\n",   rsp->ExpDataSN);
    TRACE(TRACE_RPC, 0, "Bidi Residual Count: %u\n",   rsp->bidi_res_cnt);

    return 0;
}

/*
 * Ready To Transfer
 */

int iscsi_r2t_encap(unsigned char *header, ISCSI_R2T_T *cmd) {

    TRACE(TRACE_RPC, 0, "TotalAHSLength:    %u\n", cmd->AHSlength);
    TRACE(TRACE_RPC, 0, "LUN:          %"PRIu64"\n",    cmd->lun);
    TRACE(TRACE_RPC, 0, "Tag:          0x%x\n",    cmd->tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag: 0x%x\n",    cmd->transfer_tag);
    TRACE(TRACE_RPC, 0, "StatSN:       %u\n",      cmd->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:     %u\n",      cmd->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:     %u\n",      cmd->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "R2TSN:        %u\n",      cmd->R2TSN);
    TRACE(TRACE_RPC, 0, "Offset:       %u\n",      cmd->offset);
    TRACE(TRACE_RPC, 0, "Length:       %u\n",      cmd->length);

    memset(header, 0, ISCSI_HEADER_LEN);
  
    header[0] |= 0x00|ISCSI_R2T;                                  // Opcode 
    header[1] |= 0x80;
    *((unsigned *)(header+4)) = HTONL(cmd->AHSlength&0x00ffffff); // AHSLength
    header[9] = cmd->lun&0xff;                                    // LUN
    *((unsigned *)(header+16)) = HTONL(cmd->tag);                 // Tag
    *((unsigned *)(header+20)) = HTONL(cmd->transfer_tag);        // Trans Tag
    *((unsigned *)(header+24)) = HTONL(cmd->StatSN);              // StatSN 
    *((unsigned *)(header+28)) = HTONL(cmd->ExpCmdSN);            // ExpCmdSN
    *((unsigned *)(header+32)) = HTONL(cmd->MaxCmdSN);            // MaxCmdSN
    *((unsigned *)(header+36)) = HTONL(cmd->R2TSN);               // R2TSN
    *((unsigned *)(header+40)) = HTONL(cmd->offset);              // Buff Offset
    *((unsigned *)(header+44)) = HTONL(cmd->length);              // Trans Len

    return 0;
}

int iscsi_r2t_decap(unsigned char *header, ISCSI_R2T_T *cmd) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_R2T, NO_CLEANUP, 1);

    cmd->AHSlength = NTOHL(*((unsigned *)(header+4)));         // TotalAHSLength
    cmd->lun = header[9];                                      // LUN
    cmd->tag = NTOHL(*((unsigned *)(header+16)));              // Tag
    cmd->transfer_tag = NTOHL(*((unsigned *)(header+20)));     // Transfer Tag
    cmd->StatSN = NTOHL(*((unsigned *)(header+24)));           // StatSN
    cmd->ExpCmdSN = NTOHL(*((unsigned *)(header+28)));         // ExpCmdSN 
    cmd->MaxCmdSN = NTOHL(*((unsigned *)(header+32)));         // MaxCmdSN 
    cmd->R2TSN = NTOHL(*((unsigned *)(header+36)));            // R2TSN
    cmd->offset = NTOHL(*((unsigned *)(header+40)));           // Offset
    cmd->length = NTOHL(*((unsigned *)(header+44)));           // Trans Length

    RETURN_NOT_EQUAL("Byte 1, Bits 1-7", header[1]&0x7f, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 2", header[2], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 3", header[3], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 4-7", *((unsigned *)(header+4)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 8-11", *((unsigned *)(header+8)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 12-15", *((unsigned *)(header+12)), 0, 
                     NO_CLEANUP, 1);

    TRACE(TRACE_RPC, 0, "AHSLength:    %u\n",   cmd->AHSlength);
    TRACE(TRACE_RPC, 0, "LUN:          %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Tag:          0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag: 0x%x\n", cmd->transfer_tag);
    TRACE(TRACE_RPC, 0, "StatSN:       %u\n",   cmd->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:     %u\n",   cmd->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:     %u\n",   cmd->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "R2TSN:        %u\n",   cmd->R2TSN);
    TRACE(TRACE_RPC, 0, "Offset:       %u\n",   cmd->offset);
    TRACE(TRACE_RPC, 0, "Length:       %u\n",   cmd->length);
    return 0;
}

/*
 * SCSI Write Data
 */

int iscsi_write_data_encap(unsigned char *header, ISCSI_WRITE_DATA_T *cmd) {

    TRACE(TRACE_RPC, 0, "Final:              %u\n",   cmd->final);
    TRACE(TRACE_RPC, 0, "DataSegmentLength:  %u\n",   cmd->length);
    TRACE(TRACE_RPC, 0, "LUN:                %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Task Tag:           0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag:       0x%x\n", cmd->transfer_tag);
    TRACE(TRACE_RPC, 0, "ExpStatSN:          %u\n",   cmd->ExpStatSN);
    TRACE(TRACE_RPC, 0, "DataSN:             %u\n",   cmd->DataSN);
    TRACE(TRACE_RPC, 0, "Buffer Offset:      %u\n",   cmd->offset);

    memset(header, 0, ISCSI_HEADER_LEN);
    header[0] = 0x00|ISCSI_WRITE_DATA;                       // Opcode 
    if (cmd->final) header[1] |= 0x80;                       // Final
    *((unsigned *)(header+4)) = HTONL(cmd->length);          // Length
    header[9] = cmd->lun&0xff;                               // LUN
    *((unsigned *)(header+16)) = HTONL(cmd->tag);            // Tag
    *((unsigned *)(header+20)) = HTONL(cmd->transfer_tag);   // Transfer Tag
    *((unsigned *)(header+28)) = HTONL(cmd->ExpStatSN);      // ExpStatSN
    *((unsigned *)(header+36)) = HTONL(cmd->DataSN);         // DataSN
    *((unsigned *)(header+40)) = HTONL(cmd->offset);         // Buffer Offset

    return 0;
}

int iscsi_write_data_decap(unsigned char *header, ISCSI_WRITE_DATA_T *cmd) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_WRITE_DATA,
                     NO_CLEANUP, 1);

    cmd->final = (header[1]&0x80)?1:0;                         // Final
    cmd->length = NTOHL(*((unsigned *)(header+4)));            // Length
    cmd->lun = header[9];                                      // LUN
    cmd->tag = NTOHL(*((unsigned *)(header+16)));              // Tag
    cmd->transfer_tag = NTOHL(*((unsigned *)(header+20)));     // Transfer Tag
    cmd->ExpStatSN = NTOHL(*((unsigned *)(header+28)));        // ExpStatSN 
    cmd->DataSN = NTOHL(*((unsigned *)(header+36)));           // DataSN   
    cmd->offset = NTOHL(*((unsigned *)(header+40)));           // Buffer Offset
  
    RETURN_NOT_EQUAL("Byte 1, Bits 1-7", header[1]&0x7f, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 2", header[2], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 3", header[3], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 4", header[4], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 8", header[8], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 24-27", *((unsigned *)(header+24)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 32-35", *((unsigned *)(header+32)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 44-47", *((unsigned *)(header+44)), 0, 
                     NO_CLEANUP, 1);

    TRACE(TRACE_RPC, 0, "Final:              %u\n",   cmd->final);
    TRACE(TRACE_RPC, 0, "DataSegmentLength:  %u\n",   cmd->length);
    TRACE(TRACE_RPC, 0, "LUN:                %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Task Tag:           0x%x\n", cmd->tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag:       0x%x\n", cmd->transfer_tag);
    TRACE(TRACE_RPC, 0, "ExpStatSN:          %u\n",   cmd->ExpStatSN);
    TRACE(TRACE_RPC, 0, "DataSN:             %u\n",   cmd->DataSN);
    TRACE(TRACE_RPC, 0, "Buffer Offset:      %u\n",   cmd->offset);

    return 0;
}

/*
 * SCSI Read Data
 */

int iscsi_read_data_encap(unsigned char *header, ISCSI_READ_DATA_T *cmd) {

    TRACE(TRACE_RPC, 0, "Final:             %i\n",   cmd->final);
    TRACE(TRACE_RPC, 0, "Acknowledge:       %i\n",   cmd->ack);
    TRACE(TRACE_RPC, 0, "Overflow:          %i\n",   cmd->overflow);
    TRACE(TRACE_RPC, 0, "Underflow:         %i\n",   cmd->underflow);
    TRACE(TRACE_RPC, 0, "S_bit:             %i\n",   cmd->S_bit);
    TRACE(TRACE_RPC, 0, "Status:            %u\n",   cmd->status);
    TRACE(TRACE_RPC, 0, "DataSegmentLength: %u\n",   cmd->length);
    TRACE(TRACE_RPC, 0, "LUN:               %"PRIu64"\n", cmd->lun);
    TRACE(TRACE_RPC, 0, "Task Tag:          0x%x\n", cmd->task_tag);
    TRACE(TRACE_RPC, 0, "Transfer Tag:      0x%x\n", cmd->transfer_tag);
    TRACE(TRACE_RPC, 0, "StatSN:            %u\n",   cmd->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:          %u\n",   cmd->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:          %u\n",   cmd->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "DataSN:            %u\n",   cmd->DataSN);
    TRACE(TRACE_RPC, 0, "Buffer Offset      %u\n",   cmd->offset);
    TRACE(TRACE_RPC, 0, "Residual Count:    %u\n",   cmd->res_count);

    memset(header, 0, ISCSI_HEADER_LEN);

    header[0] = 0x00|ISCSI_READ_DATA;                        // Opcode 
    if (cmd->final) header[1] |= 0x80;                       // Final
    if (cmd->ack) header[1] |= 0x40;                         // ACK
    if (cmd->overflow) header[1] |= 0x04;                    // Overflow 
    if (cmd->underflow) header[1] |= 0x02;                   // Underflow
    if (cmd->S_bit) header[1] |= 0x01;                       // S Bit
    if (cmd->S_bit)
        header[3] = cmd->status;                             // Status 
    *((unsigned *)(header+4)) = HTONL(cmd->length);          // Length
    header[9] = cmd->lun&0xff;                               // LUN
    *((unsigned *)(header+16)) = HTONL(cmd->task_tag);       // Task Tag
    *((unsigned *)(header+20)) = HTONL(cmd->transfer_tag);   // Transfer Tag
    if (cmd->S_bit)
        *((unsigned *)(header+24)) = HTONL(cmd->StatSN);     // StatSN
    *((unsigned *)(header+28)) = HTONL(cmd->ExpCmdSN);       // ExpCmdSN 
    *((unsigned *)(header+32)) = HTONL(cmd->MaxCmdSN);       // MaxCmdSN 
    *((unsigned *)(header+36)) = HTONL(cmd->DataSN);         // DataSN 
    *((unsigned *)(header+40)) = HTONL(cmd->offset);         // Buffer Offset
    if (cmd->S_bit)
        *((unsigned *)(header+44)) = HTONL(cmd->res_count);  // Residual Count 

    return 0;
}

int iscsi_read_data_decap(unsigned char *header, ISCSI_READ_DATA_T *cmd) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_READ_DATA, 
                     NO_CLEANUP, 1);

    cmd->final = (header[1]&0x80)?1:0;                       // Final
    cmd->ack = (header[1]&0x40)?1:0;                         // Acknowledge
    cmd->overflow = (header[1]&0x04)?1:0;                    // Overflow 
    cmd->underflow = (header[1]&0x02)?1:0;                   // Underflow 
    cmd->S_bit = (header[1]&0x01)?1:0;                       // S Bit 
    cmd->status = header[3];                                 // Status
    cmd->length = NTOHL(*((unsigned *)(header+4)));          // Length
    cmd->lun = header[9];                                    // LUN 
    cmd->task_tag = NTOHL(*((unsigned *)(header+16)));       // Task Tag
    cmd->transfer_tag = NTOHL(*((unsigned *)(header+20)));   // Transfer Tag 
    cmd->StatSN = NTOHL(*((unsigned *)(header+24)));         // StatSN 
    cmd->ExpCmdSN = NTOHL(*((unsigned *)(header+28)));       // ExpCmdSN 
    cmd->MaxCmdSN = NTOHL(*((unsigned *)(header+32)));       // MaxCmdSN 
    cmd->DataSN = NTOHL(*((unsigned *)(header+36)));         // DataSN 
    cmd->offset = NTOHL(*((unsigned *)(header+40)));         // Buffer Offset
    cmd->res_count = NTOHL( *((unsigned *)(header+44)));     // Residual Count 

    TRACE(TRACE_RPC, 0, "Final:             %i\n",   cmd->final);
    TRACE(TRACE_RPC, 0, "Acknowledge:       %i\n",   cmd->ack);
    TRACE(TRACE_RPC, 0, "Overflow:          %i\n",   cmd->overflow);
    TRACE(TRACE_RPC, 0, "Underflow:         %i\n",   cmd->underflow);
    TRACE(TRACE_RPC, 0, "S_bit:             %i\n",   cmd->S_bit);
    TRACE(TRACE_RPC, 0, "Status:            %u\n",   cmd->status);
    TRACE(TRACE_RPC, 0, "DataSegmentLength: %u\n",   cmd->length);
    TRACE(TRACE_RPC, 0, "Task Tag:          0x%x\n", cmd->task_tag);
    TRACE(TRACE_RPC, 0, "Residual Count:    %u\n",   cmd->res_count);
    TRACE(TRACE_RPC, 0, "StatSN:            %u\n",   cmd->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:          %u\n",   cmd->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:          %u\n",   cmd->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "DataSN:            %u\n",   cmd->DataSN);
    TRACE(TRACE_RPC, 0, "Buffer Offset      %u\n",   cmd->offset);

    RETURN_NOT_EQUAL("Byte 0, Bits 0-1", header[0]&0xc0, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 1, Bits 2-4", header[1]&0x38, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 2", header[2], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 4", header[4], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 8-11", *((unsigned *)(header+8)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 12-15", *((unsigned *)(header+12)), 0, 
                     NO_CLEANUP, 1);
    WARN_NOT_EQUAL("Underflow bit", cmd->underflow, 0);
    WARN_NOT_EQUAL("Residual count", cmd->res_count, 0);

    return 0;
}

/*
 * Reject
 */

int iscsi_reject_encap(unsigned char *header, ISCSI_REJECT_T *cmd) {

    TRACE(TRACE_RPC, 0, "Reason:   %u\n", cmd->reason);
    TRACE(TRACE_RPC, 0, "Length:   %u\n", cmd->length);
    TRACE(TRACE_RPC, 0, "StatSN:   %u\n", cmd->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN: %u\n", cmd->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN: %u\n", cmd->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "DataSN:   %u\n", cmd->DataSN);

    memset(header, 0, ISCSI_HEADER_LEN);

    header[0] |= 0x00|ISCSI_REJECT;                           // Opcode 
    header[1] |= 0x80;
    header[2] = cmd->reason;                                  // Reason
    *((unsigned *)(header+4)) = HTONL(cmd->length);           // Length 
    *((unsigned *)(header+24)) = HTONL(cmd->StatSN);          // StatSN
    *((unsigned *)(header+28)) = HTONL(cmd->ExpCmdSN);        // ExpCmdSN
    *((unsigned *)(header+32)) = HTONL(cmd->MaxCmdSN);        // MaxCmdSN
    *((unsigned *)(header+36)) = HTONL(cmd->DataSN);          // DataSN

    return 0;
}

int iscsi_reject_decap(unsigned char *header, ISCSI_REJECT_T *cmd) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_REJECT, 
                     NO_CLEANUP, 1);

    cmd->reason = header[2];                                 // Reason
    cmd->length = NTOHL(*((unsigned *)(header+4)));          // Length
    cmd->StatSN = NTOHL(*((unsigned *)(header+24)));         // StatSN
    cmd->ExpCmdSN = NTOHL(*((unsigned *)(header+28)));       // ExpCmdSN
    cmd->MaxCmdSN = NTOHL(*((unsigned *)(header+32)));       // MaxCmdSN
    cmd->DataSN = NTOHL(*((unsigned *)(header+36)));         // DataSN

    RETURN_NOT_EQUAL("Byte 0, Bits 0-1", header[0]&0xc0, 0, NO_CLEANUP, 1); 
    RETURN_NOT_EQUAL("Byte 1, Bits 1-7", header[1]&0x7f, 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 3", header[3], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Byte 4", header[4], 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 8-11", *((unsigned *)(header+8)), 0, NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 12-15", *((unsigned *)(header+12)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 20-23", *((unsigned *)(header+20)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 40-43", *((unsigned *)(header+40)), 0, 
                     NO_CLEANUP, 1);
    RETURN_NOT_EQUAL("Bytes 44-47", *((unsigned *)(header+44)), 0, 
                     NO_CLEANUP, 1);

    TRACE(TRACE_RPC, 0, "Reason:   %u\n", cmd->reason);
    TRACE(TRACE_RPC, 0, "Length:   %u\n", cmd->length);
    TRACE(TRACE_RPC, 0, "StatSN:   %u\n", cmd->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN: %u\n", cmd->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN: %u\n", cmd->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "DataSN:   %u\n", cmd->DataSN);
    return 0;
}

int iscsi_amsg_decap(unsigned char *header, ISCSI_AMSG_T *msg) {

    RETURN_NOT_EQUAL("Opcode", ISCSI_OPCODE(header), ISCSI_ASYNC, 
                     NO_CLEANUP, 1);

    msg->AHSlength = header[4];                             // TotalAHSLength
    msg->length = NTOHL(*((unsigned *)(header+4)));         // Length
    msg->lun = header[9];                                   // LUN 
    msg->StatSN = NTOHL(*((unsigned *)(header+24)));        // StatSN
    msg->ExpCmdSN = NTOHL(*((unsigned *)(header+28)));      // ExpCmdSN
    msg->MaxCmdSN = NTOHL(*((unsigned *)(header+32)));      // MaxCmdSN 
    msg->AsyncEvent = header[36];                           // Async Event
    msg->AsyncVCode = header[37];                           // Async Vendor Code

    TRACE(TRACE_RPC, 0, "TotalAHSLength:    %u\n",   msg->AHSlength);
    TRACE(TRACE_RPC, 0, "DataSegmentLength: %u\n",   msg->length);
    TRACE(TRACE_RPC, 0, "LUN:               %"PRIu64"\n", msg->lun);
    TRACE(TRACE_RPC, 0, "StatSN:            %u\n",   msg->StatSN);
    TRACE(TRACE_RPC, 0, "ExpCmdSN:          %u\n",   msg->ExpCmdSN);
    TRACE(TRACE_RPC, 0, "MaxCmdSN:          %u\n",   msg->MaxCmdSN);
    TRACE(TRACE_RPC, 0, "AsyncEvent:      %u\n",     msg->AsyncEvent);
    TRACE(TRACE_RPC, 0, "AsyncVCode:     %u\n",      msg->AsyncVCode);

    return 0;
}
