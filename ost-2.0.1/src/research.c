
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

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <inttypes.h>
#include "initiator.h"
#include "research.h"

int device_exit2(uint64_t tid) {
    unsigned char cdb[16];
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;

    memset(cdb, 0, 16);
    cdb[0] = 0x26; 
    memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T));
    args.cdb = cdb;
    if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, -1)!=0) {
        TRACE_ERROR("initiator_command() failed\n");
        return -1;
    }
    if (args.status) {
        TRACE(TRACE_SCSI, 1, "DEVICE_EXIT failed (status 0x%x)\n", args.status);
        return -1;
    }
    return 0;
}

int synchronize_cache(uint64_t tid) {
    unsigned char cdb[16];
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;

    memset(cdb, 0, 16);
    cdb[0] = 0x35; 
    memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T));
    args.cdb = cdb;
    if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, -1)!=0) {
        TRACE_ERROR("initiator_command() failed\n");
        return -1;
    }
    if (args.status) {
        TRACE(TRACE_SCSI, 1, 
              "SYNCHRONIZE_CACHE failed (status 0x%x)\n", args.status);
        return -1;
    }
    return 0;
}

int device_exit(uint64_t tid) {
    unsigned char cdb[16];
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;

    memset(cdb, 0, 16);
    cdb[0] = 0x26; 
    memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T));
    args.cdb = cdb;
    if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, -1)!=0) {
        TRACE_ERROR("initiator_command() failed\n");
        return -1;
    }
    if (args.status) {
        TRACE(TRACE_SCSI, 1, "DEVICE_EXIT failed (status 0x%x)\n", args.status);
        return -1;
    }
    return 0;
}

int counters_get2(uint64_t tid, uint64_t lun, char *data) {
    unsigned char cdb[16];
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;

    memset(cdb, 0, 16);
    cdb[0] = ISCSI_IOCTL_COUNTERS_GET; 
    cdb[1] = lun<<5;
    cdb[4] = 0;
    memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T));
    args.cdb = cdb;
    args.lun = lun;
    args.fromdev = 1;
    args.trans_len = ISCSI_COUNTERS_SIZE;
    args.recv_data = (unsigned char *) data;
    if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, -1)!=0) {
        TRACE_ERROR("initiator_command() failed\n");
        return -1;
    }
    if (args.status) {
        TRACE(TRACE_SCSI, 1, "counters_get() failed (status 0x%x)\n", 
              args.status);
        return -1;
    }
    return 0;
}

#define PARENT_READ    readpipe[0]
#define CHILD_WRITE    readpipe[1]
#define CHILD_READ     writepipe[0]
#define PARENT_WRITE   writepipe[1]

int output_stats(char *start, char *stop, char *exe, void *ptr, int type) {
    int writepipe[2];
    int readpipe[2];
    int rc;
    pid_t pid;
    char cmd[1024];
    char buff[ISCSI_COUNTERS_SIZE];
    FILE *stream = (FILE *) ptr;
    char *string = (char *) ptr;
    int completed;

    if (pipe(readpipe)<0 || pipe(writepipe)<0) {
        TRACE_ERROR("error creating pipe (errno %i)\n", errno);
        return -1;
    }
    if ((pid=fork())<0) {
        TRACE_ERROR("error forking\n");
        return -1;
    } else if (pid==0)  {
        close(PARENT_WRITE);
        close(PARENT_READ);
        dup2(CHILD_READ,  0);  close(CHILD_READ);
        dup2(CHILD_WRITE, 1);  close(CHILD_WRITE);
        sprintf(cmd, "iscsi-diff-counters");
        if (exe!=NULL) {
            sprintf(cmd+strlen(cmd), " | %s ", exe);
            if (type & OUTPUT_TYPE_HEADER) {
                sprintf(cmd+strlen(cmd), 
                        " | awk '{ORS=\" \";for(i=1;i<=NF;i++){if (i%%2==1) \
{printf(\"%%9s\", $i); if (i<(NF-1)) {printf(\" \")}}}}'");
            } else if (type & OUTPUT_TYPE_DATA) {
                sprintf(cmd+strlen(cmd), 
                        " | awk '{ORS=\" \";for(i=1;i<=NF;i++){if (i%%2!=1) \
{printf(\"%%9s\", $i); if (i<NF) {printf(\" \")}}}}'");
            } 
        }
        execlp("sh", "sh", "-c",  cmd, (char *) 0);
        TRACE_ERROR("executing (errno %i)\n", errno);
        return -1;
    } else {
        close(CHILD_READ);
        close(CHILD_WRITE);
        if (write(PARENT_WRITE, start, strlen(start))!=strlen(start)) {
            TRACE_ERROR("writing start counters to pipe\n");
            return -1;
        }
        if (write(PARENT_WRITE, stop, strlen(stop))!=strlen(stop)) {
            TRACE_ERROR("writing stop counters to pipe\n");
            return -1;
        }
        close(PARENT_WRITE);
        completed = 0;
        memset(buff, 0, ISCSI_COUNTERS_SIZE);
        while ((rc=read(PARENT_READ, buff+completed, ISCSI_COUNTERS_SIZE))>0) {
            completed += rc;
        }
        close(PARENT_READ);
        if (rc == -1) {
            TRACE_ERROR("error reading from pipe (rc %i errno %i)\n", 
                        rc, errno);
            exit(1);
        }

        if (type & OUTPUT_TYPE_STREAM) {
            fprintf(stream, "%s", buff);
        } else if (type & OUTPUT_TYPE_STRING) {
            sprintf(string, "%s", buff);
        } else {
            TRACE_ERROR("unknown output type (%i)\n", type);
            return -1;
        }
        if (waitpid(pid, &rc, 0)==-1) {
            TRACE_ERROR("waitpid() failed\n");
            return -1;
        }
    }
    return 0;
}

int i_counters_get(uint64_t tid, uint64_t lun, char *data) {
    int pfd[2], pid, rc;
    char arg1[16], arg2[16];
    int attempt = 0;

 try_again: attempt++;
    memset(data, 0, ISCSI_COUNTERS_SIZE);
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
        sprintf(arg1, "%"PRIu64"", tid);
        sprintf(arg2, "%"PRIu64"", lun);
        if ((rc=execlp("iscsi-initiator-counters", "iscsi-initiator-counters", 
                       arg1, arg2, (char *) 0))!=0) {
            TRACE_ERROR(
                        "error w/ iscsi-initiator-counters (rc %i, errno %i)\n",
                        rc, errno);
            return -1;
        }
        TRACE_ERROR(
                    "error w/ iscsi-initiator-counters (rc %i, errno %i)\n", 
                    rc, errno);
        return -1;
    }
    close(pfd[1]); 
    dup2(pfd[0], 0);
    if ((rc=read(pfd[0], data, ISCSI_COUNTERS_SIZE))<=0) {
        attempt++;
        if (attempt < 10) goto try_again;
        TRACE_ERROR("error reading pipe (attempt %i): rc %i errno %i\n", 
                    attempt, rc, errno);
        return -1;
    }
    close(pfd[0]);
    if (waitpid(pid, &rc, 0)==-1) {
        TRACE_ERROR("waitpid() failed\n");
        return -1;
    }
    if (rc!=0) {
        TRACE_ERROR("iscsi-initiator-counters failed (rc %i, errno %i)\n", 
                    rc, errno);
        return -1;
    }
    return 0;
}


int t_counters_get(uint64_t tid, uint64_t lun, char *data) {
    unsigned char cdb[16];
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;

    memset(cdb, 0, 16);
    cdb[0] = ISCSI_IOCTL_COUNTERS_GET; 
    cdb[1] = lun<<5;
    cdb[4] = 0;
    memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T));
    args.cdb = cdb;
    args.lun = lun;
    args.fromdev = 1;
    args.trans_len = ISCSI_COUNTERS_SIZE;
    args.recv_data = (unsigned char *) data;
    if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, -1)!=0) {
        TRACE_ERROR("initiator_command() failed\n");
        return 1;
    }
    if (args.status) {
        TRACE(TRACE_SCSI, 1, "counters_get() failed (status 0x%x)\n", 
              args.status);
        return 1;
    }
    return 0;
}

int read_cap(uint64_t tid, uint64_t lun, uint32_t *max_lba, 
             uint32_t *block_len) {
    unsigned char data[8], cdb[16];
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;

    memset(cdb, 0, 16); 
    cdb[0] = 0x25; 
    cdb[1] = lun<<5;  
    memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T)); 
    args.recv_data = data;
    args.fromdev = 1;
    args.lun = lun;
    args.trans_len = 8;
    args.cdb = cdb;
    if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, -1)!=0) {
        TRACE_ERROR("initiator_command() failed\n"); 
        return -1;
    }
    if (args.status) {
        TRACE_ERROR("READ_CAPACITY failed (status 0x%x)\n", args.status);
        return -1;
    } 
    *max_lba = NTOHL(*((unsigned *)(data)));
    *block_len = NTOHL(*((unsigned *)(data+4)));
    return 0;
}

int iscsi_issue_io(uint64_t tid, uint64_t lun, int writing, 
                   uint64_t offset, uint32_t len, 
                   unsigned char *buffer, uint32_t blocklen) {
    unsigned char cdb[16];
    ISCSI_SCSI_CMD_T iscsi_args;
    INITIATOR_CMD_T cmd;
 
    memset(cdb, 0, 16);
    memset(&iscsi_args, 0, sizeof(ISCSI_SCSI_CMD_T));
    memset(&cmd, 0, sizeof(INITIATOR_CMD_T));
    cdb[0] = writing?0x2a:0x28;
    *((unsigned *)(cdb+2)) = htonl(offset/blocklen);
    *((unsigned short *)(cdb+7)) = htons(len/blocklen);

    iscsi_args.lun = lun;
    iscsi_args.cdb = cdb;
    iscsi_args.fromdev = !writing;
    iscsi_args.todev = writing;
    iscsi_args.trans_len = len;
    iscsi_args.send_data = writing?buffer:NULL;
    iscsi_args.recv_data = (!writing)?buffer:NULL;
    if (initiator_command(ISCSI_SCSI_CMD, &iscsi_args, tid, &cmd, -1)!=0) {
        fprintf(stderr, "initiator_command() failed\n"); 
        return 1;
    }
    return 0;
}

int iscsi_ioctl(uint64_t tid, uint64_t lun, unsigned char op, void *ptr) {
    unsigned char cdb[16];
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;

    memset(cdb, 0, 16);
    cdb[0] = op; 
    cdb[1] = lun<<5;
    cdb[4] = 0;
    memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T));
    args.cdb = cdb;
    args.lun = lun;
    if (ptr!=NULL) {
        if (op == ISCSI_IOCTL_TRACING_SET) {
            args.todev = 1;
            args.trans_len = strlen((char *)ptr)+1;
            args.send_data = ptr;
        } else if (op == ISCSI_IOCTL_QOS_POLICY_SET) {
            args.todev = 1;
            args.trans_len = strlen((char *)ptr)+1;
            args.send_data = ptr;
        } else if (op == ISCSI_IOCTL_CACHE_MODE_SET) {
            args.todev = 1;
            args.trans_len = strlen((char *)ptr)+1;
            args.send_data = ptr;
        } else if (op == ISCSI_IOCTL_CACHE_COUNTERS_GET) {
            args.fromdev = 1;
            args.trans_len = ISCSI_COUNTERS_SIZE;
            args.recv_data = ptr;
        } else if (op == ISCSI_IOCTL_CACHE_UTILIZATION_GET) {
            args.fromdev = 1;
            args.trans_len = ISCSI_COUNTERS_SIZE;
            args.recv_data = ptr;
        } else if (op == ISCSI_IOCTL_TARGET_EXIT_WITH_ARGS) {
            args.todev = 1;
            args.trans_len = strlen((char *)ptr)+1;
            args.send_data = ptr;
        }
    }
    if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, 0)!=0) {
        TRACE_ERROR("initiator_command() failed\n");
        return -1;
    }
    if (args.status) {
        TRACE(TRACE_SCSI, 1, "iscsi_ioctl() failed (status 0x%x)\n", 
              args.status);
        return -1;
    }
    return 0;
}
