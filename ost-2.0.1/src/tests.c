

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <utime.h>
#include <scsi/scsi.h>
#include <unistd.h>
#include <netinet/in.h>
#include <inttypes.h>
#include "debug.h"
#include "iscsi.h"
#include "initiator.h"
#include "tests.h"
#include "osd.h"
#include "osd_ops.h"
#include "assert.h"
#include "math.h"
#include "research.h"

#define MIN_READ 0
#define MAX_READ 100
#define MIN_RAND 0
#define MAX_RAND 100
#define MIN_THINK 0
#define MAX_THINK 1000
#define MIN_DEPTH 1
#define MAX_DEPTH 32
#define MIN_REQUEST 1024
#define MAX_REQUEST 262144
#define MAX_SIZE 262144*1024*1ULL
#define MAX_OUTSTANDING 262144*16

typedef struct async_cmd_t {
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T scsi_cmd;
    osd_args_t osd_args;
    OSD_OPS_MEM_T osd_mem;
    ISCSI_WQ_T wq;
    unsigned char cdb[CONFIG_OSD_CDB_LEN];
    unsigned char data[MAX_REQUEST];
    int busy;
} ASYNC_CMD_T;


/*
 * Globals
 */

uint32_t g_GroupID;   /* used for async test */
uint64_t g_UserID;    /* used for async test */
uint32_t g_blocklen;  /* disk block size (returned on read capacity) */

/*
 * SCSI Command Tests
 */

int nop_out(uint64_t tid, uint64_t lun, int length, int ping, char *data) {
    INITIATOR_CMD_T cmd;
    ISCSI_NOP_OUT_T nop_cmd;

    RETURN_GREATER("length", length, 4096, NO_CLEANUP, -1);
    memset(&nop_cmd, 0, sizeof(ISCSI_NOP_OUT_T));
    nop_cmd.length = length;
    nop_cmd.data = (unsigned char *) data;
    nop_cmd.lun = lun;
    if (!ping) {
        nop_cmd.immediate = 1;
        nop_cmd.tag = 0xffffffff;
    }
    if (initiator_command(ISCSI_NOP_OUT, &nop_cmd, tid, &cmd, -1)!=0) {
        TRACE_ERROR("initiator_command() failed\n"); 
        return -1;
    }
    ISCSI_WQ_DESTROY(&cmd.wq);

    return 0;
}

int stats_reset(uint64_t tid, uint64_t lun) {
    unsigned char cdb[16];
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;

    memset(cdb, 0, 16);
    cdb[0] = 0x23; 
    cdb[1] = lun<<5;
    cdb[4] = 36;
    memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T));
    args.cdb = cdb;
    args.lun = lun;
    if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, -1)!=0) {
        TRACE_ERROR("initiator_command() failed\n");
        return -1;
    }
    if (args.status) {
        TRACE(TRACE_SCSI, 1, "STATS_RESET failed (status 0x%x)\n", args.status);
        return -1;
    }
    return 0;
}

int inquiry(uint64_t tid, uint64_t lun, unsigned *device_type, char *info, 
            int timeout) {
    unsigned char data[36], cdb[16];
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;

    memset(cdb, 0, 16);
    cdb[0] = INQUIRY; 
    cdb[1] = lun<<5;
    cdb[4] = 36;
    memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T));
    args.fromdev = 1;
    args.trans_len = 36;
    args.cdb = cdb;
    args.lun = lun;
    args.recv_data = data;

    if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, timeout)!=0) {
        TRACE_ERROR("initiator_command() failed\n");
        return -1;
    }
    if (args.status) {
        TRACE(TRACE_SCSI, 1, "INQUIRY failed (status 0x%x)\n", args.status);
        return -1;
    }
    *device_type = data[0]&0x1f;
    memcpy(info, data+8, 28);
    TRACE(TRACE_SCSI, 1, "Device Type 0x%x\n", *device_type); 

    return 0;
}

int read_capacity(uint64_t tid, uint64_t lun, unsigned *max_lba, 
                  unsigned *block_len, int timeout) {
    unsigned char data[8], cdb[16];
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;

    memset(cdb, 0, 16); 
    cdb[0] = READ_CAPACITY; 
    cdb[1] = lun<<5;
  
    memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T)); 
    args.recv_data = data;
    args.fromdev = 1;
    args.lun = lun;
    args.trans_len = 8;
    args.cdb = cdb;

    TRACE(TRACE_SCSI, 1, "READ_CAPACITY(tid %"PRIu64", lun %"PRIu64")\n", 
          tid, lun);
    if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, timeout)!=0) {
        TRACE_ERROR("initiator_command() failed\n"); 
        return -1;
    }
    if (args.status) {
        TRACE_ERROR("READ_CAPACITY failed (status 0x%x)\n", args.status);
        return -1;
    } 

    *max_lba = NTOHL(*((unsigned *)(data)));
    *block_len = NTOHL(*((unsigned *)(data+4)));

    TRACE(TRACE_SCSI, 1, "READ_CAPACITY(tid %"PRIu64", lun %"PRIu64") DONE\n", 
          tid, lun);
    TRACE(TRACE_SCSI, 1, "    Max LBA (lun %"PRIu64"): %u\n", lun, *max_lba);
    TRACE(TRACE_SCSI, 1, "  Block Len (lun %"PRIu64"): %u\n", lun, *block_len);
    TRACE(TRACE_SCSI, 1, "            Capacity: %"PRIu64" MB\n", 
          ((uint64_t) *max_lba**block_len)/1048576);

    if (*max_lba == 0) {
        TRACE_ERROR("Device returned Maximum LBA of zero\n");
        return -1;
    }
    if (*block_len%2) {
        TRACE_ERROR("Device returned strange block len: %u\n", *block_len);
        return -1;
    }
    return 0;
}

/* write_read_test() writes a pattern to the first and last block of
   the target:lun specified and then reads back this pattern.  <type>
   is either 6 or 10 ans specifies WRITE_6/READ_6 and WRITE_10/READ_10,
   respectively. */

int write_read_test(uint64_t tid, uint64_t lun, int type) {
    unsigned max_lba;
    unsigned char data[4096], cdb[16];
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;
    int len = 1;
    int i, j;
    int timeout = 5;

    if ((type!=6)&&(type!=10)) {
        TRACE_ERROR("bad type, select 6 or 10\n");
        return -1;
    }

    // determine last block on device

    if (read_capacity(tid, lun, &max_lba, &g_blocklen, timeout)!=0) {
        TRACE_ERROR("read_capacity() failed\n");
        return -1;
    }

    // write pattern into first and last block on device

    for (i=0; i<=max_lba; i+=max_lba) {
        for (j=0; j<g_blocklen; j++) 
            data[j] = (unsigned char) (i+j);
        memset(cdb, 0, 16); 
        if (type==10) {
            cdb[0] = WRITE_10;
            cdb[1] = lun<<5;

            // Strongarm aligns on word boundaries. 
            // So HTONL and NTOHL won't work here.

#if (BYTE_ORDER == BIG_ENDIAN)
            cdb[2] = ((unsigned char *)&i)[2];
            cdb[3] = ((unsigned char *)&i)[3];
            cdb[4] = ((unsigned char *)&i)[0];
            cdb[5] = ((unsigned char *)&i)[1];
            cdb[7] = ((unsigned char *)&len)[0];
            cdb[8] = ((unsigned char *)&len)[1];
#else
            cdb[2] = ((unsigned char *)&i)[3];
            cdb[3] = ((unsigned char *)&i)[2];
            cdb[4] = ((unsigned char *)&i)[1];
            cdb[5] = ((unsigned char *)&i)[0];
            cdb[7] = ((unsigned char *)&len)[1];
            cdb[8] = ((unsigned char *)&len)[0];
#endif

        } else {
            *((unsigned *)(cdb+0)) = HTONL(i);
            cdb[0] = WRITE_6;
            cdb[1] = (cdb[1]&0x1f)|lun<<5;
            cdb[4] = len;
        }
        memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T)); 
        args.send_data = data;
        args.todev = 1;
        args.length = g_blocklen;
        args.lun = lun;
        args.trans_len = g_blocklen;
        args.cdb = cdb;

        if (initiator_command(ISCSI_SCSI_CMD, &args, (int) tid, &cmd, -1)!=0) {
            TRACE_ERROR("initiator_command() failed\n"); 
            return -1;
        }
        if (args.status) {
            TRACE_ERROR("initiator_command() failed\n");
            return -1;
        }
    }


    // read pattern back from first and last block
    for (i=0; i<=max_lba; i+=max_lba) {
        memset(cdb, 0, 16); 
        if (type==10) {
            cdb[0] = READ_10;
            cdb[1] = lun<<5;

            // Strongarm aligns on word boundaries.
            // So HTONL and NTOHL won't work here.
  
#if (BYTE_ORDER == BIG_ENDIAN)
            cdb[2] = ((unsigned char *)&i)[2];
            cdb[3] = ((unsigned char *)&i)[3];
            cdb[4] = ((unsigned char *)&i)[0];
            cdb[5] = ((unsigned char *)&i)[1];
            cdb[7] = ((unsigned char *)&len)[0];
            cdb[8] = ((unsigned char *)&len)[1];
#else
            cdb[2] = ((unsigned char *)&i)[3];
            cdb[3] = ((unsigned char *)&i)[2];
            cdb[4] = ((unsigned char *)&i)[1];
            cdb[5] = ((unsigned char *)&i)[0];
            cdb[7] = ((unsigned char *)&len)[1];
            cdb[8] = ((unsigned char *)&len)[0];
#endif

        } else {
            *((unsigned *)(cdb+0)) = HTONL(i);
            cdb[0] = READ_6;
            cdb[1] = (cdb[1]&0x1f)|lun<<5;
            cdb[4] = len;
        }
        memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T)); 
        args.recv_data = data;
        args.fromdev = 1;
        args.lun = lun;
        args.trans_len = g_blocklen;
        args.cdb = cdb;
        if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, -1)!=0) {
            TRACE_ERROR("initiator_command() failed\n"); 
            return -1;
        }
        if (args.status) {
            TRACE_ERROR("initiator_command() failed\n");
            return -1;
        }
        for (j=0;j<g_blocklen;j++) {
            if (data[j] != (unsigned char) (i+j)) {
                TRACE_ERROR("Bad byte. data[%i] = %u, expected %u.\n", 
                            j, data[j], (unsigned char) (i+j));
                return -1;
            }
        }
    }
    return 0;
}

/*
 * WRITE_10|READ_10
 */

int read_or_write(uint64_t tid, uint64_t lun, unsigned lba, unsigned len, 
                  unsigned block_len, unsigned char *data, int sg_len, 
                  int write,
                  int timeout) {
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;
    unsigned char cdb[16];
    int mark=0;

    // Build CDB

    memset(cdb, 0, 16);
    cdb[0] = write?WRITE_10:READ_10;
    cdb[1] = lun<<5 | mark<<1;
    cdb[2] = ((unsigned char *)&lba)[3];
    cdb[3] = ((unsigned char *)&lba)[2];
    cdb[4] = ((unsigned char *)&lba)[1];
    cdb[5] = ((unsigned char *)&lba)[0];
    cdb[6] = 0;
    cdb[7] = ((unsigned char *)&len)[1];
    cdb[8] = ((unsigned char *)&len)[0];

    // Build iSCSI command

    memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T));
    args.lun = lun;
    args.todev = write?1:0;
    args.fromdev = write?0:1;
    args.trans_len = len*block_len;
    args.send_data = write?data:NULL;
    args.send_sg_len = write?sg_len:0;
    args.recv_data = write?NULL:data;
    args.recv_sg_len = write?0:sg_len;
    args.cdb = cdb;
 
    PRINT_BUFF(TRACE_SCSI, 1, cdb, 8, 8);

    if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, timeout)!=0) {
        TRACE_ERROR("initiator_command() failed\n"); 
        return -1;
    }

    if (args.status) {
        TRACE_ERROR("scsi_command() failed (status 0x%x)\n", args.status);
        return -1;
    }
    return 0;
}

int async_callback(void *arg) {
    ASYNC_CMD_T *cmd = (ASYNC_CMD_T *) arg;

    TRACE(TRACE_SCSI, 1, "cmd %p tag 0x%x done (iscsi status %i scsi "
          "status %i)\n", 
          &cmd->cmd, cmd->cmd.tag, cmd->cmd.status, cmd->scsi_cmd.status);
    if (cmd->cmd.status!=0) {
        TRACE_ERROR("command 0x%x failed\n", cmd->cmd.tag);
        exit(1);
    }
    ISCSI_WAKE_ELSE(((cmd->busy=0)||1), &cmd->wq, return -1);    

    return 0;
}

int build_command_disk(INITIATOR_CMD_T *cmd, ISCSI_SCSI_CMD_T *scsi_cmd, 
                       unsigned char *cdb, uint64_t lun, unsigned lba, 
                       unsigned len, int read, unsigned char *data) {

    /* initialize cdb */
    memset(cdb, 0, 16);
    cdb[0] = (!read)?WRITE_10:READ_10;
    cdb[1] = lun<<5;
    cdb[2] = ((unsigned char *)&lba)[3];
    cdb[3] = ((unsigned char *)&lba)[2];
    cdb[4] = ((unsigned char *)&lba)[1];
    cdb[5] = ((unsigned char *)&lba)[0];
    cdb[7] = ((unsigned char *)&len)[1];
    cdb[8] = ((unsigned char *)&len)[0];        

    /* initialize scsi cmd */
    memset(scsi_cmd, 0, sizeof(ISCSI_SCSI_CMD_T));
    scsi_cmd->lun = lun;
    scsi_cmd->todev = (!read)?1:0;
    scsi_cmd->fromdev = (!read)?0:1;
    scsi_cmd->trans_len = len*g_blocklen;
    scsi_cmd->send_data = (!read)?data:NULL;
    scsi_cmd->send_sg_len = 0;
    scsi_cmd->recv_data = (!read)?NULL:data;
    scsi_cmd->recv_sg_len = 0;
    scsi_cmd->cdb = cdb;

    return 0;
}

/*
 * Globals for async test
 */

ISCSI_WORKER_T worker[MAX_DEPTH];
ISCSI_LOCK_T g_worker_lock;
uint64_t g_offset;
uint64_t g_size = MAX_SIZE;
uint32_t g_rqst = 1024;
int g_random_percent = 0;
int g_write_percent = 0;
int g_think_time = 0;
uint64_t g_total_bytes;
uint64_t g_total_ops;
uint64_t g_total_latency = 0;
int g_tid = 0;
int g_lun = 0;
int g_osd = 0;
int g_sampling = 1;

/*
 * worker for osd async test
 */

int worker_proc_t(void *arg) {
    ISCSI_WORKER_T *me = (ISCSI_WORKER_T *) arg;
    unsigned char buffer[MAX_REQUEST];
    int writing;
    ISCSI_SCSI_CMD_T iscsi_args;
    INITIATOR_CMD_T cmd;
    unsigned char cdb[256];
    uint64_t offset;
    OSD_DEVICE_T dev = {g_tid, g_lun};
    OSD_OPS_MEM_T osd_mem;
    osd_args_t osd_args;
    uint64_t len;
    char line[256];
    struct timeval start, stop;
    uint32_t latency = 0;

    START_ELSE(me, 1, return -1);
    while (!WORKER_STOPPING(me)) {
        len = g_rqst;
        ISCSI_LOCK_ELSE(&g_worker_lock, EXIT(me, -1));
        if (g_sampling) {

            /* sequential vs. random, read vs. write */
            if ((rand()%100+1)<=g_random_percent) {
                g_offset = rand()%(g_size/len)*len;
            } else {
                g_offset = g_offset+len;            
                if (g_offset == g_size) g_offset = 0;
            }
            g_total_bytes += len;
            g_total_ops++;
            offset = g_offset;
            
            /* read vs. write */
            if ((rand()%100+1)<=g_write_percent) {
                writing = 1;
            } else {
                writing = 0;
            }

        } else {
            if (fscanf(stdin, "%i %"PRIu64" %"PRIu64"\n", &writing, &offset, 
                       &len)!=3) {
                PRINT("***CORRUPTED STREAM***\n");
                EXIT(me, 1);
            }
        }
        g_total_latency += latency;
        ISCSI_UNLOCK_ELSE(&g_worker_lock, EXIT(me, -1));

        if (len%512) {
            TRACE_ERROR("bad length %"PRIu64" (line %s)\n", len, line);
            exit(1);
        }
        if (len>MAX_REQUEST) {
            TRACE_ERROR("bad length %"PRIu64" > max %u\n", len, MAX_REQUEST);
            EXIT(me, 1);
        }

        /* build up command */
        memset(cdb, 0, 256);
        memset(&iscsi_args, 0, sizeof(ISCSI_SCSI_CMD_T));
        memset(&cmd, 0, sizeof(INITIATOR_CMD_T));
        if (g_osd) {
            osd_args.opcode = 0x7F;
            if (writing) {
                osd_args.service_action = OSD_WRITE;
            } else {
                osd_args.service_action = OSD_READ;
            }
            osd_args.GroupID = g_GroupID;
            osd_args.UserID = g_UserID;
            osd_args.offset = offset;
            osd_args.length = len;
            memset(&osd_mem, 0, sizeof(OSD_OPS_MEM_T));
            if (writing) {
                osd_mem.send_data = buffer;
                osd_mem.send_len = len;
            } else {
                osd_mem.recv_data = buffer;
                osd_mem.recv_len = len;
            }
            osd_command_build(&iscsi_args, &osd_args, cdb, &osd_mem, &dev);
        } else {
            cdb[0] = writing?0x2a:0x28;
            *((unsigned *)(cdb+2)) = htonl(offset/g_blocklen);
            *((unsigned short *)(cdb+7)) = htons(len/g_blocklen);
            iscsi_args.lun = g_lun;
            iscsi_args.cdb = cdb;
            iscsi_args.fromdev = !writing;
            iscsi_args.todev = writing;
            iscsi_args.trans_len = len;
            iscsi_args.send_data = writing?buffer:NULL;
            iscsi_args.recv_data = (!writing)?buffer:NULL;
        }

        /* do I/O */
        gettimeofday(&start, NULL);
        if (initiator_command(ISCSI_SCSI_CMD, &iscsi_args, g_tid, &cmd, -1)
            !=0) {
            TRACE_ERROR("initiator_command() failed\n"); 
            EXIT(me, -1);
        }
        gettimeofday(&stop, NULL);
        latency = (stop.tv_sec*1000000+stop.tv_usec)-(start.tv_sec*1000000+
                                                      start.tv_usec);
        if (iscsi_args.status) {
            TRACE_ERROR("scsi_command() failed (status 0x%x)\n", 
                        iscsi_args.status);
            EXIT(me, -1);
        }

        /* think some */
        if (g_sampling) {
            ISCSI_USLEEP(g_think_time);
        }
    }
    /* add in the last command */
    ISCSI_LOCK_ELSE(&g_worker_lock, EXIT(me, -1));
    g_total_latency += latency;
    ISCSI_UNLOCK_ELSE(&g_worker_lock, EXIT(me, -1));

    EXIT(me, 0);
}

int async_test(uint64_t tid, uint64_t lun, uint64_t max_size, 
               unsigned request_size, unsigned queue_depth, 
               unsigned read_percent, unsigned random_percent,
               int warm_time, int test_time, FILE *stream,
               int think_time, int iter, int first_run, int osd, 
               int sample, int replay) {
    struct timeval t_start, t_stop;
    int j;
    double actual_warm_time = 0;
    double actual_test_time;
    int outstanding = queue_depth;
    char counters_start[4096];
    char counters_stop[4096];
    char rqst_file[256] = "file";

    printf("doing async test!\n");

    /* set test parameters (used by workers) */
    g_osd = osd;
    g_tid = tid;
    g_lun = lun;
    g_sampling = !replay;
    g_think_time = think_time;

    ISCSI_LOCK_INIT_ELSE(&g_worker_lock, return -1);

    /* check a few things */
    if (queue_depth>MAX_DEPTH) {
        TRACE_ERROR("max queue_depth is %u\n", MAX_DEPTH);
        return -1;
    }

    if (!replay) {
        if (request_size>MAX_REQUEST) {
            TRACE_ERROR("max request_size is %u\n", MAX_REQUEST);
            return -1;
        }
        if (request_size%512) {
            TRACE_ERROR("request_size must be a multiple of 512\n");
            return -1;
        }

        /* set test parameters  */
        g_size = max_size;
        g_rqst = request_size;
        g_write_percent = 100-read_percent;
        g_random_percent = random_percent;

    }
        
    /* get beginning counters on target */
#if 0
    COUNTERS_GET(tid, lun, counters_start);
#else
    fprintf(stderr, "NOT GETTING START COUNTERS\n");
#endif

    g_total_bytes = g_total_ops = 0;

    /* test */
    gettimeofday(&t_start, 0);
    for (j=0; j<outstanding; j++)  START_WORKER_ELSE("worker", j, worker[j], 
                                                     worker_proc_t, 
                                                     NULL, return -1);  
    ISCSI_SLEEP(test_time);
    for (j=0; j<outstanding; j++)  STOP_WORKER_ELSE(worker[j], NO_SIGNAL, 
                                                    return -1);
    gettimeofday(&t_stop, 0);
    actual_test_time = toSeconds(t_stop)-toSeconds(t_start);

    /* get ending counters */
#if 0
    COUNTERS_GET(tid, lun, counters_stop);
#else
    fprintf(stderr, "NOT GETTING STOP COUNTERS\n");
#endif

#if 0
    /* output stats */
    if (first_run) {
        if (output_stats(counters_start, counters_stop, "counters_diff", 
                         (void *) stream, OUTPUT_TYPE_HEADER|
                         OUTPUT_TYPE_STREAM)!=0) {
            fprintf(stderr, "output_stats() failed\n");
        }
     }
    fprintf(stream, "%i %i %i %i %i %i ", iter, g_rqst/1024, outstanding, 
            g_random_percent, 100-g_write_percent, think_time);
    fprintf(stream, "%.2lf %.2lf %.0lf %.2lf %.2lf ", actual_warm_time, 
            actual_test_time, 
            (double) g_total_ops/actual_test_time, 
            g_total_bytes/actual_test_time/1048576, 
            (double) g_total_latency/g_total_ops);
    if (output_stats(counters_start, counters_stop, "counters_diff", 
                     (void *) stream, OUTPUT_TYPE_DATA|OUTPUT_TYPE_STREAM)) {
        fprintf(stderr, "output_stats() failed\n");
    }
    fflush(stream);
#endif
        
    /* clean up */
    ISCSI_LOCK_DESTROY_ELSE(&g_worker_lock, return -1);

    return 0;
}

int integrity_test(uint64_t tid, uint64_t lun, unsigned length, int sg_factor) {
    unsigned max_lba, block_len;
    unsigned char **data;
    struct iovec *sg;
    int i, j;
    int timeout = 5;

    /* Get device block len & capacity; and check args */
    if (read_capacity(tid, lun, &max_lba, &block_len, timeout)!=0) {
        TRACE_ERROR("read_capacity() failed\n");
        return -1;
    }
    if (length%block_len) {
        TRACE_ERROR("length must be a multiple of block len %u\n", block_len);
        return -1;
    }
    if (!sg_factor) { 
        TRACE_ERROR("sg_factor must be at least 1\n");
        return -1;
    }
    if (length%sg_factor) {
        TRACE_ERROR("length must be a multiple of sg_factor\n");
        return -1;
    }
    if ((uint64_t)length>((uint64_t)(max_lba+1)*block_len)) {
        TRACE_ERROR("attempt to read past device (max length %"PRIu64", "
                    "request %u)\n", (uint64_t) max_lba*block_len, length);
        return -1;
    } 

    /* Allocate sg and data buffers; fill data buffers with pattern */
    if ((sg=iscsi_malloc(sg_factor*sizeof(struct iovec)))==NULL) {
        TRACE_ERROR("out of memory\n");
        return -1;
    }
    if ((data=iscsi_malloc(sg_factor*sizeof(unsigned char*)))==NULL) {
        TRACE_ERROR("out of memory\n");
        return -1;
    }
    for (i=0; i<sg_factor; i++) {
        if ((data[i]=iscsi_malloc(length/sg_factor))==NULL) {
            TRACE_ERROR("out of memory\n");
            return -1;
        }
        for (j=0; j<(length/sg_factor); j++) {
            data[i][j] = (unsigned char)(i+1);
        }
    }

    /* Write blocks */
    for (j=0; j<sg_factor; j++) {
        sg[j].iov_base = data[j];
        sg[j].iov_len = length/sg_factor;
    }
    if (read_or_write(tid, lun, 0, length/block_len, block_len, 
                      (unsigned char *)sg, sg_factor, 1, timeout)!=0) {
        TRACE_ERROR("read_or_write() failed\n");
        goto done;
    }

    /* Read blocks */
    for (j=0; j<sg_factor; j++) {
        memset(data[j], 0, length/sg_factor);
        sg[j].iov_base = data[j];
        sg[j].iov_len = length/sg_factor;
    }
    if (read_or_write(tid, lun, 0, length/block_len, block_len, 
                      (unsigned char *)sg, sg_factor, 0, timeout)!=0) {
        TRACE_ERROR("read_or_write() failed\n");
        goto done;
    }
    for (i=0; i<sg_factor; i++) {
        for (j=0; j<(length/sg_factor); j++) {
            if ((unsigned char)(data[i][j]) != (unsigned char)(i+1)) {
                TRACE_ERROR("Bad byte data[%i][%i]: got %u, expected %u\n", 
                            i, j, data[i][j], (unsigned char) (i+1));
                exit(1);
            }
        }
    }

    /* cleanup */
 done: 
    for (i=0; i<sg_factor; i++) {
        if (data[i]) iscsi_free(data[i]);
    }
    iscsi_free(data);
    iscsi_free(sg);

    return 0;
}

int nop_test(uint64_t tid, uint64_t lun, unsigned iters) {
    struct timeval t_start, t_stop;
    char *data;
    int i, j, k;

    if ((data=iscsi_malloc(4096))==NULL) {
        TRACE_ERROR("iscsi_malloc() failed\n");
        return -1;
    }

    // Fill with some pattern

    for(i=0; i<4096; i++) 
        data[i] = (unsigned char) i;

    for (k=0; k<=1; k++) {             // 0 = no ping, 1= ping
        for (j=0; j<=4096; j*=4) {       // payload between 0 and 4K 
            gettimeofday(&t_start, 0);
            for (i=0; i<iters; i++) {
                if (nop_out(tid, lun, j, k, data)!=0) {
                    TRACE_ERROR("nop_out() failed\n");
                    return -1;
                }
            }
            gettimeofday(&t_stop, 0);
            PRINT("NOP_OUT (%4i bytes, ping = %i): %u iters in %.2f sec --> "
                  "%.2f usec\n", j, k,
                  iters, toSeconds(t_stop)-toSeconds(t_start),
                  ((toSeconds(t_stop)-toSeconds(t_start))*1e6)/iters);
            if (!j) j = 1;
        }
    }
    iscsi_free(data);

    return 0;
}

// latency_test() performs <op> for a number of iterations and outputs
// the average latency.  <op> can be any of WRITE_10, READ_10, 
// TEST_UNIT_READY, READ_CAPACITY or INQUIRY.

int latency_test(uint64_t tid, uint64_t lun, unsigned char op, unsigned iters, 
                 double *latency_usec) {
    unsigned length, trans_len;
    unsigned max_lba;
    unsigned block_len = 1024;
    int input, output;
    unsigned char *data, cdb[16];
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;
    struct timeval t_start, t_stop;
    int i, rc = -1;
    unsigned lba = 0;
    unsigned short len;
    int timeout = 5;
    int request_size = 65536;

    /* Get device block len */
    if ((op==WRITE_10)||(op==READ_10)) {
        if (read_capacity(tid, lun, &max_lba, &block_len, timeout)!=0) {
            TRACE_ERROR("read_capacity() failed\n");
            return -1;
        }
        if ((data = iscsi_malloc(request_size))==NULL) {
            TRACE_ERROR("out of memory\n");
            return -1;
        }
    } else {
        if ((data = iscsi_malloc(block_len))==NULL) {
            TRACE_ERROR("out of memory\n");
            return -1;
        }
    }
    len = request_size/block_len;

    /* Build CDB */
    memset(cdb, 0, 16);
    input = output = length = trans_len = 0;
    cdb[1] = lun<<5;
    switch (op) {
    case (TEST_UNIT_READY):
        cdb[0] = TEST_UNIT_READY; 
        break;
    case (INQUIRY):
        cdb[0] = INQUIRY; 
        cdb[4] = 36;
        input = 1; 
        trans_len = 36;
        break;
    case (READ_CAPACITY):
        cdb[0] = READ_CAPACITY; 
        input = 1; 
        trans_len = 8;
        break;
    case (READ_10):
        input = 1;
        trans_len = request_size;
        cdb[0] = READ_10;
        cdb[1] = lun<<5;

        // Strongarm aligns on word boundaries.
        // So HTONL and NTOHL won't work here.

#if (BYTE_ORDER == BIG_ENDIAN)
        cdb[2] = ((unsigned char *)&lba)[2];
        cdb[3] = ((unsigned char *)&lba)[3];
        cdb[4] = ((unsigned char *)&lba)[0];
        cdb[5] = ((unsigned char *)&lba)[1];
        cdb[7] = ((unsigned char *)&len)[0];
        cdb[8] = ((unsigned char *)&len)[1];
#else
        cdb[2] = ((unsigned char *)&lba)[3];
        cdb[3] = ((unsigned char *)&lba)[2];
        cdb[4] = ((unsigned char *)&lba)[1];
        cdb[5] = ((unsigned char *)&lba)[0];
        cdb[7] = ((unsigned char *)&len)[1];
        cdb[8] = ((unsigned char *)&len)[0];
#endif
        break;
    case (WRITE_10):
        output = 1;
        trans_len = request_size;
        cdb[0] = WRITE_10;
        cdb[1] = lun<<5;

        // Strongarm aligns on word boundaries.
        // So HTONL and NTOHL won't work here.

#if (BYTE_ORDER == BIG_ENDIAN)
        cdb[2] = ((unsigned char *)&lba)[2];
        cdb[3] = ((unsigned char *)&lba)[3];
        cdb[4] = ((unsigned char *)&lba)[0];
        cdb[5] = ((unsigned char *)&lba)[1];
        cdb[7] = ((unsigned char *)&len)[0];
        cdb[8] = ((unsigned char *)&len)[1];
#else
        cdb[2] = ((unsigned char *)&lba)[3];
        cdb[3] = ((unsigned char *)&lba)[2];
        cdb[4] = ((unsigned char *)&lba)[1];
        cdb[5] = ((unsigned char *)&lba)[0];
        cdb[7] = ((unsigned char *)&len)[1];
        cdb[8] = ((unsigned char *)&len)[0];
#endif
        break;
    default:
        TRACE_ERROR("op 0x%x not implemented\n", op); 
        return -1;
    }

    /* Run test */
  
    gettimeofday(&t_start, 0);
    for (i=0; i<iters; i++) {

        memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T));
        args.lun = lun;
        args.cdb = cdb;
        args.fromdev = input;
        args.todev = output;
        args.trans_len = trans_len;
        args.send_data = output?data:NULL;
        args.recv_data = input?data:NULL;

        if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, -1)!=0) {
            TRACE_ERROR("initiator_command() failed\n"); 
            goto done;
        }
        if (args.status) {
            TRACE_ERROR("scsi_command() failed (status 0x%x)\n", args.status);
            goto done;
        }
    }
    gettimeofday(&t_stop, 0);
    PRINT("op 0x%2x: %u iters in %.2f sec --> %.2f usec ",
          op, iters, toSeconds(t_stop)-toSeconds(t_start),
          ((toSeconds(t_stop)-toSeconds(t_start))*1e6)/iters);
    if ((op == WRITE_10)||(op == READ_10)) {
        PRINT("(%.2lf IOPS, %.2lf MB/sec)\n", iters/(toSeconds(t_stop)-
                                                     toSeconds(t_start)), 
              ((iters*len*g_blocklen)/(toSeconds(t_stop)-
                                       toSeconds(t_start)))/1048576.0);
    } else {
        PRINT("\n");
    }
    *latency_usec = ((toSeconds(t_stop)-toSeconds(t_start))*1e6)/iters;

    rc = 0;
 done:
    iscsi_free(data); 
    return rc;
}

// scatter_gather() tests scatter/gather performance for WRITE_10 and READ_10.
// Instead of specifying a data buffer in args.send_data and arg.recv_data, we
// specify a scatter/gather list.

int scatter_gather_test(uint64_t tid, uint64_t lun, unsigned char op) {
    unsigned length, trans_len;
    unsigned char cdb[16];
    unsigned lba = 0;
    struct iovec *sg;
    unsigned char **buff;
    unsigned block_len, max_lba;
    unsigned short len;
    int i, n;
    int input, output;
    INITIATOR_CMD_T cmd;
    ISCSI_SCSI_CMD_T args;
    int xfer_size = 128*1048576;
    int xfer_chunk = 1048576;
    int rc = -1;
    struct timeval t_start, t_stop;
    int timeout = 5;

    // Number of iterations (xfer_chunk bytes read/written per iteration)

    if (xfer_size%xfer_chunk) {
        TRACE_ERROR("xfer_size (%i) is not a multiple of xfer_chunk (%i)\n", 
                    xfer_size, xfer_chunk);
        return -1;
    }
    n = xfer_size/xfer_chunk;

    // Number of blocks per iteration

    if (read_capacity(tid, lun, &max_lba, &block_len, timeout)!=0) {
        TRACE_ERROR("read_capacity() failed\n");
        return -1;
    }
    if (xfer_chunk%block_len) {
        TRACE_ERROR("xfer_chunk (%i) is not a multiple of block_len (%i)\n", 
                    xfer_chunk, block_len);
        return -1;
    }
    len = xfer_chunk/block_len; 

    // Build CDB

    memset(cdb, 0, 16); 
    cdb[1] = lun<<5;
    trans_len = block_len*len; 
    length = input = output = 0;
    switch (op) {
    case WRITE_10:
        cdb[0] = WRITE_10;
        output = 1;
        length = trans_len;
        break;
    case READ_10:
        cdb[0] = READ_10;
        input = 1;
        break;
    default:
        TRACE_ERROR("scatter/gather test not implemented for SCSI op 0x%x\n", 
                    op);
        return -1;
    }

    // Allocate buffers for scatter/gather

    if ((buff = iscsi_malloc(len*sizeof(unsigned char *)))==NULL) {
        TRACE_ERROR("out of memory\n");
        return -1;
    } 
    for (i=0; i<len; i++) {
        buff[i] = iscsi_malloc(block_len);
        if (buff[i] == NULL) {
            TRACE_ERROR("out of memory\n");
        }
    }
    if ((sg = iscsi_malloc(len*sizeof(struct iovec)))==NULL) {
        TRACE_ERROR("out of memory\n");
        return -1;
    }
    for (i=0; i<len; i++) {
        sg[i].iov_base = buff[i];
        sg[i].iov_len = block_len;
    }

    // Strongarm aligns on word boundaries. 
    // So HTONL and NTOHL won't work here.

#if (BYTE_ORDER == BIG_ENDIAN)
    cdb[2] = ((unsigned char *)&lba)[2];
    cdb[3] = ((unsigned char *)&lba)[3];
    cdb[4] = ((unsigned char *)&lba)[0];
    cdb[5] = ((unsigned char *)&lba)[1];
    cdb[7] = ((unsigned char *)&len)[0];
    cdb[8] = ((unsigned char *)&len)[1];
#else
    cdb[2] = ((unsigned char *)&lba)[3];
    cdb[3] = ((unsigned char *)&lba)[2];
    cdb[4] = ((unsigned char *)&lba)[1];
    cdb[5] = ((unsigned char *)&lba)[0];
    cdb[7] = ((unsigned char *)&len)[1];
    cdb[8] = ((unsigned char *)&len)[0];
#endif

    gettimeofday(&t_start, 0);

    // Begin I/O 

    for (i=0; i<n; i++) {

#if 0
        for (j=0; j<len; j++) {
            sg[j].iov_base = buff[j];
            sg[j].iov_len = block_len;
        }
#endif

        memset(&args, 0, sizeof(ISCSI_SCSI_CMD_T)); 
        args.send_data = output?((unsigned char *)sg):NULL; 
        args.send_sg_len = output?len:0; 
        args.recv_data = input?((unsigned char *)sg):NULL; 
        args.recv_sg_len = input?len:0; 
        args.fromdev = input;
        args.todev = output;
        args.length = length;
        args.lun = lun;
        args.trans_len = trans_len;
        args.cdb = cdb;

        if (initiator_command(ISCSI_SCSI_CMD, &args, tid, &cmd, -1)!=0) {
            TRACE_ERROR("initiator_command() failed\n"); 
            goto done;
        }
        if (args.status) {
            TRACE_ERROR("scsi_command() failed (status 0x%x)\n", args.status);
            goto done;
        }
    }
    gettimeofday(&t_stop, 0);
    printf("SCSI op 0x%x: %u bytes (%s) in %.2f secs --> %.2f MB/sec\n",
           op, xfer_size, (op==WRITE_10)?"gathered":"scattered",
           toSeconds(t_stop)-toSeconds(t_start),
           (xfer_size/1048576)/(toSeconds(t_stop)-toSeconds(t_start)));

    rc = 0;

 done:
    for (i=0; i<len; i++) { 
        iscsi_free(buff[i]);
    }
    iscsi_free(buff);
    iscsi_free(sg);
    return rc;
}

/*
 * Throughput test
 */

#define RANDOMLY_SAMPLE \
    request_size = pow(2, (log(min_request_size)/log(2)+rand()%         \
                           ((int)(log(max_request_size)/log(2)-         \
                                  log(min_request_size)/log(2)+1))));   \
    depth = rand()%(max_depth-min_depth+1)+min_depth;                   \
    think_time = rand()%(MAX_THINK-MIN_THINK+1)+MIN_THINK;              \
    read = rand()%(MAX_READ-MIN_READ+1)+MIN_READ;                       \
    random = rand()%(MAX_RAND-MIN_RAND+1)+MIN_RAND; 

int throughput_test(uint64_t tid, uint64_t lun, int samples, int iters, 
                    int seed, FILE *stream, int skip, int osd, int warm_time, 
                    int test_time, int replay) {
    int min_depth = MIN_DEPTH;    
    int max_depth = MAX_DEPTH;
    int min_request_size = MIN_REQUEST;
    int max_request_size = MAX_REQUEST;
    uint64_t max_size = MAX_SIZE;
    int max_outstanding = MAX_OUTSTANDING;
    int i, s, to_skip;
    int think_time;
    int first_run = skip?0:1;
    int request_size;
    int depth, random, read;

    if (replay) {
        fprintf(stderr, "REPLAYING FROM STDIN...\n");
        if (async_test(tid, lun, MAX_SIZE, MAX_REQUEST, MAX_DEPTH, 0, 0, 
                       warm_time, test_time, stream, 0, 0, first_run, osd, 
                       0, replay)!=0) {
            TRACE_ERROR("async_test_disk() failed\n");
            return -1;
        }
        return 0;
    }

    if (first_run&&samples) {
        fprintf(stream, "%s %s %s %s %s %s %s %s %s %s %s ",
                "iter", "rqst", "qdep", "rnd", "rd", "think",
                "warm", "test", "iops", "bw", "lat");
        fflush(stream);
    }
    to_skip = skip;
    for (s=0; s<samples+skip/iters; s++) {
        srand(seed+s);
    again: RANDOMLY_SAMPLE;                        

#if 0
        PRINT("***OVERRIDING SAMPLING***\n");
        request_size = 262144;
        depth = 16;
        think_time = 0;
        read = 100;
        random = 100;
#endif

        if ((request_size*depth) > max_outstanding) goto again;
        for (i=0; i<iters; i++) {
            if (!to_skip) {
                fprintf(stderr, "sample %i (iter %i of %i) - tid %"PRIu64", "
                        "lun %"PRIu64"\n", s, i+1, iters, tid, lun);
                if (async_test(tid, lun, max_size, request_size, depth, 
                               read, random, warm_time, test_time, stream, 
                               think_time, i, first_run, osd, 0, 0)!=0) {
                    TRACE_ERROR("async_test_disk() failed\n");
                    return -1;
                }
                first_run = 0;
                fflush(stream);
#if 0
                device_exit(tid); /* restart the target */
#else
                fprintf(stderr, "NOT RESETTING DEVICE\n");
#endif
                ISCSI_SLEEP(1);
            } else {
                fprintf(stderr, "sample %i (iter %i of %i) - skipping\n", 
                        s, i+1, iters);
                to_skip--;                   
            }
        } 
    }
    return 0;
}

int disk_tests(uint64_t tid, uint64_t lun, int samples, int iters, int seed, 
               FILE *stream, int skip, int warm, int test, int replay) {
    unsigned max_lba, block_len;
    double read_lat, write_lat, other_lat;
    int timeout = 5;
    unsigned char buffer[1048576];

#if 0
    if (read_or_write(tid, lun, 0, 2048, 512, buffer, 0, 1, -1)!=0) {
        TRACE_ERROR("read_or_write() failed\n");
        return -1;
    }
    return 0;
#endif

    if (!samples&&!replay) {
        /* initial tests */
        if (read_capacity(tid, lun, &max_lba, &block_len, timeout)!=0) {
            TRACE_ERROR("read_capacity() failed\n");
            return -1;
        }
        PRINT("read_capacity PASSED\n");
        if (write_read_test(tid, lun, 10)!=0) {
            TRACE_ERROR("write_read_test() failed\n");
            return -1;
        }
        PRINT("write_read_test PASSED\n"); 
        if (integrity_test(tid, lun, MAX_REQUEST, 1)!=0) {
            TRACE_ERROR("integrity_test() failed\n");
            return -1;
        }
        PRINT("integrity_test PASSED\n");  

#if 0
        if (scatter_gather_test(tid, lun, READ_10)!=0) {
            TRACE_ERROR("integrity_test() failed\n");
            return -1;
        }
        PRINT("scatter_gather_test (READ_10) PASSED\n");
        
        if (scatter_gather_test(tid, lun, WRITE_10)!=0) {
            TRACE_ERROR("integrity_test() failed\n");
            return -1;
        }
        PRINT("scatter_gather_test (WRITE_10) PASSED\n");
#endif
        
        /* latency tests */
        if (latency_test(tid, lun, READ_10, 1000, &read_lat)!=0) {
            TRACE_ERROR("latency_test(READ_10) failed\n");
            return -1;
        }
        if (latency_test(tid, lun, WRITE_10, 1000, &write_lat)!=0) {
            TRACE_ERROR("latency_test(WRITE_10) failed\n");
            return -1;
        }
        if (latency_test(tid, lun, READ_CAPACITY, 1000, &other_lat)!=0) {
            TRACE_ERROR("latency_test(READ_CAPACITY) failed\n");
            return -1;
        }
    } else {
        throughput_test(tid, lun, samples, iters, seed, stream, skip, 0, 
                        warm, test, replay);
    }
    return 0;
}

int osd_tests(uint64_t tid, uint64_t lun, int samples, int iters, int seed, 
              FILE *stream, int skip, int warm, int test, int replay) {
    char buffer[1024];
    OSD_DEVICE_T dev = {tid, lun};
    uint16_t len;

    if (osd_create_group((void *)&dev, &osd_command, &g_GroupID)!=0) {
        TRACE_ERROR("osd_create_group() failed\n");
        return -1;
    }
    if (osd_create((void *)&dev, g_GroupID, &osd_command, &g_UserID)!=0) {
        TRACE_ERROR("osd_create() failed\n");
        return -1;
    }
 
    if (!samples) {
        if (osd_write((void *)&dev, g_GroupID, g_UserID, 0, 13, 
                      "Hello, World!", 0, &osd_command)!=0) {
            TRACE_ERROR("osd_write() failed\n");
            return -1;
        }
        PRINT("OSD_WRITE: PASSED\n");

        if (osd_read((void *)&dev, g_GroupID, g_UserID, 0, 13, buffer, 0, 
                     &osd_command)!=0) {
            TRACE_ERROR("osd_write() failed\n");
        }
        if (strncmp(buffer, "Hello, World!", 13)) {
            TRACE_ERROR("osd_read() failed\n");
            return -1;
        } 
        PRINT("OSD_READ: PASSED\n");
        
        if (osd_set_one_attr((void *)&dev, g_GroupID, g_UserID, 0x30000000, 
                             0x1, INODE_LEN, buffer, &osd_command)!=0) {
            TRACE_ERROR("osd_write() failed\n");
            return -1;
        }
        PRINT("OSD_SET_ATTR: PASSED\n");
 
        if (osd_get_one_attr((void *)&dev, g_GroupID, g_UserID, 0x30000000, 
                             0, INODE_LEN, &osd_command, &len, buffer)!=0) {
            TRACE_ERROR("osd_get_one_attr() failed\n");
            return -1;
        }
        if (strncmp(buffer, "Hello, World!", 13)) {
            TRACE_ERROR("osd_read() failed\n");
            return -1;
        } 
        PRINT("OSD_GET_ATTR: PASSED\n");
    } else {
        if (samples||replay) {
            throughput_test(tid, lun, samples, iters, seed, stream, skip, 1, 
                            warm, test, replay);
        }
    }

    /* clean up */
    if (osd_remove((void *)&dev, g_GroupID, g_UserID, &osd_command)!=0) {
        TRACE_ERROR("osd_remove() failed\n");
        return -1;
    }
    if (osd_remove_group((void *)&dev, g_GroupID, &osd_command)!=0) {
        TRACE_ERROR("osd_remove_group() failed\n");
        return -1;
    }

    return 0;
}

int test_all(uint64_t tid, uint64_t lun, int full, int samples, int iters, 
             int seed, FILE *stream, 
             int skip, int warm, int test, int replay) {
    unsigned device_type = 0;
    char *data = "Hello, world!!!!"; 
    int latency_iters = 100;
    double latency;
    char info[36];
    char vendor[9];
    uint32_t max_lba, block_len;
    char model[17];
    char version[5];
    int timeout = 5;

#if 0
    {
        unsigned char buffer[512];
        if (read_or_write(tid, lun, 123, 1, 512, buffer, 0, 1, -1)!=0) {
            TRACE_ERROR("read_or_write() failed\n");
            return -1;
        }
        return 0;
    }
#endif

    /* SBC or OSD */
    if (!((inquiry(tid, lun, &device_type, info, timeout)==0)&&
          (device_type!=0x1f))) {
        TRACE_ERROR("lun %"PRIu64": offline\n", lun);
        return -1;
    }        
    memcpy(vendor, info, 8); vendor[8] = '\0';
    memcpy(model, info+8, 16); model[16] = '\0';
    memcpy(version, info+24, 4); version[4] = '\0';
    if (device_type == 0x0) {
        if (read_capacity(tid, lun, &max_lba, &block_len, timeout)!=0) {
            TRACE_ERROR("read_capacity() failed\n");
            return -1;
        }
    }
    if (!samples) {
        PRINT("tid %"PRIu64" lun %"PRIu64": Vendor: %8s Model: %16s Rev: "
              "%4s Cap: %4.0f GB\n", 
              tid, lun, vendor, model, version, (max_lba+1)*
              (block_len/(1048576.0*1024)));
    }

    if (full|samples|replay) {

        /* NOP_OUT */
        if (nop_out(tid, lun, strlen(data), 0, data)!=0) {
            TRACE_WARN("nop_out() failed\n");
        }

        /* NOP_OUT w/ PING */
        if (nop_out(tid, lun, strlen(data), 1, data)!=0) {
            TRACE_WARN("nop_out() w/ ping failed\n");
        }

        switch(device_type) {
        case 0x00:
            if (disk_tests(tid, lun, samples, iters, seed, stream, skip, 
                           warm, test, replay)!=0) {
                TRACE_ERROR("disk_tests() failed\n");
                return -1;
            }
            break;
        case 0x0e:
            if (osd_tests(tid, lun, samples, iters, seed, stream, skip, 
                          warm, test, replay)!=0) {  
                TRACE_ERROR("osd_tests() failed\n");
                return -1;
            }
            break;
        case 0x1f:
            PRINT("invalid LUN\n");
            return -1;
            break;
        default:
            break;

        }
    }

    return 0;
}
