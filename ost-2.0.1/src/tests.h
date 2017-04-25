
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

#ifndef _SCSI_TEST_H
#define _SCSI_TEST_H

#include "initiator.h"

int inquiry(uint64_t tid, uint64_t lun, unsigned *device_type, 
            char *info, int timeout);
int read_capacity(uint64_t tid, uint64_t lun, unsigned *max_lba, 
                  unsigned *block_len, int timeout);
int read_or_write(uint64_t tid, uint64_t lun, unsigned lba, 
                  unsigned len, unsigned block_len,
                  unsigned char *data, int sg_len, int write, int timeout);
int test_all(uint64_t tid, uint64_t lun, int full, int samples, 
             int iters, int seed, FILE *stream, int skip, int warm, 
             int test, int replay);
int integrity_test(uint64_t tid, uint64_t lun, unsigned length, int sg_factor);
int stats_reset(uint64_t tid, uint64_t lun);
int nop_test(uint64_t tid, uint64_t lun, unsigned iters);
int latency_test(uint64_t tid, uint64_t lun, unsigned char op, 
                 unsigned iters, double *latency_usec);
int throughput_test(uint64_t tid, uint64_t lun, int samples,
                    int iters, int seed, FILE *stream, int skip, 
                    int osd, int warm, int test, int replay);
int nop_out(uint64_t tid, uint64_t lun, int ping, int length, char *data);
int write_read_test(uint64_t target, uint64_t lun, int type);
int scatter_gather_test(uint64_t target, uint64_t lun, unsigned char op);
#endif
