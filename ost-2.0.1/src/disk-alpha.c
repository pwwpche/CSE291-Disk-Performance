
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

#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <scsi/scsi.h>
#include <sys/times.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include "iscsi.h"
#include "util.h"
#include "debug.h"
#include "device.h"
#include "initiator.h"
#include "tests.h"
#include "target.h"
#include "research.h"
#include "config.h"
#include "util.h"

typedef struct workload_char {
    uint64_t ops;
    uint64_t ops_write;
    uint64_t ops_read;
    uint64_t ops_bidi;
    uint64_t ops_other;
    uint64_t outstanding;
    uint64_t outstanding_write;
    uint64_t outstanding_read;
    uint64_t outstanding_bidi;
    uint64_t outstanding_other;
    uint64_t depths;
    uint64_t depths_write;
    uint64_t depths_read;
    uint64_t depths_bidi;
    uint64_t depths_other;
    uint64_t latencies;
    uint64_t latencies_write;
    uint64_t latencies_read;
    uint64_t latencies_bidi;
    uint64_t latencies_other;
    uint64_t bytes_todev;
    uint64_t bytes_fromdev;

    uint64_t jumps;
    uint64_t jumps_write;
    uint64_t jumps_read;
    uint64_t last_lba, last_wr_lba, last_rd_lba;

    ISCSI_SPIN_LOCK_T lock;
} DEVICE_STATS_T;

#define CONFIG_DISK_NUM_LUNS_DFLT           1
#define CONFIG_DISK_BLOCK_LEN_DFLT          512
#define CONFIG_DISK_NUM_BLOCKS_DFLT         262144
#define CONFIG_DISK_MAX_LUNS                1
#define CONFIG_DISK_MAX_WORKERS_PER_LUN     16
#define CONFIG_LUN_QUEUE_DEPTH              256
#define CONFIG_DISK_MAX_BURST               1048576
#define CONFIG_CACHE_BLOCKS_DFLT            (16*1024)

#define CONFIG_MAX_FILENAME 1024

/*
 * Macros
 */

#define USLEEP(T) {                                                     \
        struct timeval start, stop;                                     \
        unsigned diff;                                                  \
        gettimeofday(&start, NULL);                                     \
        do {                                                            \
            gettimeofday(&stop, NULL);                                  \
            diff = (stop.tv_sec*1e6+stop.tv_usec)-                      \
                (start.tv_sec*1e6+start.tv_usec);                       \
        } while (diff < T);                                             \
    }

/*
 * Constants
 */

#define DISK_RAM  1
#define DISK_SG   2
#define DISK_SD   3
#define DISK_FS   4
#define DISK_RAW  5
#define DISK_CTRL 6

#define DISK_INITIALIZED 1
#define DISK_SHUTTING_DOWN 2
#define DISK_CONFIG_A 0
#define DISK_CONFIG_B 1
#define DISK_CONFIG_C 2
#define DISK_CONFIG_D 3

/*
 * Types
 */

typedef struct lun_worker_args_t {
    uint64_t lun;
    int id;
} LUN_WORKER_ARGS_T;

/*
 * Globals
 */

static DEVICE_STATS_T g_device_stats[CONFIG_DISK_MAX_LUNS]; /* old one */
OST_DEVICE_COUNTERS_T g_device_counters[CONFIG_DISK_MAX_LUNS]; /* new one */
static int g_fid[CONFIG_DISK_MAX_LUNS][CONFIG_DISK_MAX_WORKERS_PER_LUN][2];
static unsigned char *g_ramdisk[CONFIG_DISK_MAX_LUNS];
static uint64_t g_disk_num_blocks = CONFIG_DISK_NUM_BLOCKS_DFLT;
uint32_t g_disk_block_len = CONFIG_DISK_BLOCK_LEN_DFLT;
static uint32_t g_disk_num_luns = CONFIG_DISK_NUM_LUNS_DFLT;
static unsigned g_type = DISK_RAM;
static unsigned g_sync = 0;
static char g_device[256] = "";
static ISCSI_QUEUE_T g_cmd_queue[CONFIG_DISK_MAX_LUNS];
static ISCSI_WORKER_T 
g_lun_worker[CONFIG_DISK_MAX_LUNS][CONFIG_DISK_MAX_WORKERS_PER_LUN];
static ISCSI_WORKER_T g_stats_worker;
static void *g_staging_buff[CONFIG_DISK_MAX_LUNS][CONFIG_LUN_QUEUE_DEPTH];
static void *g_staging_base[CONFIG_DISK_MAX_LUNS][CONFIG_LUN_QUEUE_DEPTH];
static ISCSI_QUEUE_T g_staging_queue[CONFIG_DISK_MAX_LUNS];
static LUN_WORKER_ARGS_T 
g_lun_worker_args[CONFIG_DISK_MAX_LUNS][CONFIG_DISK_MAX_WORKERS_PER_LUN];
static int g_state = 0;
static int g_counters = 0;
static char g_tracefile[CONFIG_MAX_FILENAME] = "";
static int g_tracing = 1;
static FILE *g_fptr[CONFIG_DISK_MAX_LUNS];
static int g_workers_per_lun = CONFIG_DISK_MAX_WORKERS_PER_LUN;
static int g_direct=0;
static int g_write_sleep=0;
static int g_read_sleep=0;

static int g_hybrid = 0;
static char g_device_2[CONFIG_MAX_FILENAME] = "";
static unsigned g_type_2 = DISK_RAM;
static int g_pinning=0;

/* for RAID controller mode */
static char g_target_config[CONFIG_MAX_FILENAME];
static int g_tids = 0;
static unsigned g_block_len[CONFIG_INITIATOR_MAX_TARGETS];
static unsigned g_max_lba[CONFIG_INITIATOR_MAX_TARGETS];
static int g_online[CONFIG_INITIATOR_MAX_TARGETS];
static int g_crash = -1;

extern char g_hostname[64];
extern int g_port;

/*
 * Prototypes
 */

void dump_and_reset_counters(char *buffer, int len);
static int stats_worker_proc(ISCSI_WORKER_T *me);
static int lun_worker_proc(ISCSI_WORKER_T *me);
static int command_done(TARGET_CMD_T *cmd);
static int disk_read(TARGET_CMD_T *cmd, unsigned char lun, 
                     uint64_t lba, unsigned short len);
static int prepare_write(TARGET_CMD_T *cmd, unsigned char lun, 
                         uint64_t lba, unsigned len);
static int sg_cmd_ioctl(int fid, char *cmd, int cmd_len, char *data, 
                        int data_len, int direction, int timeout);

/* 
 * Target's interface.  The target system calls all commands named "device_"
 */

OST_DEVICE_COUNTERS_T* device_counters_get(uint64_t lun) {  /* this is the new one */
    return &g_device_counters[lun];
}

DEVICE_STATS_T* device_stats_get(uint64_t lun) { /* this is the old one */
    return &g_device_stats[lun];
}

void device_pargs() {
    PRINT("  -b <block length>   "
          "Choose from 512, 1024, 2048, 4096 (dflt %i)\n", 
          CONFIG_DISK_BLOCK_LEN_DFLT);
    PRINT("  -n <num blocks>     "
          "Number of blocks per logical unit (dflt %i)\n", 
          CONFIG_DISK_NUM_BLOCKS_DFLT);
    PRINT("  -l <luns>           "
          "Number of logical units (dflt %i)\n", 
          CONFIG_DISK_NUM_LUNS_DFLT);
    PRINT("  -d <device>         "
          "e.g., /dev/sdb, /dev/sg1, /dev/raw/raw2, /tmp/file\n");
    PRINT("  -w <workers>        "
          "worker threads per lun (dflt %i)\n", 
          CONFIG_DISK_MAX_WORKERS_PER_LUN);
    PRINT("  -t <tracefile>      "
          "Output I/O trace to <tracefile> (replayed by fitness)\n");
}

static long parse_next_iarg(int i, int argc, char *argv[]) {
    /* not good enough, but better than nothing */

    char *p;
    long value;

    ++i;
    if (i >= argc || argv[i][0] == '-') {
	fprintf(stderr,"Option %s requires an argument\n",argv[i-1]);
	exit(1);
    }
    
    value = strtoul(argv[i], &p, 0);
    switch (*p) {
    case 'G': 
	value *= 1024;
    case 'M': 
	value *= 1024;
    case 'K': 
	value *= 1024;
	if (*(p+1)) {
	    fprintf(stderr, "Unknown trailing characters on argument to %s\n",
                    argv[i-1]);
	    exit(1);
	}
    case 0:   
	break;
    default: 
	fprintf(stderr, "Unknown trailing characters on argument to %s\n",
                argv[i-1]);
	exit(1);
    }

    return value;
}

static void parse_next_sarg(char *dst, int i, int argc, char *argv[]) {
    /* not good enough, but better than nothing */

    ++i;

    if (i >= argc || argv[i][0] == '-') {
	fprintf(stderr,"Option %s requires an argument",argv[i-1]);
	exit(1);
    }
    strcpy(dst, argv[i]);
}
   
int device_args(int argc, char *argv[]) {
    int i;
    
    for (i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-b")) {
            g_disk_block_len = parse_next_iarg(i++, argc, argv);
        } else if (!strcmp(argv[i], "-n")) {
            g_disk_num_blocks =  parse_next_iarg(i++, argc, argv);
	} else if (!strcmp(argv[i], "-l")) {
            g_disk_num_luns = parse_next_iarg(i++, argc, argv);
            if (g_disk_num_luns > CONFIG_DISK_MAX_LUNS) {
                TRACE_ERROR("max luns is %i\n", CONFIG_DISK_MAX_LUNS);
                return -1;
            }
        } else if (!strcmp(argv[i], "-d")) {
            parse_next_sarg(g_device, i++, argc, argv);
            if (strstr(g_device, "/dev/sg")) {
                g_type = DISK_SG;
            } else if (strstr(g_device, "/dev/raw")) {
                g_type = DISK_RAW;
            } else if (strstr(g_device, "/dev/sd")) {
                g_type = DISK_SD;
            } else { 
                g_type = DISK_FS;
            }
        } else if (!strcmp(argv[i], "-d2")) {
            parse_next_sarg(g_device_2, i++, argc, argv);
            if (strstr(g_device_2, "/dev/sg")) {
                g_type_2 = DISK_SG;
            } else if (strstr(g_device_2, "/dev/raw")) {
                g_type_2 = DISK_RAW;
            } else if (strstr(g_device_2, "/dev/sd")) {
                g_type_2 = DISK_SD;
            } else { 
                g_type_2 = DISK_FS;
            }
            g_hybrid = 2;
        } else if (!strcmp(argv[i], "-w")) {
            g_workers_per_lun = parse_next_iarg(i++, argc, argv);
            if (g_workers_per_lun > CONFIG_DISK_MAX_WORKERS_PER_LUN) {
                PRINT("max lun workers is %i\n",  
                      CONFIG_DISK_MAX_WORKERS_PER_LUN);
                return -1;
            }
        } else if (!strcmp(argv[i], "-c")) {
            g_type = DISK_CTRL;
	    parse_next_sarg(g_target_config, i++, argc, argv);
        } else if (!strcmp(argv[i], "-x")) {
            g_crash = parse_next_iarg(i++, argc, argv);
        } else if (!strcmp(argv[i], "-t")) {
	    parse_next_sarg(g_tracefile, i++, argc, argv);
        } else {
            fprintf(stderr, "unknown option %s\n", argv[i]);
            return -1;
        }
    }
    return 0;
}

int device_init_real(char *device, unsigned type) {
    int i, j;
    uint64_t size = g_disk_num_blocks*g_disk_block_len;
    struct stat statbuff;

    /* initialize disks */
    for (i=0; i<g_disk_num_luns; i++) {
        if (type == DISK_RAM) {
            if ((g_ramdisk[i]=iscsi_malloc(size))==NULL) {
                TRACE_ERROR("iscsi_malloc() failed\n");
                return -1;
            }
            PRINT("LUN %i: %"PRIu64" MB RAMDISK (%u byte block)\n", 
                  i, size/1048576, g_disk_block_len);
        }
        if (type == DISK_CTRL) {
            int timeout = 1;
            unsigned device_type;
            char vendor[9];
            char model[17];
            char version[5];
            char info[36];
            uint64_t lun = 0;
            uint64_t j;

            PRINT("LUN %i: %"PRIu64" MB RAID Controller (%u byte block)\n", 
                  i, size/1048576, g_disk_block_len);
            if ((g_tids=initiator_init(g_target_config, 0, AF_INET))<=0) {
                TRACE_ERROR("no targets found\n");
                return -1;
            }
            for (j=0; j<g_tids; j++) {
                if (!((inquiry(j, lun, &device_type, info, timeout)==0)
                      &&(device_type!=0x1f))) {
                    PRINT("tid %"PRIu64" lun %"PRIu64": offline\n", j, lun);
                    g_online[j] = 0;
                } else {
                    memcpy(vendor, info, 8); vendor[8] = '\0';
                    memcpy(model, info+8, 16); model[16] = '\0';
                    memcpy(version, info+24, 4); version[4] = '\0';
                    if (device_type == 0x0) {
                        if (read_capacity(j, lun, &g_max_lba[j],
                                          &g_block_len[j], timeout)!=0) {
                            TRACE_ERROR("read_capacity() failed\n");
                            return -1;
                        }
                    } else {
                        TRACE_ERROR("device type 0x%x not supported\n", 
                                    device_type);
                        return -1;
                    }
                    PRINT("tid %"PRIu64" lun %"PRIu64
                          ": Vendor: %8s Model: %16s Rev: %4s Cap: %4.0f GB\n",
                          j, lun, vendor, model, version, 
                          (g_max_lba[j]+1)*(g_block_len[j]/(1048576.0*1024)));
                    g_online[j] = 1;
                }
            }
        } else {
            if (type == DISK_FS) {
                int fid;
                char buffer[4096];

                if ((fid=open(device, O_CREAT|O_RDWR, 0666))==-1) {
                    TRACE_ERROR("error opening \"%s\"\n", device);
                    return -1;
                }
                if (lseek(fid, size-g_disk_block_len, SEEK_SET) == -1) {
                    TRACE_ERROR("error seeking \"%s\"\n", device);
                    return -1;
                }
                if (write(fid, buffer, g_disk_block_len) == -1) {
                    TRACE_ERROR("error writing \"%s\" (errno %i)", 
                                device, errno);
                    return -1;
                }
                close(fid);
            }
            if (type == DISK_SG) {
                char data[36];
                char inquiry[6] = {0x12, 0, 0, 0, 36, 0};
                char read_capacity[9] = {0x25, 0, 0, 0, 0, 0, 0, 0, 0};
                int fid;

                if (stat(device, &statbuff)==-1) {
                    TRACE_ERROR("cannot stat %s (errno %i)\n", device, errno);
                    return -1;
                }
                if (!S_ISCHR(statbuff.st_mode)) {
                    TRACE_ERROR("%s is not a character device\n", device);
                    return -1;
                }
                if ((fid=open(device, O_RDWR, 0666))==-1) {
                    TRACE_ERROR("error opening \"%s\"\n", device);
                    return -1;
                }
                if (sg_cmd_ioctl(fid, inquiry, 6, data, 36, SG_DXFER_FROM_DEV,
                                 10000)!=0) {
                    fprintf(stderr, "INQUIRY failed\n");
                    return -1;
                }
                PRINT("LUN %i: Vendor %.8s Model %.16s Rev %.4s\n", i, data+8, 
                      data+16, data+32);                
#if 0
                if (sg_cmd_ioctl(fid, read_capacity, 10, data, 8, 
                                 SG_DXFER_FROM_DEV, 10000)!=0) {
                    fprintf(stderr, "READ_CAPACITY failed\n");
                    return -1;
                }
#endif
                close(fid);
                size = (uint64_t) g_disk_num_blocks*g_disk_block_len;
                PRINT("LUN %i: %"PRIu64" MB SG DISK (%u byte block)\n", i, 
                      size/1048576, g_disk_block_len);
            } else if (type == DISK_SD) {
                if (stat(device, &statbuff)==-1) {
                    TRACE_ERROR("cannot stat %s (errno %i)\n", device, errno);
                    return -1;
                }
                if (!S_ISBLK(statbuff.st_mode)) {
                    TRACE_ERROR("\"%s\" is not a block device\n", device);
                    return -1;
                }
                PRINT("LUN %i: %"PRIu64" MB SCSI DISK (%u byte block)\n", i, 
                      size/1048576, g_disk_block_len);
            } else if (type == DISK_RAW) {
                if (stat(device, &statbuff)==-1) {
                    TRACE_ERROR("cannot stat %s (errno %i)\n", device, errno);
                    return -1;
                }
                if (!S_ISCHR(statbuff.st_mode)) {
                    TRACE_ERROR("%s is not a character device\n", device);
                    return -1;
                }
                PRINT("LUN %i: %"PRIu64" MB RAW SCSI DISK (%u byte block)\n",
                      i, size/1048576, g_disk_block_len);
            } else if (type == DISK_FS) {
                PRINT("LUN %i: %"PRIu64" MB DISK FILE (%u byte block)\n",
                      i, size/1048576, g_disk_block_len);
            }
        }
    }
    return 0;
}

int device_init() {
    int i, j;

    if ((g_disk_block_len!=512)
        &&(g_disk_block_len!=1024)
        &&(g_disk_block_len!=2048)
        &&(g_disk_block_len!=4096)) {
        TRACE_ERROR("Invalid block len %i. Choose 512, 1024, 2048, or 4096.\n", 
                    g_disk_block_len);
        return -1;
    }

    if (device_init_real(g_device, g_type)!=0) return -1;
    if (g_hybrid) {
        if (device_init_real(g_device_2, g_type_2)!=0) return -1;
    }

    for (i=0; i<g_disk_num_luns; i++) {
        /* open trace file per lun */
        if (strlen(g_tracefile)) {
            if ((g_fptr[i]=fopen(g_tracefile, "w+"))==NULL) {
                fprintf(stderr, "error creating tracefile \"%s\"\n", 
                        g_tracefile);
                return -1;
            }
            PRINT("\nI/O tracing enabled (see %s)\n", g_tracefile);
        }
    
        /* initialize workload characteristics */
        memset(&g_device_stats[i], 0, sizeof(DEVICE_STATS_T));
        ISCSI_LOCK_INIT_ELSE(&g_device_stats[i].lock, return -1);

        /* lun command queue */
        if (iscsi_queue_init(&g_cmd_queue[i], CONFIG_LUN_QUEUE_DEPTH)!=0) {
            TRACE_ERROR("iscsi_queue_init() failed\n");
            return -1;
        }

        /* 
         * staging buffers for incoming/outgoing data.  We can have
         * at most CONFIG_LUN_QUEUE_DEPTH of these.
         */
        if (iscsi_queue_init(&g_staging_queue[i], CONFIG_LUN_QUEUE_DEPTH)!=0) {
            TRACE_ERROR("iscsi_queue_init() failed\n");
            return -1;
        }
        for (j=0; j<(CONFIG_LUN_QUEUE_DEPTH); j++) {
            void *ptr;

            if ((ptr=iscsi_malloc(CONFIG_DISK_MAX_BURST+g_disk_block_len))
                ==NULL) {
                TRACE_ERROR("iscsi_malloc() failed\n");
                return -1;
            }            
            g_staging_buff[i][j] = g_staging_base[i][j] = ptr;
            g_staging_buff[i][j] += g_disk_block_len - 
                (((unsigned long)(g_staging_base[i][j]))%g_disk_block_len);
            if (iscsi_queue_insert(&g_staging_queue[i], 
                                   g_staging_buff[i][j])!=0) {
                TRACE_ERROR("error inserting into staging queue\n");
                return -1;
            }
        }

        /* lun workers */
        for (j=0; j<g_workers_per_lun; j++) {
            void *ptr;

            /* each worker has its own file descriptor */
            if ((g_type != DISK_RAM)&&(g_type != DISK_CTRL)) {
                if (g_direct) PRINT("using O_DIRECT for primary device\n");
                if ((g_fid[i][j][0]=open(g_device, 
                                      O_RDWR|(g_direct?O_DIRECT:0), 
                                      0666))==-1) {
                    TRACE_ERROR("error opening \"%s\"\n", g_device);
                    return -1;
                }
            }
            if ((g_hybrid)&&((g_type_2 != DISK_RAM)&&(g_type_2 != DISK_CTRL))) {
                if (g_direct) PRINT("using O_DIRECT for primary device\n");
                if ((g_fid[i][j][1]=open(g_device_2, 
                                        O_RDWR|(g_direct?O_DIRECT:0), 
                                        0666))==-1) {
                    TRACE_ERROR("error opening \"%s\"\n", g_device_2);
                    return -1;
                }
            }

            /* set lun worker id info and start lun worker */
            g_lun_worker_args[i][j].lun = i;
            g_lun_worker_args[i][j].id = j;
            START_WORKER_ELSE("lun worker", j, g_lun_worker[i][j], 
                              lun_worker_proc,
                              &g_lun_worker_args[i][j], 
                              {iscsi_queue_destroy(&g_cmd_queue[i]); 
                                  return -1;});
        }
    }

    if (g_counters) {
        /* worker to output stats at regular intervals */
        START_WORKER_ELSE("stats worker", 0, g_stats_worker, 
                          stats_worker_proc, NULL, return -1);
    }

    g_state = DISK_INITIALIZED;

    return 0;
}

int device_shutdown() {
    int i, j;
    int rc;
    char buffer[4096];

    PRINT("*** SHUTTING DOWN ***\n");
    g_state = DISK_SHUTTING_DOWN;
    for (i=0; i<g_disk_num_luns; i++) {
        if (strlen(g_tracefile)) fclose(g_fptr[i]);
        ISCSI_LOCK_DESTROY_ELSE(&g_device_stats[i].lock, return -1);
        for (j=0; j<g_workers_per_lun; j++) {
            STOP_WORKER_ELSE(g_lun_worker[i][j], 1, return -1);
        }
        for (j=0; j<g_workers_per_lun*2; j++) {
            iscsi_free(g_staging_base[i][j]);
        }
        iscsi_queue_destroy(&g_staging_queue[i]);
        iscsi_queue_destroy(&g_cmd_queue[i]);
        if ((g_type == DISK_RAM)||(g_hybrid==1)) {
            iscsi_free(g_ramdisk[i]);
        }
    }

    PRINT("*** SHUTDOWN COMPLETE ***\n");

    return 0;
}

int device_place_data(TARGET_CMD_T *cmd, unsigned off, unsigned len) {
    TARGET_SESSION_T *sess = cmd->sess;
    struct iovec *iov, *iov_ptr = NULL;
    int iov_len;
    int rc = -1;
   
    if (!len) {
        TRACE(TRACE_SCSI, 1, "data transfer completed (tag 0x%x)\n",
              cmd->scsi_cmd.tag);
        cmd->done = 1;
        if (device_enqueue(cmd)!=0)  {
            TRACE_ERROR("device_enqueue() failed\n");
            goto error;
        }
        return 0;
    }
    
    TRACE(TRACE_SCSI, 1, "placing %u bytes of data at offset %u (tag 0x%x)\n",
          len, off, cmd->scsi_cmd.tag);
    
    /* create iovec for just this portion of the data  */    
    if ((iov=iov_ptr=iscsi_malloc_atomic(sizeof(struct iovec)*
                                         cmd->scatter_len))==NULL) {
        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        goto error;
    }
    memcpy(iov, cmd->scatter_list, sizeof(struct iovec)*cmd->scatter_len); 
    iov_len = cmd->scatter_len;
    if (modify_iov(&iov, &iov_len, off, len)!=0) {
        TRACE_ERROR("modify_iov() failed\n");
        goto error;
    }
    if ((rc=iscsi_sock_msg(sess->sock, 0, len, iov, iov_len))!=len) {
        TRACE_ERROR("iscsi_sock_msg() failed (rc %u)\n", rc);
        exit(1);
        goto error;;
    }
    rc = len;
    
 error:
    if (iov_ptr) iscsi_free_atomic(iov_ptr);
    return rc;
}

int device_enqueue(TARGET_CMD_T *cmd) {
    ISCSI_SCSI_CMD_T *args = &(cmd->scsi_cmd);
    unsigned char *cdb = args->cdb;
    int is_read = 0;
    int is_write = 0;
    unsigned lba;
    unsigned short len;
    int i;

    /* 
     * We update the randomness (jump) counters here instead of in
     * device_command() in order to preserve the ordering of the
     * commands.  Once the lun workers get them the randomness
     * can change relative to what the initiator saw.
     */
 
    if (g_crash != -1) {
        g_crash--;
        if (g_crash <=0) {
            int time;
            srand(getpid());
            time = rand()%5;
            PRINT("**CRASHING (for %i secs)**\n", time);
            ISCSI_SLEEP(time);
            exit(1);
        }
    }

    switch(cdb[0]) {
    case WRITE_6:
        lba = NTOHL(*((unsigned *)cdb))&0x001fffff;
        len = cdb[4];
        if (!len) len = 256;
        is_write = 1;
        break;
    case READ_6:
        lba = NTOHL(*((unsigned *)cdb))&0x001fffff;
        len = cdb[4];
        if (!len) len = 256;
        is_read = 1;
        break;
    case WRITE_10:
#if (BYTE_ORDER == BIG_ENDIAN)
        ((unsigned char *)&lba)[0] = cdb[2];
        ((unsigned char *)&lba)[1] = cdb[3];
        ((unsigned char *)&lba)[2] = cdb[4];
        ((unsigned char *)&lba)[3] = cdb[5];
        ((unsigned char *)&len)[0] = cdb[7];
        ((unsigned char *)&len)[1] = cdb[8];
#else
        ((unsigned char *)&lba)[0] = cdb[5];
        ((unsigned char *)&lba)[1] = cdb[4];
        ((unsigned char *)&lba)[2] = cdb[3];
        ((unsigned char *)&lba)[3] = cdb[2];
        ((unsigned char *)&len)[0] = cdb[8];
        ((unsigned char *)&len)[1] = cdb[7];
#endif
        is_write = 1;
        break;
    case READ_10:
#if (BYTE_ORDER == BIG_ENDIAN)
        ((unsigned char *)&lba)[0] = cdb[2];
        ((unsigned char *)&lba)[1] = cdb[3];
        ((unsigned char *)&lba)[2] = cdb[4];
        ((unsigned char *)&lba)[3] = cdb[5];
        ((unsigned char *)&len)[0] = cdb[7];
        ((unsigned char *)&len)[1] = cdb[8];
#else
        ((unsigned char *)&lba)[0] = cdb[5];
        ((unsigned char *)&lba)[1] = cdb[4];
        ((unsigned char *)&lba)[2] = cdb[3];
        ((unsigned char *)&lba)[3] = cdb[2];
        ((unsigned char *)&len)[0] = cdb[8];
        ((unsigned char *)&len)[1] = cdb[7];
#endif
        is_read = 1;
        break;
    }

    if (!cmd->done) {
        if (is_write) {
            g_device_stats[args->lun].jumps_write += 
                abs(lba-g_device_stats[args->lun].last_wr_lba);
            g_device_stats[args->lun].jumps += 
                abs(lba-g_device_stats[args->lun].last_lba);
            g_device_stats[args->lun].last_lba = 
                g_device_stats[args->lun].last_wr_lba = lba+len;
        } else if (is_read) {
            g_device_stats[args->lun].jumps_read += 
                abs(lba-g_device_stats[args->lun].last_rd_lba);
            g_device_stats[args->lun].jumps += 
                abs(lba-g_device_stats[args->lun].last_lba);
            g_device_stats[args->lun].last_lba = 
                g_device_stats[args->lun].last_rd_lba = lba+len;
        } 
    }

    /* execute directly if the lun is invalid */
    if (args->lun>=g_disk_num_luns) return device_command(cmd);

    /* else give to a lun worker */
    if (iscsi_queue_insert(&g_cmd_queue[args->lun], cmd)!=0) {
        TRACE_ERROR("LUN %"PRIu64" CMD QUEUE FULL - EXITING\n", args->lun);
        exit(1);
    }

    /* wake up a worker that's not busy */
    for (i=0; i<g_workers_per_lun; i++) {
        if (!HAS_WORK(&g_lun_worker[args->lun][i])) {
            //printf("%i: waking up worker %i\n", getpid(), i);
            ISCSI_WAKE_ELSE(SET_WORK(&g_lun_worker[args->lun][i]), 
                            &g_lun_worker[args->lun][i].wq, return -1);
            //printf("%i: worker %i all done!\n", getpid(), i);
            return 0;
        }
    }

    return 0;
}

int device_command(TARGET_CMD_T *cmd) {
    ISCSI_SCSI_CMD_T *args = &(cmd->scsi_cmd);
    unsigned lba;
    unsigned short len;
    unsigned char data[4096];
    unsigned char *cdb = args->cdb;
    unsigned char lun = args->lun;
    unsigned cdb_lun = cdb[1]>>5;
    int done = 0;
    int override_gather = 0;
    static int fail = 0;
    int i,k;
    static uint64_t opcount = 0;
    struct timeval arrival;

    gettimeofday(&arrival, NULL);

    assert(cmd->gather_len == 0);
    assert(cmd->scatter_len == 0);
    assert(args->recv_data == NULL);
    assert(args->recv_sg_len == 0);
    assert(args->send_data == NULL);
    assert(args->send_sg_len == 0);
 
    TRACE(TRACE_SCSI, 1, 
          "session %i: received SCSI op 0x%x (tag 0x%x, lun %u)\n", 
          cmd->sess->id, args->cdb[0], args->tag, lun);

    /* check for correct lun */    
    if (lun != cdb_lun) {
        PRINT("LUN mismatch (iSCSI says %u, CDB says %u)\n", lun, cdb_lun);
    }
    if ((lun>=g_disk_num_luns)&&(cdb[0] == INQUIRY)) {
        memset(data, 0, cdb[4]);
        data[0] = 0x60; // peripheral qualifier
        data[0] |= 0x7F;  // no device
        if (cdb[4] != args->trans_len) {
            TRACE_WARN("WARN: cdb alloc length (%u) != iSCSI trans len (%u)\n", 
                       cdb[4], args->trans_len);
        }
        args->fromdev = 1;
        args->status = 0;
        args->length = args->trans_len;
        TRACE(TRACE_SCSI, 1, "SCSI op 0x%x (lun %u): Invalid lun\n", 
              cdb[0], lun);
        done = 1; goto done;
    } else if (lun>=g_disk_num_luns) {
        args->status = 1;
        args->fromdev = 0;
        args->length = 0;
        TRACE(TRACE_SCSI, 1, "SCSI op 0x%x (lun %u): Invalid lun\n", 
              cdb[0], lun);
        done = 1; goto done;
    }

    switch(cdb[0]) {
        
    case TEST_UNIT_READY:
        TRACE_CLEAN(TRACE_SCSI, 0, " TEST_UNIT_READY(lun %i)\n", lun);
        args->status = 0;
        args->length = 0;
        done = 1; 
        break;

    case INQUIRY:
        TRACE_CLEAN(TRACE_SCSI, 0, " INQUIRY(lun %i)\n", lun);
     
        memset(data, 0, cdb[4]);                  // Clear allocated buffer
        data[0] = 0;                              // Peripheral Device Type
        //data[1] |= 0x80;                        // Removable Bit
        data[2] |= 0x02;                          // ANSI-approved version
        //data[3] |= 0x80;                        // AENC 
        //data[3] |= 0x40;                        // TrmIOP 
        //data[3] |= 0x20;                        // NormACA 
        data[4] = cdb[4]-4;                       // Additional length 
        //data[7] |= 0x80;                        // Relative addressing 
        data[7] |= 0x40;                          // WBus32 
        data[7] |= 0x20;                          // WBus16
        //data[7] |= 0x10;                        // Sync 
        //data[7] |= 0x08;                        // Linked Commands
        //data[7] |= 0x04;                        // TransDis 
        if (CONFIG_LUN_QUEUE_DEPTH>1) {
            data[7] |= 0x02;                      // Tagged Command Queueing
        }
        //data[7] |= 0x01;                              // SftRe
        strcpy((char *)(data+8), "Intel   " );          // Vendor
        strcpy((char *)(data+16), "iSCSI Reference " ); // Product ID
        sprintf((char *)(data+32), "%s", "2   ");       // Product Revision

        if (cdb[4] != args->trans_len) {
            TRACE_WARN("cdb alloc len (%u) != iSCSI trans len (%u)\n", 
                       cdb[4], args->trans_len);
        }
        args->fromdev = 1;
        args->status = 0;
        args->length = args->trans_len;

        done = 1; 
        break;

    case READ_CAPACITY:
        TRACE_CLEAN(TRACE_SCSI, 0, " READ_CAPACITY(lun %i)\n", lun);

        *((unsigned *) data) = HTONL(g_disk_num_blocks-1);   // Max LBA
        *((unsigned *) (data+4)) = HTONL(g_disk_block_len);  // Block len
        args->fromdev = 1;
        args->length = 8;
        args->status = 0;
        done = 1; 
        break;

    case WRITE_6:
        lba = NTOHL(*((unsigned *)cdb))&0x001fffff;
        len = cdb[4];
        if (!len) len = 256;
        TRACE_CLEAN(TRACE_SCSI, 0, "WRITE_6(lun %i lba %8u blocks %3u)\n", 
                    lun, lba, len);

        ISCSI_SPIN_LOCK_ELSE(&g_device_stats[lun].lock, exit(1));
        if (g_fptr[lun] && g_tracing) fprintf(g_fptr[lun], "%.6lf w %"PRIu64
                                 " %u\n",
                                 arrival.tv_sec + arrival.tv_usec/1e6,
                                 (uint64_t) lba*g_disk_block_len, 
                                 len*g_disk_block_len);
        ISCSI_SPIN_UNLOCK_ELSE(&g_device_stats[lun].lock, exit(1));

        if (prepare_write(cmd, lun, lba, len)!=0) {
            TRACE_ERROR("prepare_write() failed\n");
            args->status = 0x01;
        }
        args->length = 0;
        cmd->is_write = 1;

        break;

    case READ_6:
        lba = NTOHL(*((unsigned *)cdb))&0x001fffff;
        len = cdb[4];
        if (!len) len = 256;
        TRACE_CLEAN(TRACE_SCSI, 0, "READ_6(lun %i lba %8u blocks %3u)\n", 
                    lun, lba, len);

        ISCSI_SPIN_LOCK_ELSE(&g_device_stats[lun].lock, exit(1));
        if (g_fptr[lun] && g_tracing) fprintf(g_fptr[lun], "%.6lf r %"PRIu64" %u\n",
                                 arrival.tv_sec + arrival.tv_usec/1e6,
                                 (uint64_t) lba*g_disk_block_len, 
                                 len*g_disk_block_len);
        ISCSI_SPIN_UNLOCK_ELSE(&g_device_stats[lun].lock, exit(1));

        if ((args->length=disk_read(cmd, lun, lba, len))==-1) {
            TRACE_ERROR("disk_read() failed\n");
            args->status = 2;
            args->length = 0;
        } else {
            args->status = 0;
            args->fromdev = 1;
            override_gather = 1;
        }
        done = 1; 

        break;

    case WRITE_10:

        // Some platforms (like strongarm) align on
        // word boundaries.  So HTONL and NTOHL won't
        // work here.
        
        cmd->is_write = 1;
        
#if (BYTE_ORDER == BIG_ENDIAN)
        ((unsigned char *)&lba)[0] = cdb[2];
        ((unsigned char *)&lba)[1] = cdb[3];
        ((unsigned char *)&lba)[2] = cdb[4];
        ((unsigned char *)&lba)[3] = cdb[5];
        ((unsigned char *)&len)[0] = cdb[7];
        ((unsigned char *)&len)[1] = cdb[8];
#else
        ((unsigned char *)&lba)[0] = cdb[5];
        ((unsigned char *)&lba)[1] = cdb[4];
        ((unsigned char *)&lba)[2] = cdb[3]; 
        ((unsigned char *)&lba)[3] = cdb[2];
        ((unsigned char *)&len)[0] = cdb[8];
        ((unsigned char *)&len)[1] = cdb[7];
#endif
        
        TRACE_CLEAN(TRACE_SCSI, 0, 
                    "WRITE_10(sess %i tag 0x%x lun %u lba %8u blocks %3u)\n", 
                    cmd->sess->id, args->tag, lun, lba, len);
	if (g_write_sleep) usleep(g_write_sleep);
        ISCSI_SPIN_LOCK_ELSE(&g_device_stats[lun].lock, exit(1));
        if (g_fptr[lun] && g_tracing) fprintf(g_fptr[lun], "%.6lf w %"
                                 PRIu64" %u\n",
                                 arrival.tv_sec + arrival.tv_usec/1e6,
                                 (uint64_t) lba*g_disk_block_len,
                                 len*g_disk_block_len);
        ISCSI_SPIN_UNLOCK_ELSE(&g_device_stats[lun].lock, exit(1));
        if (prepare_write(cmd, lun, lba, len)!=0) {
            TRACE_ERROR("prepare_write() failed\n");
            args->status = 2;
        }
        args->length = 0;

        break;

    case READ_10:

        // Some platforms (like strongarm) align on
        // word boundaries.  So HTONL and NTOHL won't
        // work here.

#if (BYTE_ORDER == BIG_ENDIAN)
        ((unsigned char *)&lba)[0] = cdb[2];
        ((unsigned char *)&lba)[1] = cdb[3];
        ((unsigned char *)&lba)[2] = cdb[4];
        ((unsigned char *)&lba)[3] = cdb[5];
        ((unsigned char *)&len)[0] = cdb[7];
        ((unsigned char *)&len)[1] = cdb[8];
#else
        ((unsigned char *)&lba)[0] = cdb[5];
        ((unsigned char *)&lba)[1] = cdb[4];

        ((unsigned char *)&lba)[2] = cdb[3];
        ((unsigned char *)&lba)[3] = cdb[2];
        ((unsigned char *)&len)[0] = cdb[8];
        ((unsigned char *)&len)[1] = cdb[7];
#endif

        TRACE_CLEAN(TRACE_SCSI, 0, 
                    " READ_10(sess %i tag 0x%x lun %u lba %8u blocks %3u)\n",
                    cmd->sess->id, args->tag, lun, lba, len);
        ISCSI_SPIN_LOCK_ELSE(&g_device_stats[lun].lock, exit(1));
        if (g_fptr[lun] && g_tracing) fprintf(g_fptr[lun], "%.6lf r %"
                                 PRIu64" %u\n",
                                 arrival.tv_sec + arrival.tv_usec/1e6,
                                 (uint64_t) lba*g_disk_block_len, 
                                 len*g_disk_block_len);
        ISCSI_SPIN_UNLOCK_ELSE(&g_device_stats[lun].lock, exit(1));

        if ((args->length=disk_read(cmd, lun, lba, len))==-1) {
            TRACE_ERROR("disk_read() failed\n");
            exit(1);
            args->status = 2;
            args->length = 0;
        } else {
            args->status = 0;
            args->fromdev = 1;
            override_gather = 1;
        }
        done = 1;
        break;

    case VERIFY:
        TRACE_CLEAN(TRACE_SCSI, 0, "VERIFY(lun %u)\n", lun);
        args->status = 0;
        done = 1; 
        break;

    case MODE_SENSE:
        TRACE_CLEAN(TRACE_SCSI, 0, " MODE_SENSE(lun %u)\n", lun);
        memset(data, 0, 4+cdb[4]);
        data[0] = 0x19;
        data[4+0] = 0x03;
        data[4+1] = 0x16;
        *((unsigned *)(data+4+12)) = htons(g_disk_block_len);
        data[4+20] = 0x80;
        args->fromdev = 1; 
        args->length = cdb[4];
        args->status = 0; 
        done = 1; 
        break;

    case 0xa0:
        TRACE_CLEAN(TRACE_SCSI, 0, " REPORT_LUNS\n");
        memset(data, 0, args->trans_len);
        *((unsigned *)data) = HTONL(g_disk_num_luns*8); // lun list length
        for (i=0; i<g_disk_num_luns; i++) {
            *((unsigned char *)(data+8+8*i)) = 0x0;   // single level address
            *((unsigned char *)(data+8+8*i+1)) = i;   // lun
        }
        args->status = 0x0;
        args->trans_len = args->length = 8+8*g_disk_num_luns;
        args->fromdev = 1;
        done = 1; 
        break;

    case ISCSI_IOCTL_TARGET_EXIT:
        fprintf(stderr, "0x%x: DEVICE_EXIT\n", args->tag);
        done = 1; args->status = 0; args->length = 0;

        break;

    case ISCSI_IOCTL_TRACING_ON:
        PRINT("Tracing ON (tracefile = \"%s\")\n", g_tracefile);
        g_tracing=1;
        args->status = 0;
        args->length = 0;
        done = 1; 
        break;

    case ISCSI_IOCTL_TRACING_OFF:
        PRINT("Tracing OFF (tracefile = \"%s\")\n", g_tracefile);
        g_tracing=0;
        args->status = 0;
        args->length = 0;
        done = 1; 
        break;

    case ISCSI_IOCTL_TRACING_SET:
        g_tracing=1;
        if (prepare_write(cmd, lun, 0, 0)!=0) {
            TRACE_ERROR("prepare_write() failed\n");
            args->status = 0x02;
            done = 1; 
        }
        args->length = 0;

        break;

    default:
        TRACE_ERROR("UNKNOWN OPCODE 0x%x (lun %u)\n", cdb[0], lun); 
        args->status = 2;
        args->length = 0;
        done = 1; 
        break;
    }

 done:
    if (done) {
        if (args->fromdev && args->length && !override_gather) {
            cmd->gather_list[0].iov_base = data;
            cmd->gather_list[0].iov_len = args->length;
            cmd->gather_type[0] = MEM_STACK;
            cmd->gather_len = 1;
        }
        return command_done(cmd);
    }
    
    return 0;
}

/**********************
 * Internal functions *
 **********************/

void print_sg_list(unsigned len, struct iovec *vec) {
}

static void *write_allocate_space(TARGET_CMD_T *cmd, unsigned char lun, 
                                  uint64_t lba, 
			    unsigned len, unsigned scatter_index) {
    ISCSI_SCSI_CMD_T *args = &cmd->scsi_cmd;
    uint64_t offset = lba*g_disk_block_len;
    uint32_t num_bytes = len*g_disk_block_len;
    void *ptr = NULL;
    unsigned align_len;
    
    /* allocate space for write data */
    if (g_type == DISK_RAM) {
        ptr = g_ramdisk[lun]+offset;
    } else if ((g_type == DISK_RAW)||(g_type == DISK_SG)||
               (g_type == DISK_CTRL)||((g_type == DISK_SD)&&g_direct)) {

        /* temporary staging buffer */
        if ((ptr=iscsi_queue_remove(&g_staging_queue[lun]))==NULL) {
            TRACE_ERROR("no staging buffers available\n");
            return NULL;
        }
        cmd->scatter_type[scatter_index] = MEM_STAGING;

        if (len*g_disk_block_len>CONFIG_DISK_MAX_BURST) {
            TRACE_ERROR("maximum burst size exceeded (%u>%u)\n", 
                        len*g_disk_block_len, CONFIG_DISK_MAX_BURST);
            return NULL;
        }
    } else if ((g_type == DISK_FS)||(g_type == DISK_SD)) {
        int fid;

        align_len = offset%4096;
        fid = g_fid[lun][cmd->worker_id][0];
        if ((ptr = mmap(0, num_bytes+align_len, PROT_WRITE, MAP_SHARED, 
                        fid, offset-align_len))==NULL) {
            TRACE_ERROR("mmap() failed: %i\n", errno);
            exit(1);
        }
        ptr += align_len;
        TRACE(TRACE_SCSI, 1, "mapped scatter_list[%u] (%p bytes %u align %u)\n",
              scatter_index, ptr, num_bytes+align_len, align_len);
        cmd->scatter_type[scatter_index] = MEM_MAPPED;
        cmd->scatter_align[scatter_index] = align_len;
    } else {
        TRACE_ERROR("unknown disk type\n");
        exit(1);
    }

    /* private data for device_place_data() */
    args->lba = lba;
    args->len = len;

    return ptr;
}

static int prepare_write(TARGET_CMD_T *cmd, unsigned char lun, uint64_t lba, 
                         unsigned len) {
    ISCSI_SCSI_CMD_T *args = &cmd->scsi_cmd;
    uint32_t num_bytes = len*g_disk_block_len;
    void *ptr = NULL;
    unsigned request_marked = 0;

    if (args->cdb[0] == ISCSI_IOCTL_TRACING_SET) {
        cmd->scatter_list[0].iov_base = g_tracefile;
        cmd->scatter_list[0].iov_len = CONFIG_MAX_FILENAME;
        cmd->scatter_len = 1;
        goto transfer;
    }

    /* check that we're not writing past the disk */
    if ((lba>(g_disk_num_blocks-1))||((lba+len)>g_disk_num_blocks)) {
        TRACE_ERROR("attempt to write beyond end of media\n");
        TRACE_ERROR("max_lba = %"PRIu64", requested lba = %"PRIu64", "
                    "len = %u\n", g_disk_num_blocks-1, lba, len);
        return -1;
    } 
    if ((ptr = write_allocate_space(cmd, lun, lba, len, 0)) == NULL) {
        return -1;
    }
    cmd->scatter_list[0].iov_base = ptr; 
    cmd->scatter_list[0].iov_len = num_bytes;
    cmd->scatter_len = 1;

    /* copy in any immediate data */
transfer: if (args->length) {
	uint64_t bytes_to_copy = args->length;
	uint64_t size;
	uint64_t index = 0;
	while (bytes_to_copy > 0) {
	    ASSERT(index < cmd->scatter_len);
	    size = MIN(cmd->scatter_list[index].iov_len, bytes_to_copy);
	    memcpy(cmd->scatter_list[index].iov_base, 
		   cmd->immediate + (args->length - bytes_to_copy), size);
	    bytes_to_copy -= size;
	    index += 1;
	}
	TRACE(TRACE_SCSI, 1, "copied in %u bytes immediate data\n", 
              args->length);
    }
    args->bytes_todev = args->length;

    /* request transfer from target system - as the data becomes available,
     * the target code will call device_place_data(). */
    if (args->todev && (args->trans_len-args->bytes_todev)) {
        TRACE(TRACE_SCSI, 1, 
              "requesting transfer of %u bytes write data (tag 0x%x)\n", 
              num_bytes-args->length, cmd->scsi_cmd.tag);
        if (target_transfer_data(cmd)!=0) {
            TRACE_ERROR("target_transfer_data() failed\n");
            return -1;
        }
    } else {
        if (command_done(cmd)!=0) {
            TRACE_ERROR("command_done() failed\n");
            return -1;
        }
    }

    return 0;
}

static void *read_allocate_space(TARGET_CMD_T *cmd, unsigned char lun, 
				 uint64_t lba, unsigned short len, 
                                 unsigned index) {
    uint64_t byte_offset = lba*g_disk_block_len;
    uint32_t num_bytes = len*g_disk_block_len;
    int align_len;
    void *ptr;
    int fid;

    if (g_type == DISK_RAM) {
        ptr = g_ramdisk[lun]+byte_offset;
    } else if ((g_type==DISK_FS)||(g_type==DISK_SD)) {
        align_len = byte_offset%4096;
        fid = g_fid[lun][cmd->worker_id][0];
        if ((ptr = mmap(0, num_bytes+align_len, PROT_READ, 
                        MAP_SHARED, fid,
                        byte_offset-align_len))==NULL) {
            TRACE_ERROR("mmap() failed\n");
            exit(1);
        }
        TRACE(TRACE_SCSI, 1, "mapped gather_list[index] (%p len %u align %u)\n",
              ptr, num_bytes+align_len, align_len);
        ptr += align_len;
        cmd->gather_type[index] = MEM_MAPPED;
        cmd->gather_align[index] = align_len;
    } else if (g_type == DISK_RAW) {
        /* temporary staging buffer */
        if ((ptr=iscsi_queue_remove(&g_staging_queue[lun]))==NULL) {
            TRACE_ERROR("no staging buffers available\n");
            return NULL;
        }
        cmd->gather_type[index] = MEM_STAGING;
        fid = g_fid[lun][cmd->worker_id][0];
        if (lseek(fid, byte_offset, SEEK_SET) == -1) {
            TRACE_ERROR("error seeking \"%s\"\n", g_device);
            exit(1);
        }
        if (read(fid, ptr, num_bytes) == -1) {
            TRACE_ERROR("error reading \"%s\" (errno %i)", g_device, errno);
            exit(1);
        }
        TRACE(TRACE_SCSI, 1, 
              "rawdisk: read %u bytes at offset %"PRIu64" (id %i)\n", 
              num_bytes, byte_offset, cmd->worker_id);
    } else if ((g_type == DISK_SG)||(g_type == DISK_CTRL)) {
        char cdb[10];

        memset(cdb, 0, 10);
        cdb[0] = 0x28;
        *((unsigned *)(cdb+2)) = htonl(lba);
        *((unsigned short *)(cdb+7)) = htons(len);

        /* temporary staging buffer */
        if ((ptr=iscsi_queue_remove(&g_staging_queue[lun]))==NULL) {
            TRACE_ERROR("no staging buffers available\n");
            return NULL;
        }
        cmd->gather_type[index] = MEM_STAGING;
        if (g_type == DISK_SG) {
            fid = g_fid[lun][cmd->worker_id][0];
            if (sg_cmd_ioctl(fid, cdb, 10, 
                             ptr, num_bytes, SG_DXFER_FROM_DEV, 10000)!=0) {
                TRACE_ERROR("READ_10 failed\n");
                return NULL;
            }
            TRACE(TRACE_SCSI, 1, 
                  "rawdisk: read %u bytes at lba %"PRIu64"=offset %"PRIu64" "
                  "(id %i)\n", num_bytes, lba, byte_offset, cmd->worker_id);
        } else {
            struct iovec sg[1];
            int sg_len;
            int num_blocks;
            int timeout = 1;
            int writing = 0;
            uint64_t lun;
            int lba;
            int i;
            char tmp[4096];

            PRINT("%i: ***DOING CONTROLLER READ***\n", OST_GETPID);

            /* The data needs to be read into the ptr staging buffer
               (allocated above). Right now we just read one block from
               each disk (round-robin) into a tmp buffer. */
            for (i=0; i<g_tids; i++) {
                sg[0].iov_base = tmp; 
                sg[0].iov_len = g_block_len[i]; 
                sg_len = 1;
                num_blocks = 1;
                lun = 0;
                lba = 0;
                if (read_or_write(i, lun, lba, num_blocks, g_block_len[i], 
                                  (unsigned char *) sg, sg_len, 
                                  writing, timeout)!=0) {
                    PRINT("I/O to tid %i failed -- ignoring\n", i);
                }
            }
        }
    } else {
        TRACE_ERROR("unknown type\n");
        exit(1);
    }

    /* private data (need these for in command_done() */
    cmd->scsi_cmd.lba = lba;
    cmd->scsi_cmd.len = len;

    return ptr;
}    

/*
 * need to updated controller.c accordingly
 */

static int disk_read(TARGET_CMD_T *cmd, unsigned char lun,
                     uint64_t lba, unsigned short len) {
    uint32_t num_bytes = len*g_disk_block_len;
    unsigned request_marked = 0;
    void *ptr = NULL;
    
    /* check args */
    RETURN_EQUAL("len", len, 0, NO_CLEANUP, -1);
    if ((lba>(g_disk_num_blocks-1))||((lba+len)>g_disk_num_blocks)) {
        TRACE_ERROR("attempt to read beyond end of media\n");
        TRACE_ERROR("max_lba=%"PRIu64", requested lba=%"PRIu64", len=%u\n", 
                    g_disk_num_blocks-1, lba, len);
        return -1;
    } 
    
    if (len*g_disk_block_len>CONFIG_DISK_MAX_BURST) {
        TRACE_ERROR("maximum burst size exceeded (%u>%u)\n", 
                    len*g_disk_block_len, CONFIG_DISK_MAX_BURST);
        return -1;
    }
    
    /* read the data */
    if ((ptr = read_allocate_space(cmd, lun, lba, len, 0)) == NULL) {
        return -1;
    }
    cmd->gather_list[0].iov_base = ptr; 
    cmd->gather_list[0].iov_len = num_bytes;
    cmd->gather_len = 1;

    return num_bytes;
}

static int command_done(TARGET_CMD_T *cmd) {
    int i;
    void *ptr;
    uint32_t lun = cmd->scsi_cmd.lun;
    uint64_t lba = cmd->scsi_cmd.lba;
    unsigned len;

    TRACE(TRACE_SCSI, 1,
          "session %i: completed SCSI op 0x%x "
          "(status 0x%x, tag 0x%x, is_write %i)\n",
          cmd->sess->id, cmd->scsi_cmd.cdb[0], cmd->scsi_cmd.status,
          ISCSI_ITT(cmd->header), cmd->is_write);

    if (cmd->scsi_cmd.cdb[0] == ISCSI_IOCTL_TRACING_SET) {
        if (cmd->scsi_cmd.status) { 
            PRINT("I/O trace set failed (status %u)\n",
                  cmd->scsi_cmd.status);
        } else {
            PRINT("I/O trace set to \"%s\"\n", g_tracefile);
            if ((g_fptr[lun]=fopen(g_tracefile, "w+"))==NULL) {
                fprintf(stderr, "error creating tracefile \"%s\"\n", 
                        g_tracefile);
                return -1;
            }
        }
    }

    /* write out data */
    if (cmd->is_write) {
        int fd = g_fid[cmd->scsi_cmd.lun]
                      [cmd->worker_id]
                      [0];
        uint64_t lba = cmd->scsi_cmd.lba;
        uint64_t len = cmd->scsi_cmd.len;
        char cdb[10];
        char *ptr = cmd->scatter_list[0].iov_base;
        unsigned num_bytes = len*g_disk_block_len;

        if ((g_type == DISK_RAW) || ((g_type==DISK_SD) && g_direct)) {
            if (lseek(fd, lba*g_disk_block_len, SEEK_SET) == -1) {
                TRACE_ERROR("error seeking \"%s\"\n", g_device);
                exit(1);            
            }
            if (writev(fd, cmd->scatter_list, cmd->scatter_len) == -1) {
                TRACE_ERROR("error writing \"%s\" (errno %i)", g_device, errno);
                exit(1);
            }     
            TRACE(TRACE_SCSI, 1, 
                  "rawdisk: wrote %"PRIu64" bytes ok at offset %"PRIu64" "
                  "(fid %i)\n", len*g_disk_block_len,  lba*g_disk_block_len, 
                  cmd->worker_id);
        } else if (g_type == DISK_SG) {
            memset(cdb, 0, 10);
            cdb[0] = 0x2a;
            *((unsigned *)(cdb+2)) = htonl(lba);
            *((unsigned short *)(cdb+7)) = htons(len);
            if (sg_cmd_ioctl(fd, cdb, 10, ptr, num_bytes, SG_DXFER_TO_DEV, 
                             10000)!=0) {
                TRACE_ERROR("WRITE_10 failed\n");
                exit(-1);
            }
            TRACE(TRACE_SCSI, 1, 
                  "sgdisk: wrote %"PRIu64" bytes ok at lba %"PRIu64" = "
                  "offset %"PRIu64" (fid %i)\n", len*g_disk_block_len,  lba, 
                  lba*g_disk_block_len, cmd->worker_id);
        } else if (g_type == DISK_CTRL) {
            int timeout = 1;
            int writing = 1;
            int num_blocks;
            uint64_t lun = 0;
            int i;

            PRINT("***DOING CONTROLLER WRITE***\n");

            /* The data to be written is in cmd->scatter_list. Right now
               we just write block 0 of each disk, starting at the same
               offset (0) in the scatter list.  This will of course need
               to be changed so that the correct data is written to the correct
               lba of each disk */

            for (i=0; i<g_tids; i++) {
                num_blocks = 1;
                lba = 0;
                if (read_or_write(i, lun, lba, num_blocks, g_block_len[i], 
                                  (unsigned char *) cmd->scatter_list, 
                                  cmd->scatter_len, writing, timeout)!=0) {
                    PRINT("Write I/O to tid %i failed -- ignoring\n", i);
                }
            }
        }
    }

    /* notify target that we're done (we grab the lock to keep
       the StatSN in order */
    done:  ISCSI_LOCK_ELSE(&g_device_stats[lun].lock, exit(1));
    if (cmd->callback == NULL) {
        TRACE_ERROR("cmd->callback is NULL!?\n");
        exit(1);
    }
    if (cmd->callback(cmd->callback_arg)!=0) {
        TRACE_ERROR("target callback failed for 0x%x - EXITING\n", 
                    ISCSI_ITT(cmd->header));
        exit(1);
    }
    ISCSI_UNLOCK_ELSE(&g_device_stats[lun].lock, exit(1));

    /* clean up after command */

    if ((g_type==DISK_SG)||(g_type==DISK_RAW)||(g_type==DISK_CTRL)||
        ((g_type==DISK_SD)&&g_direct)) {
        uint32_t lun = cmd->scsi_cmd.lun;


        for (i=0; i<cmd->scatter_len; i++) {
	    ptr = cmd->scatter_list[i].iov_base;
	    switch (cmd->scatter_type[i]) {
	    case (MEM_STACK):
		break;          
	    case (MEM_STAGING):
		ptr = cmd->scatter_list[i].iov_base;
		iscsi_queue_insert(&g_staging_queue[lun], ptr);
		break;
	    }  
        }
        for (i=0; i<cmd->gather_len; i++) {
            switch (cmd->gather_type[i]) {
            case (MEM_STACK):
                break;          
            case (MEM_STAGING):
                ptr = cmd->gather_list[i].iov_base;
                iscsi_queue_insert(&g_staging_queue[lun], ptr);
                break;
            }  
        }
    } else  if ((g_type==DISK_FS)||(g_type==DISK_SD)) { 
        for (i=0; i<cmd->scatter_len; i++) {
            switch (cmd->scatter_type[i]) {
            case (MEM_STACK):
                break;
            case (MEM_MAPPED):
                ptr = cmd->scatter_list[i].iov_base-cmd->scatter_align[i];
                len = cmd->scatter_list[i].iov_len+cmd->scatter_align[i];
                TRACE(TRACE_SCSI, 1, 
                      "unmapping scatter_list[%i] (%p len %u align %u)\n", 
                      i, ptr, len, cmd->scatter_align[i]);              
                if (g_sync) {
                    if (msync(ptr, len, MS_SYNC)!=0) {
                        TRACE_ERROR("msync() failed\n");
                        return -1;
                    }
                }
                if (munmap(ptr, len)!=0) {
                    TRACE_ERROR("munmap() failed\n");
                    return -1;
                }
                break;
            default:
                TRACE_ERROR("unknown memory type %i\n", cmd->gather_type[i]);
                exit(1);            
            }
        }
        for (i=0; i<cmd->gather_len; i++) {
            switch (cmd->gather_type[i]) {
            case (MEM_STACK):
                break;
            case (MEM_MAPPED):
                ptr = cmd->gather_list[i].iov_base-cmd->gather_align[i];
                len = cmd->gather_list[i].iov_len+cmd->gather_align[i];
                TRACE(TRACE_SCSI, 1, 
                      "unmapping gather_list[%i] (%p len %u align %u)\n", 
                      i, ptr, len, cmd->gather_align[i]);
                if (munmap(ptr, len)!=0) {
                    TRACE_ERROR("munmap() failed\n");
                    return -1;
                }
                break;
            default:
                TRACE_ERROR("unknown memory type %i\n", cmd->gather_type[i]);
                exit(1);            
            }
        }
    }

    /* DEVICE EXIT */
    finish: 

    if (cmd->scsi_cmd.cdb[0] == ISCSI_IOCTL_TARGET_EXIT) {
        char tmp[1024];

        if (g_tracing) {
            for (i=0; i<g_disk_num_luns; i++) {
		if (g_fptr[i]) {
                    printf("closing g_fptr[%i]\n", i);
                    fclose(g_fptr[i]);
                }
            }
        }

        sprintf(tmp, "rm /tmp/UDISK.%i", g_port);
        fprintf(stderr, "exiting from utest -E\n");
        system(tmp);
        exit(1);
    }
 
    return 0;
}

static int stats_worker_proc(ISCSI_WORKER_T *me) {
    char data[4096];
    char filename[256];
    FILE *stream;

    sprintf(filename, "/tmp/disk_counters");
    if ((stream=fopen(filename, "a+"))==NULL) {
        TRACE_ERROR("error opening %s\n", filename);
        return -1;
    }
    PRINT("counter output: %s\n", filename);

    START_ELSE(me, 1, EXIT(me, -1));
    while (!WORKER_STOPPING(me)) {
        read_counters(0, data);
        fprintf(stream, data);
        fflush(stream);
        ISCSI_SLEEP(g_counters);
    }
    fclose(stream);   
    EXIT(me, 0);
}

static int lun_worker_proc(ISCSI_WORKER_T *me) {
    TARGET_CMD_T *cmd;
    ISCSI_SCSI_CMD_T *args;
    LUN_WORKER_ARGS_T *worker_args = me->ptr;

    worker_args->lun +=0;
    START_ELSE(me, 1, EXIT(me, -1));
    while (1) { 
        ISCSI_WAIT(&me->wq, WORKER_STOPPING(me)||
                   (HAS_WORK(me)||
                    iscsi_queue_depth(&g_cmd_queue[worker_args->lun])));
        if (WORKER_STOPPING(me)) {
            TRACE(TRACE_WORKER, 0, "got shutdown signal\n");
            break;
        } 
        if ((cmd=iscsi_queue_remove(&g_cmd_queue[worker_args->lun]))!=NULL) {
            TRACE(TRACE_SCSI, 1, "lun worker %i removed cmd 0x%x (%p)\n", 
                  worker_args->id, cmd->scsi_cmd.tag, cmd);
            cmd->worker_id = me->id;
            args = &(cmd->scsi_cmd);
            if (cmd->done) {
                command_done(cmd);
            } else if (device_command(cmd)!=0) {
                TRACE_ERROR("device_command() failed - ignoring\n");
            } 
        }
        CLEAR_WORK(me);
    }
    EXIT(me, 0);
}

static int sg_cmd_ioctl(int fid, char *cmd, int cmd_len, char *data, 
                        int data_len, int direction, int timeout) {
    unsigned char sense[256];
    sg_iovec_t iovec;
    sg_io_hdr_t sg_hdr;
    int rc;

    memset(&sg_hdr, 0, sizeof(struct sg_io_hdr));
    iovec.iov_base = data;
    iovec.iov_len = data_len;
    sg_hdr.interface_id = 'S';
    sg_hdr.dxfer_direction = direction;
    sg_hdr.cmd_len = cmd_len;
    sg_hdr.mx_sb_len = 255;
    sg_hdr.iovec_count = 1;
    sg_hdr.dxfer_len = iovec.iov_len;
    sg_hdr.dxferp = &iovec;
    sg_hdr.cmdp = (unsigned char *) cmd;
    sg_hdr.sbp = sense;
    sg_hdr.timeout = timeout;
    sg_hdr.flags = 0;
    sg_hdr.pack_id = 1;
    sg_hdr.usr_ptr = NULL;
    if ((rc=ioctl(fid, SG_IO, &sg_hdr))!=0) {
        fprintf(stderr, "error sending ioctl (errno %i)\n", errno);
        return -1;
    }

#if 0
    printf("       status: %u\n", sg_hdr.status);
    printf("masked_status: %u\n", sg_hdr.masked_status);
    printf("   msg_status: %u\n", sg_hdr.msg_status);
    printf("    sb_len_wr: %u\n", sg_hdr.sb_len_wr);
    printf("  host_status: %u\n", sg_hdr.host_status);
    printf("driver_status: %u\n", sg_hdr.driver_status);
    printf("        resid: %i\n", sg_hdr.resid);
    printf("     duration: %u\n", sg_hdr.duration);
    printf("         info: %u\n", sg_hdr.info);
#endif

    if (sg_hdr.resid) return -1;
    return sg_hdr.status;
}
