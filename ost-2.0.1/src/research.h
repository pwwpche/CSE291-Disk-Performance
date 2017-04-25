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

#define ISCSI_IOCTL_COUNTERS_GET          0x24
#define ISCSI_IOCTL_TARGET_EXIT           0x26
#define ISCSI_IOCTL_MARKERS_SET           0x33
#define ISCSI_IOCTL_MARKERS_CLEAR         0x34
#define ISCSI_IOCTL_CACHE_COUNTERS_GET    0x36
#define ISCSI_IOCTL_TRACING_ON            0x37
#define ISCSI_IOCTL_TRACING_OFF           0x38
#define ISCSI_IOCTL_TRACING_SET           0x39
#define ISCSI_IOCTL_CACHE_UTILIZATION_GET 0x40
#define ISCSI_IOCTL_QOS_POLICY_SET        0x41
#define ISCSI_IOCTL_CACHE_MODE_SET        0x42
#define ISCSI_IOCTL_TARGET_EXIT_WITH_ARGS 0x43
#define ISCSI_IOCTL_TARGET_CACHE_FLUSH    0x44
#define ISCSI_IOCTL_TARGET_CACHE_BYPASS   0x45
#define ISCSI_IOCTL_TARGET_CACHE_ENABLE   0x46

int synchronize_cache(uint64_t tid);
int device_exit(uint64_t tid);
int tracing_on(uint64_t tid, uint64_t lun);
int tracing_off(uint64_t tid, uint64_t lun);
int tracing_set(uint64_t tid, uint64_t lun, char *file);
int counters_get(uint64_t tid, uint64_t lun, char *data, int len);
int iscsi_ioctl(uint64_t tid, uint64_t lun, unsigned char op, void *ptr);
int t_counters_get(uint64_t tid, uint64_t lun, char *data);
int i_counters_get(uint64_t tid, uint64_t lun, char *data);
int read_cap(uint64_t tid, uint64_t lun, uint32_t *max_lba, 
             uint32_t *block_len);
int iscsi_issue_io(uint64_t tid, uint64_t lun, int writing, 
                   uint64_t offset, uint32_t len, unsigned char *buffer,
                   uint32_t blocklen);
int output_stats(char *start, char *stop, char *exe, void *ptr, int type);

#define OUTPUT_TYPE_STREAM 1
#define OUTPUT_TYPE_STRING 2
#define OUTPUT_TYPE_HEADER 4
#define OUTPUT_TYPE_DATA   8

#define ISCSI_COUNTERS_SIZE 8192

#if 0
#define SPRINTF_COUNTERS(START, STOP, VAR) \
    { \
        FILE *fptr; \
        char file1[256]; \
        char file2[256]; \
        char data[4096]; \
        char cmd[256]; \
        sprintf(file1, "/tmp/counters.%i", getpid());  \
        fptr = fopen(file1, "w+"); \
        fprintf(fptr, "%s\n", START); \
        fprintf(fptr, "%s\n", STOP); \
        fclose(fptr); \
        sprintf(file2, "/tmp/counters_diff.%i", getpid());  \
        sprintf(cmd, "cat %s | iscsi-diff-counters > %s", file1, file2); \
        system(cmd); \
        fptr = fopen(file2, "r"); \
        memset(data, 0, 4096); \
        fread(data, 4096, 1, fptr); \
        fclose(fptr); \
        data[strlen(data)-1] = '\0'; \
        sprintf(VAR, "%s", data); \
        sprintf(cmd, "rm -f %s\n", file1); system(cmd); \
        sprintf(cmd, "rm -f %s\n", file2); system(cmd); \
    }
#endif
