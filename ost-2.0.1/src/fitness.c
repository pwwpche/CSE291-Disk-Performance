
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
 *  list of conditions and the followin disclaimer.
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

#if 1
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/time.h>
#include <math.h>
#include <errno.h>
#include <sys/wait.h>
#include <regex.h>
#include <inttypes.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>
#include "initiator.h"
#include "util.h"
#include "debug.h"
#include "research.h"
#ifdef CONFIG_MPI
#include "mpi.h"
#endif
#include "config.h"

/*
 * Modify these constants to configure the random sampling. If the
 * stepping type is exponential (EXP), the STEP_ macro is ignored,
 * but it still must be non-zero.
 */

#define LIN 1
#define EXP 2
#define MAX_REQUEST_SIZE 256

#define     MIN_WR 0
#define     MAX_WR 100
#define    STEP_WR 20
#define    TYPE_WR LIN

#define   MIN_WRND 0
#define   MAX_WRND 100
#define  STEP_WRND 20
#define  TYPE_WRND LIN

#define   MIN_RRND 0
#define   MAX_RRND 100
#define  STEP_RRND 20
#define  TYPE_RRND LIN

#define    MIN_STR 1
#define    MAX_STR 1
#define   STEP_STR 1
#define   TYPE_STR LIN

#define  MIN_THINK 0
#define  MAX_THINK 0
#define STEP_THINK 1000
#define TYPE_THINK LIN

#define   MIN_WRSZ 1
#define   MAX_WRSZ MAX_REQUEST_SIZE
#define  STEP_WRSZ 1
#define  TYPE_WRSZ LIN

#define   MIN_RDSZ 1
#define   MAX_RDSZ MAX_REQUEST_SIZE
#define  STEP_RDSZ 1
#define  TYPE_RDSZ LIN

#define   MIN_QDEP 1
#define   MAX_QDEP 64
#define  STEP_QDEP 1
#define  TYPE_QDEP EXP

#define MAX_COV_SAMPLES 32

/*
 * Defaults
 */

#define DFLT_CAP   128
#define DFLT_SEED 1234
#define DFLT_SAMPLES 1
#define DFLT_SKIP 0
#define DFLT_ITERS 1
#define DFLT_WARM 10
#define DFLT_TEST 10
#define DFLT_DISP 0
#define DFLT_I_COUNTERS 0
#define DFLT_T_COUNTERS 0
#define DFLT_COV_SAMPLES 3
#define DFLT_BREATHER 0
#define DFLT_ALLOW_SPARSE 0
#define DFLT_NO_CREATE 0
#define DFLT_BARRIER -1
#define DFLT_ISCSI_CONFIG IPS_CONFIG_FILE

#define MODE_MOUNT     0x01
#define MODE_FILE      0x02
#define MODE_DEVICE    0x04
#define MODE_ISCSI     0x08
#define MODE_COUNTERS  0x10 
#define MODE_RESET     0x20

/*
 * Macros
 */

#ifdef CONFIG_MPI
#define WAIT_TURN                                               \
    if (!g_no_sync && g_myid) {                                 \
        MPI_Status status;                                      \
        int from;                                               \
                                                                \
        if (MPI_Recv(&from, 1, MPI_INT, g_myid-1, 1234,         \
                     MPI_COMM_WORLD, &status) != MPI_SUCCESS) { \
            TRACE_ERROR("MPI_Recv failed\n");                   \
            exit(1);                                            \
        }                                                       \
        g_verbose = 1;                                          \
    }
#else
#define WAIT_TURN
#endif

#ifdef CONFIG_MPI
#define NOTIFY_AND_WAIT                                         \
    fflush(stdout); fflush(stderr); ISCSI_MSLEEP(100);          \
    if (!g_no_sync) {                                           \
        int to = g_myid+1;                                      \
        if (g_myid != (g_num_procs-1)) {                        \
            if (MPI_Send(&g_myid, 1, MPI_INT, to, 1234,         \
                         MPI_COMM_WORLD) != MPI_SUCCESS) {      \
                TRACE_ERROR("MPI_Send failed\n");               \
                exit(1);                                        \
            }                                                   \
        }                                                       \
        MPI_Barrier(MPI_COMM_WORLD);                            \
        g_verbose = (g_myid==0)?1:0;                            \
    }
#else
#define NOTIFY_AND_WAIT
#endif

#define PRINT_SAMPLES                           \
    fprintf(stderr, "[");                       \
    for (k=0; k<s.cov_samples; k++) {           \
        if (bw[k] == -1) {                      \
            fprintf(stderr, "%6s ", "*");       \
            flag = 1;                           \
        } else {                                \
            fprintf(stderr, "%6.2lf ", bw[k]);  \
            flag = 0;                           \
        }                                       \
    }                                           \
    fprintf(stderr, "\b]");

#define BACKSPACE_SAMPLES                                               \
    for (k=0; k<s.cov_samples; k++) fprintf(stderr, "\b\b\b\b\b\b\b");  \
    fprintf(stderr, "\b");

#define FPRINTF(args...) if (g_verbose) (fprintf(args))

#define I_COUNTERS_GET(TID, LUN, BUFF) {                \
        if (i_counters_get(TID, LUN, BUFF)!=0) {        \
            TRACE_ERROR("i_counters_get() failed\n");   \
            exit(1);                                    \
        }                                               \
    }

#define T_COUNTERS_GET(TID, LUN, BUFF) {                \
        if (t_counters_get(TID, LUN, BUFF)!=0) {        \
            TRACE_ERROR("t_counters_get() failed\n");   \
            exit(1);                                    \
        }                                               \
    }

#define OUTPUT_STATS(START, STOP, EXE, VAR, TYPE)       \
    if (output_stats(START, STOP, EXE, VAR, TYPE)!=0) { \
        TRACE_ERROR("output_stats() failed\n");         \
        exit(1);                                        \
    }                                                   \
                                                        \
    /*
     * Types
     */

typedef struct sample_counters_t {
    double warm;            // actual warm time (secs)
    double test;            // actual test time (secs)
    uint64_t wr_offset;     // current write offset (bytes)
    uint64_t rd_offset;     // current read offset (bytes)
    uint64_t ops_warm;      // total ops while warming
    uint64_t ops_test;      // total ops while testing
    uint64_t ops;           // total ops
    uint64_t lat_warm;      // sum of latencies (usec) while warming
    uint64_t lat_test;      // sum of latencies (usec) while testing
    uint64_t lat;           // sum of latencies (usec)
    uint64_t bytes_warm;    // total read or written while warming
    uint64_t bytes_test;    // total read or written while testing
    uint64_t bytes;         // total read or written
    uint64_t bytes_actual;  // completed (not just on wire)
    ISCSI_LOCK_T lock;      // for mutually exclusion on updates
} SAMPLE_COUNTERS_T;

typedef struct sample_t {
    int seed;
    int samples;
    int skip;
    int iters;
    int warm;                         // warm time in secs
    int warm_io;                      // warm for this many IOs
    int warm_mb;                      // warm for this many MB of data
    int test;                         // test time
    uint32_t qdep;                    // queue depth
    int wr;                           // write percent (0 to 100)
    uint32_t wrsz;                    // write request size (KB)
    uint32_t rdsz;                    // read request size (KB)
    int wrnd;                         // prcnt of writes that are random (0-100)
    int rrnd;                         // prcnt of reads that are random (0-100)
#if 0
    int wrsk_dist[10];                // average seek (KB) per write I/O
    int rdsk_dist[10];                // average seek (KB) per read I/O
#endif
    uint64_t cap;                     // working set size (MB)
    uint64_t disp;                    // displacement from lba 0 (MB)
    uint64_t term;                    // term sample after this much I/O (MB)
    int think;                        // think time between each I/O (usec)
    int str;                          // number of streams (1 or 2)
    char device[256];                 // target device (e.g., /dev/sd1)
    char file[256];                   // target file (e.g., /mnt/test/data)
    char tracefile[256];              // strace file to replay
#if 0
    int timing_accurate;              // attempt a timing accurate replay
#endif
    char mkfs[256];                   // dir to mount a new fs (e.g., /mnt/test)
    char flush[256];                  // command to run between each iteration
    char flush_args[256];             // args to pass to flush command
    char external[256];               // run after each iter (vs. sampling)
    char external_args[256];          // args to pass to external command
    char external_shutdown[256];      // cmdto run to shutdown external command
    int sync;                         // mount synchronously
    int direct;                       // open with O_DIRECT
    int i_counters;                   // get counters from /proc/scsi
    int i_counters_loop;              // repeatedly get counters /proc/scsi
    char i_counters_save[256];        // file to save counters in
    int t_counters;                   // get counters from target (experimental)
    int t_counters_loop;              // repeatedly get counters from target
    char t_counters_save[256];        // file to save counters in
    int breather;                     // time to rest between each iteration
    int mock;                         // don't actually do any I/O
    unsigned mode;                    // mode of operation
    int allow_sparse;                 // allow reads without previous writes
    int no_create;                    // don't do initial creating file
    SAMPLE_COUNTERS_T c;              // counters
    int barrier;                      // sync every this many ops
#if 0
    int filter;                       // apply sample filter
#endif
    uint32_t wr_stride;               // write stride size (KB)
    uint32_t rd_stride;               // read stride size (KB)
} SAMPLE_T;

/*
 * Globals
 */

ISCSI_WORKER_T worker[MAX_QDEP];
ISCSI_WORKER_T extern_worker[1];
int g_extern_pid;
char g_i_counters_start[ISCSI_COUNTERS_SIZE], 
    g_i_counters_stop[ISCSI_COUNTERS_SIZE];
char g_t_counters_start[ISCSI_COUNTERS_SIZE], 
    g_t_counters_stop[ISCSI_COUNTERS_SIZE];
int g_fid[MAX_QDEP];    /* each worker has its own file handle */
SAMPLE_T s;
int g_myid = 0;
uint32_t g_blocklen;
int g_tid = -1;         /* assume not an iSCSI device by default */
int g_lun = 0;
int g_reset = 0;
int g_verbose = 1;
int g_concurrent = 0;
int g_prepend = 0;      /* prepend node id to results file (for MPI) */  
int g_append = 0;       /* append hostname to results file (for MPI) */  
uint64_t g_cap = 0;     /* the actual size of the drive (in MB)      */
FILE *g_fptr = NULL;    /* shared file pointer for trace file        */
int g_traceline = 0;
int g_num_procs = 1;
int g_end_of_file = 0;
int g_end_of_program = 0;
struct timeval g_start_trace, g_start_time;
int g_flag;
#ifdef CONFIG_MPI
int g_extern_mastered = 0; /* launch extern through node 0 */
int g_scale_disp = 0;      /* scale --disp by node's rank  */
int g_interleaved_wr = 0;  /* interleaved writes           */
int g_interleaved_rd = 0;  /* interleaved reads            */
int g_no_warm_sync = 0;    /* no barrier after warming     */
int g_no_sync = 0;         /* don't do any synchronization */
#endif
int g_extern_suppress;  /* suppress output from external command */
char g_hostname[256];
int g_qdep_x=0, g_wr_x=0, g_wrsz_x=0, g_rdsz_x=0;
int g_wrnd_x=0, g_rrnd_x=0, g_think_x=0, g_str_x=0;
int g_start_warming;
char g_extern_warm_mark[1024];
int g_detail=1;
int g_use_sg=0;
int g_no_fs_sync = 0;
uint64_t g_integrity = 0;
uint64_t g_integrity_offset = 0;

/*
 * Constants
 */

#define FILENAME_LEN 1024
#define TRACE_RECORD_LEN 1024

/*
 * Function declarations
 */

static int sg_cmd_ioctl(int g_fid, char *cmd, int cmd_len, unsigned char *data, 
                        unsigned len, int direction, int timeout);

void usage(char *argv[]) {
    printf("Usage: fitness [options]\n");
    printf("\nOptions to specify the I/O target:\n");
    printf(" --file <string>          "
           "Data file (e.g., /mnt/data)\n");
    printf(" --no_create              "
           "Assume file already exists\n");
    printf(" --allow_sparse           "
           "Do not zero-fill file before doing I/O\n");
    printf(" --no_fs_sync             "
           "Do not sync file system after each iteration\n");
    printf(" --device <string>        "
           "Target device (e.g., /dev/sdb, /dev/sg5)\n");
    printf(" --mount <string>         "
           "Create and mount Ext2 (use with --file and --device)\n");
    printf(" --direct                 "
           "open file/device with O_DIRECT (no page cache)\n");
#if 0
    printf(" --dcap <size>            "
           "Device capacity (MB)\n");
#endif
    printf(" --config <string>        "
           "ips.conf file (dflt %s)\n", DFLT_ISCSI_CONFIG);
    printf(" --tid <int>              "
           "Target id (specify -1 if not iSCSI)\n");
    printf(" --lun <int>              "
           "iSCSI lun (dflt %i)\n", g_lun);     

    printf("\nOptions for workload generation:\n");
    printf(" --samples <int>          "
           "Number of samples (dflt %i)\n", DFLT_SAMPLES);
    printf(" --skip <int>             "
           "Samples to skip over (dflt %i)\n", DFLT_SKIP);
    printf(" --iters <int>            "
           "Iterations per sample (dflt %i)\n", DFLT_ITERS);
    printf(" --seed <int>             "
           "Seed for RNG (dflt %i)\n", DFLT_SEED);
    printf(" --flush <string>         "
           "Command to run after each iteration\n");
    printf(" --flush_args <string>    "
           "Args to pass to flush command\n");
    printf(" --breather <int>         "
           "Time to rest after each iteration (dflt %i)\n", DFLT_BREATHER);
    printf(" --cap <int>              "
           "Working set size in MB (dflt %i)\n", DFLT_CAP);
    printf(" --disp <int>             "
           "Displacement from lba 0 in MB (dflt %i)\n", DFLT_DISP);
    printf(" --wr <int>               "
           "Percent writes (dflt random)\n");
    printf(" --str <1|2>              "
           "Shared (1) or separate (2) rd/wr streams (dflt 1)\n");
    printf(" --qdep <int>             Number of outstanding I/Os (dflt random)\n");
    printf(" --wrsz <int>             Write size (KB) (dflt random)\n");
    printf(" --rdsz <int>             Read size (KB) (dflt random)\n");
    printf(" --wr_stride <int>        Write stride (KB) (dflt 0)\n");
    printf(" --rd_stride <int>        Read stride (KB) (dflt 0)\n");
    printf(" --wrnd <int>             Percent random writes (dflt random)\n");
    printf(" --rrnd <int>             Percent random reads (dflt random)\n");
#if 0
    printf(" --wrsk <dist>            Seek (KB) distribution per write I/O\n");
    printf(" --rdsk <dist>            Seek (KB) distribution per read I/O\n");
#endif
    printf(" --think <int>            Think time (usec) between I/O (dflt 0)\n");
    printf(" --warm <int>             Warm time (secs) (dflt %i)\n", 
           DFLT_WARM);
    printf(" --warm_io <int>          Warm for <int> IOs\n");
    printf(" --warm_mb <int>          Warm for <int> MB of data\n");
    printf(" --test <int>             Test time (secs) (dflt %i)\n", DFLT_TEST);
    printf(" --term <int>             "
           "Stop after <int> MB bytes transferred\n");
    printf(" --outfile <string>       "
           "Output file for results (dflt stdout)\n");
    printf(" --new                    "
           "Start new I/O phase (must specify all args)\n");
    printf(" --continue               "
           "Continue I/O phase with modified arguments\n");
    printf(" --mock                   "
           "Output randomly generated workload parameters (no I/O)\n");
    printf("\nOptions to generate I/O using an external process (e.g., postmark):\n");
    printf(" --extern <string>        "
           "Execute command instead of sampling (e.g., postmark)\n");
    printf(" --extern_args <string>   "
           "Pass these args to the external command\n");
    printf(" --extern_suppress        "
           "Suppress stdout and stderr from external command\n");
    printf(" --extern_shutdown        "
           "Run this command to shutdown external command\n");
    printf(" --extern_mark <string>   "
           "Start warming after <string> encountered in stdout\n");
#ifdef CONFIG_MPI
    printf("\nOptions for MPI:\n");
    printf(" --concurrent             Run tests concurrently\n");
    printf(" --interleaved_wr         Make sequential writes interleaved\n");
    printf(" --interleaved_rd         Make sequential reads interleaved\n");
    printf(" --barrier <freq>         "
           "Barrier sync every <freq> ops (dflt %i)\n", DFLT_BARRIER);
    printf(" --no_warm_sync           "
           "Do not barrier sync after warming phase\n");
    printf(" --no_sync                Do not do any synchronization\n");
    printf(" --prepend                Prepend node id to <outfile>\n");
    printf(" --append                 Append hostname to <outfile>\n");
    printf(" --id <int>               Specify flags for a specific node\n");
    printf(" --scale_disp             Scale --disp by the node's rank\n");
    printf(" --extern_mastered        "
           "Only run <cmd> from node 0 (another MPI job)\n");
#endif
    printf("\nOptions for trace replay:\n");
    printf(" --replay <trace>         "
           "Replay udisk trace AFAP (see udisk -h and utest -h)\n");
#ifdef CONFIG_EXPERIMENTAL
#if 0
    printf(" --timing_accurate        "
           "Attempt a timing accurate trace replay\n");
#endif
#if 0
    printf(" --filter                 "
           "Apply sample filter hardcoded in fitness.c\n");
#endif
#endif
    printf("\nOptions for workload characterization:\n");
    printf(" --i_counters             "
           "Output /proc/scsi/iscsi counters after each iteration\n");
    printf(" --i_counters_loop <int>  "
           "Output /proc/scsi/iscsi counters every <int> secs\n");
    printf(" --t_counters             "
           "Retrieve target counters after each iteration\n");
    printf(" --t_counters_loop <secs> "
           "Retrieve target counters every <secs> secs\n");
    printf("\nMiscellaneous options:\n");
    printf(" --integrity              Write block number into each block (verify on read)\n");
    printf(" --integrity_off <int>    Offset to add to integrity block number\n");
    printf(" --version                Show version\n");
    printf(" --help                   Show usage\n");
}

#define RAND(MIN,MAX,STEP,TYPE) (TYPE == LIN)?                                \
    ((rand()%((MAX-MIN)/STEP+1))*STEP+MIN):                                   \
    (pow(2,(log(MIN)/log(2)+rand()%((int)(log(MAX)/log(2)-log(MIN)/log(2)+1)))))

#define SAMPLE                                                                \
    if (!g_wr_x)       s.wr = RAND(MIN_WR, MAX_WR, STEP_WR, TYPE_WR);         \
    if (!g_wrnd_x)   s.wrnd = RAND(MIN_WRND, MAX_WRND, STEP_WRND, TYPE_WRND); \
    if (!g_rrnd_x)   s.rrnd = RAND(MIN_RRND, MAX_RRND, STEP_RRND, TYPE_RRND); \
    if (!g_str_x)     s.str = RAND(MIN_STR, MAX_STR, STEP_RRND, TYPE_RRND);   \
    if (!g_think_x) s.think = RAND(MIN_THINK, MAX_THINK, STEP_THINK,          \
                                   TYPE_THINK);                               \
    if (!g_qdep_x)   s.qdep = RAND(MIN_QDEP, MAX_QDEP, STEP_QDEP, TYPE_QDEP); \
    if (!g_wrsz_x)   s.wrsz = RAND(MIN_WRSZ, MAX_WRSZ, STEP_WRSZ, TYPE_WRSZ); \
    if (!g_rdsz_x)   s.rdsz = RAND(MIN_RDSZ, MAX_RDSZ, STEP_RDSZ, TYPE_RDSZ);
               
int match(char *string, char *pattern) {
    int     status;
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0)  {
        return(0);
    }
    status = regexec(&re, string, 0, NULL, 0);
    regfree(&re);
    if (status != 0)  {
        return(0);
    }
    return(1);
}

#define PARENT_READ    readpipe[0]
#define CHILD_WRITE    readpipe[1]
#define CHILD_READ     writepipe[0]
#define PARENT_WRITE   writepipe[1]

#define MAX_BUFF 262144

int extern_worker_proc_t(void *arg) {
    ISCSI_WORKER_T *me = (ISCSI_WORKER_T *) arg;
    int rc, status;
    int writepipe[2];
    int readpipe[2];
    char buff[MAX_BUFF*2];
    int completed = 0;

    memset(buff, 0, MAX_BUFF);
    START_ELSE(me, 1, return -1);
    if (pipe(readpipe)<0 || pipe(writepipe)<0) {
        TRACE_ERROR("error creating pipe (errno %i)\n", errno);
        return -1;
    }
    if ((g_extern_pid=fork())==0) {
        close(PARENT_WRITE);
        close(PARENT_READ);
        dup2(CHILD_READ,  0);  close(CHILD_READ);
        dup2(CHILD_WRITE, 1);  close(CHILD_WRITE);
        if (strlen(s.external_args)) {
            rc=execlp(s.external, s.external, s.external_args, NULL);
        } else {
            rc=execlp(s.external, s.external, NULL);
        }
        fprintf(stderr, "execlp failed (rc %i errno %i)\n", rc, errno);
        exit(1);
    } else {
        close(CHILD_READ);
        close(CHILD_WRITE);
        close(PARENT_WRITE); /* assuming command takes no input */
        while ((rc=read(PARENT_READ, buff+completed, ISCSI_COUNTERS_SIZE))>0) {
            buff[completed+rc] = '\0';
            if (!g_extern_suppress) {
                fprintf(stderr, "%s", buff+completed);
                fflush(stderr);
            }
            completed += rc;
            if (strstr(buff, g_extern_warm_mark)!=NULL) {
                g_start_warming = 1;
            }
            if (completed>MAX_BUFF) {
                TRACE_ERROR("buffer overflow\n");
                exit(1);
            }
        }
        close(PARENT_READ);
        if (rc == -1) {
            TRACE_ERROR("error reading from pipe (rc %i errno %i)\n", rc, 
                        errno);
            exit(1);
        }
    }
    waitpid(g_extern_pid, &status, 0);
    g_end_of_program = 1;
    EXIT(me, 0); 
}

int worker_proc_t(void *arg) {
    ISCSI_WORKER_T *me = (ISCSI_WORKER_T *) arg;
    struct timeval start, stop, arrival;
    int writing = 0;
    uint32_t len = 0;
    double latency;
    unsigned char *buffer, *buffer_ptr;
    int rc;
    int size;
#ifdef CONFIG_MPI
    int count = 1;
    int i;
#endif
    uint64_t offset = 0;
    int barrier_count = 0;
    char *record;
    size_t record_len;
    int fh, ver;
    int replaying=0;
    struct timeval current_time;
    uint64_t elapsed=0, elapsed_trace=0;
    double trash;
    //int i;
    static int count=0; 

    /* page-aligned I/O buffer (for O_DIRECT) */
    buffer = buffer_ptr = malloc((MAX_REQUEST_SIZE<<10)+4096);

#if 0
    /* fill with some data */
    printf("filling data buffer...\n");
    for (i=0; i<((MAX_REQUEST_SIZE<<10)+4096); i++) {
	buffer[i] = 'X';
    }
#endif

    while ((unsigned long)buffer%4096) buffer++;
    record = malloc(TRACE_RECORD_LEN);

    if (me->id == 0) {
        if (strlen(s.tracefile)) {
            if ((g_fptr=fopen(s.tracefile, "r"))==NULL) {
                fprintf(stderr, "error opening tracefile \"%s\"\n", 
                        s.tracefile);
                return -1;
            }
        } else {
            g_fptr = NULL;
        }
    }
    if (g_fptr) {
        replaying = 1;
    }

    START_ELSE(me, 1, return -1);

    while (!WORKER_STOPPING(me)||barrier_count%100) {

        /* grab lock for updating counters */
        ISCSI_LOCK_ELSE(&s.c.lock, EXIT(me, -1));

        if (replaying) {
            double time;
            char op;

        read_record: 
            if (g_end_of_file||feof(g_fptr)) {
                g_end_of_file = 1;
                fprintf(stderr, "EOF at line %i\n", g_traceline);
                ISCSI_UNLOCK_ELSE(&s.c.lock, EXIT(me, -1));
                goto done;
            }
            g_traceline++;
            record_len = TRACE_RECORD_LEN;
            if (getline(&record, &record_len, g_fptr)==-1) {
                if (feof(g_fptr)) {
                    g_end_of_file = 1;
                    fprintf(stderr, "EOF at line %i\n", g_traceline);
                    ISCSI_UNLOCK_ELSE(&s.c.lock, EXIT(me, -1));
                    goto done;
                } else {
                    exit(1);
                }
                ISCSI_UNLOCK_ELSE(&s.c.lock, EXIT(me, -1));
                exit(1);
            }

            /*
             * trace format: <secs>.<usecs> <r|w> <off in bytes> <len in bytes>
             * cello format: <secs>.<usecs> <r|w> <off in blocks> <len in bytes>
             */
 
            if (sscanf(record, "%lf %c %"PRIu64" %u\n", &trash, &op, &offset, 
                       &len)!=4) {               
                fprintf(stderr, "error parsing line %i:%s\n", 
                        g_traceline, record);
                ISCSI_UNLOCK_ELSE(&s.c.lock, EXIT(me, -1));
                exit(1);
            }
            if (offset == (uint64_t) (-1)) {
                goto read_record;
            }

            /* offset *= 512 */
            /* len *= 512; */

            //PRINT("worker %i read I/O (op %c offset %"PRIu64" len %u, count %i)\n", 
            //    me->id, op, offset/512, len/512, count++);

            /* check offset and length */
            if ((offset+len)>g_cap) {
#if 0
                fprintf(stderr, 
                        "line %i: offset (%"PRIu64") + len (%u) > cap (%"
                        PRIu64") -- setting offset to %"PRIu64"\n", 
                        g_traceline, offset, len, g_cap, offset%g_cap);
#endif
                offset = offset%g_cap;
            }
            if (len>MAX_REQUEST_SIZE<<10) {
                fprintf(stderr, 
                        "ignoring line %i: max request size is %uKB:\n%s", 
                        g_traceline, MAX_REQUEST_SIZE, record);
                goto read_record;
            }
            switch (op) {
            case 'w':
                writing = 1;
                break;
            case 'r':
                writing = 0;
                break;
            default:
                fprintf(stderr, "ignoring line %i: unknown operation:\n%s",
                        g_traceline, record);
                goto read_record;
            }
        }

        /* even when replaying a trace, we can override the operation type,
         * offset and length of each I/O on the command-line */
        if (!replaying || g_wr_x) {
            if ((rand()%100+1)<=s.wr) {
                writing = 1;
            } else {
                writing = 0;
            }
        }

        if (writing) {
            if (!replaying||g_rdsz_x) offset = s.c.wr_offset; 
            if (!replaying||g_wrsz_x) len = s.wrsz*1024; 
            if (!replaying||g_wrnd_x) offset = s.c.wr_offset;
            writing = 1;
            if ((rand()%100+1)<=s.wrnd) {  /* random write     */
                s.c.wr_offset = ((rand()%((s.cap<<10)/s.wrsz))<<10)*
                    s.wrsz+(s.disp<<20);
            } else {                       /* sequential write */
#ifdef CONFIG_MPI
                if (s.wr_stride) {
                    s.c.wr_offset += (s.wrsz<<10) + (s.wr_stride<<10);
                } else {
                    s.c.wr_offset += (s.wrsz<<10)*(g_interleaved_wr?
                                                   g_num_procs:1);
                }
#else
                s.c.wr_offset += (s.wrsz<<10) + (s.wr_stride<<10);
#endif
                if ((s.c.wr_offset+(s.wrsz<<10))>(s.disp+s.cap)<<20) 
                    s.c.wr_offset = s.disp<<20;
            }
        } else { /* reading */
            if (!replaying||g_rrnd_x) offset = s.c.rd_offset; 
            if (!replaying||g_rdsz_x) len = s.rdsz*1024;
            if (!replaying||g_rrnd_x) offset = s.c.rd_offset;
            writing = 0;
            if ((rand()%100+1)<=s.rrnd) {  /* random read     */
                s.c.rd_offset = ((rand()%((s.cap<<10)/s.rdsz))<<10)*
                    s.rdsz+(s.disp<<20);
            } else {                       /* sequential read */
#ifdef CONFIG_MPI
                if (s.rd_stride) {
                    s.c.rd_offset += (s.rdsz<<10) + (s.rd_stride<<10);
                } else {
                    s.c.rd_offset += (s.rdsz<<10)*(g_interleaved_rd?
                                                   g_num_procs:1);
                }
#else
                s.c.rd_offset += (s.rdsz<<10) + (s.rd_stride<<10);
#endif
                if ((s.c.rd_offset+(s.rdsz<<10))>(s.disp+s.cap)<<20) 
                    s.c.rd_offset = s.disp<<20;
            }
        }       

        /* unless streams is one, we maintain two different 
           offsets (reading and writing) */
        if (s.str == 1) {
            if (writing) {
                s.c.rd_offset = s.c.wr_offset;
            } else {
                s.c.wr_offset = s.c.rd_offset;
            }
        }
    
        /* make sure we're not reading past device or EOF */
        if (g_tid != -1 && (offset+len)>(g_cap)) {
            fprintf(stderr, 
                    "accessing past device (device cap is %"PRIu64", "
                    "requesting %u at offset %"PRIu64")!\n", g_cap, len, 
                    offset);
            exit(1);
        }
        if ((s.file[0]!='\0')&&((offset+len)>(s.cap<<20))) {
            fprintf(stderr, 
                    "warning: passing EOF (offset %"PRIu64" + len %u > "
                    "cap %"PRIu64")\n", offset, len, s.cap<<20);
            fprintf(stderr, 
                    "warning: shortening request from %u to %"PRIu64"\n", 
                    len, (s.cap<<20)-offset);
            len = (s.cap<<20)-offset;
        }

        s.c.bytes += len;

#if 0
        /* determine how long to sleep before issuing I/O */
        if (replaying && s.timing_accurate) { 
            gettimeofday(&current_time, NULL);
            if (!g_flag) {
                memcpy(&g_start_time, &current_time, sizeof(struct timeval));
                memcpy(&g_start_trace, &arrival, sizeof(struct timeval));
                g_flag = 1;
            }
            elapsed = (current_time.tv_sec*1e6+current_time.tv_usec)-
                (g_start_time.tv_sec*1e6+g_start_time.tv_usec);
            elapsed_trace = (arrival.tv_sec*1e6+arrival.tv_usec)-
                (g_start_trace.tv_sec*1e6+g_start_trace.tv_usec);
        }
#endif
        
        /* release lock */
        ISCSI_UNLOCK_ELSE(&s.c.lock, EXIT(me, -1));

#if 0
        if (replaying && s.timing_accurate) { 
            if (elapsed_trace > elapsed) {
                usleep(elapsed_trace-elapsed);
            }
        }
#endif

        if (g_integrity) {
            /* add integrity check to outgoing sectors */
            if (writing) {
		uint64_t sector;
                
		for (sector=0; sector<len/512; sector++) {
                    *((uint64_t *)(buffer+sector*512)) = 
			offset/512+sector+g_integrity_offset;
		}
            } else {
		memset(buffer, 0, len);
            }
        }

        //PRINT("worker %i issuing I/O (writing %i offset %"PRIu64" len %u)\n", 
        //   me->id, writing, offset/512, len/512);
            
        /* issue and time I/O */
        gettimeofday(&start, NULL);
        if (g_fid[me->id] == -1) {
            if (iscsi_issue_io(g_tid, g_lun, writing, offset, len, buffer, 
                               g_blocklen)!=0) {
                fprintf(stderr, "failed to %s offset %"PRIu64" len %u\n", 
                        writing?"write":"read", offset, len);
                exit(1);
            }
        } else {
            if (!g_use_sg) {
                if (lseek64(g_fid[me->id], offset, SEEK_SET)!=offset) {
                    fprintf(stderr, "failed to seek to %"PRIu64"\n", offset);
                }
                if (writing) {
                    if ((rc=write(g_fid[me->id], buffer, len))!=len) {
                        fprintf(stderr, 
                                "write failed (buff %p, rc %i, offset %"
                                PRIu64", ""len %i, errno %i)\n", 
                                buffer, rc, offset, len, 
                                errno);
                        if (replaying) fprintf(stderr, "replay line %i: %s\n", 
                                               g_traceline, record);
                        exit(1);
                    }
                } else {
                    if ((rc = read(g_fid[me->id], buffer, len))!=len) {
                        fprintf(stderr, 
                                "read failed (rc %i, offset %"PRIu64", len %i, "
                                "errno %i)\n", rc, offset, len, errno);
                        exit(1);
                    }
                }
            } else { /* use SCSI generic interface */
                char cdb[10];

                memset(cdb, 0, 10);
                if (writing) cdb[0] = 0x2a;
                else cdb[0] = 0x28;                
                if (offset%512) 
                    TRACE_ERROR("WARN: offset is not a multiple of 512\n");
                *((unsigned *)(cdb+2)) = htonl(offset/512);
                *((unsigned short *)(cdb+7)) = htons(len/512);

		cdb[6] = 0;

                if (sg_cmd_ioctl(g_fid[me->id], cdb, 10, buffer, len, 
                                 writing?SG_DXFER_TO_DEV:SG_DXFER_FROM_DEV, 
                                 10000)!=0) {
                    TRACE_ERROR("SCSI generic I/O failed\n");
                    exit(1);
                }
            }
        }
        gettimeofday(&stop, NULL);
        latency = (stop.tv_sec*1000000+stop.tv_usec)-
            (start.tv_sec*1000000+start.tv_usec);

        ISCSI_LOCK_ELSE(&s.c.lock, EXIT(me, -1));
        s.c.lat += latency;
        s.c.ops++;
        s.c.bytes_actual += len;
        ISCSI_UNLOCK_ELSE(&s.c.lock, EXIT(me, -1));

        if (g_integrity) {
            /* check integrity on reads */
            if (!writing) {
		uint64_t sector;
                
		for (sector=0; sector<len/512; sector++) {
                    if (*((uint64_t *)(buffer+sector*512)) != 
                        offset/512+sector+g_integrity_offset) {
                        printf("integrity check failed: read %"PRIu64
                               " from sector %"PRIu64" (expected %"PRIu64")\n", \
                               *((uint64_t *)(buffer+sector*512)+g_integrity_offset), 
                               offset/512+sector,
                               offset/512+sector+g_integrity_offset);
                        exit(1);
                    }
		}
            }
        }

        /* sleep some */
        if (s.think) ISCSI_USLEEP(rand()%s.think);

#ifdef CONFIG_MPI
        if (!g_no_sync && g_concurrent && s.barrier!=-1) {
            if ((count%s.barrier)==0) {
                MPI_Barrier(MPI_COMM_WORLD); /* global synchronization test */
                barrier_count++;
            }
            count++;
        }
#endif
    } 

 done: 
    free(buffer_ptr); 
    free(record); 
    if ((me->id==0)&&replaying) fclose(g_fptr);
    EXIT(me, 0);
}

void grab_counters(uint64_t tid, uint64_t lun) {
    char buff[ISCSI_COUNTERS_SIZE];
    int i_save = strlen(s.i_counters_save);
    FILE *i_stream = NULL;
    FILE *t_stream = NULL;
    int t_save = strlen(s.t_counters_save);

    if (i_save) {
        if ((i_stream=fopen(s.i_counters_save, "w"))==NULL) {
            TRACE_ERROR("error opening counters file \"%s\"\n", 
                        s.i_counters_save);
            exit(1);
        }
    }

    if (t_save) {
        if ((t_stream=fopen(s.t_counters_save, "w"))==NULL) {
            TRACE_ERROR("error opening counters file \"%s\"\n", 
                        s.t_counters_save);
            exit(1);
        }
    }

    while (1) {
        if (s.i_counters_loop) {
            I_COUNTERS_GET(g_tid, g_lun, buff);
            if (i_save) {
                fprintf(i_stream, "%s", buff);
                fflush(i_stream);
            }
            fprintf(stdout, "%s", buff); 
            fflush(stdout);
            sleep(s.i_counters_loop);
        }
        if (s.t_counters_loop) {
            T_COUNTERS_GET(g_tid, g_lun, buff);
            if (t_save) {
                fprintf(t_stream, "%s", buff);
                fflush(t_stream);
            }
            fprintf(stdout, "%s", buff); 
            fflush(stdout);
            sleep(s.t_counters_loop);
        }
    }
}

int main(int argc, char *argv[]) {
    char c;
    int digit_optind = 0;
    int sample, iter, k;
    int warm_x, test_x;
    struct timeval t_start, t_stop;
    int tids;
    char iscsi_config_file[256];
    int iscsi_config_file_x = 0;
    int skipped;
    struct sigaction act;
    uint32_t max_lba;
    FILE *stream = stdout;
    char outfile[FILENAME_LEN] = "";
    int next_to_output = 0;
    int next_to_go = 0;
    int repeat;
    uint64_t last_wr_offset = 0;
    uint64_t last_rd_offset = 0;
    int phase = 0;
    int fresh = 1;
    int samples = 0;
    int skip = 0;
    int i;
    int current_id = -1;
    int filter = 0;
    int filtered = 0;
    char cmd[FILENAME_LEN];
    char counters_start[ISCSI_COUNTERS_SIZE], 
        counters_stop[ISCSI_COUNTERS_SIZE];
    char counters_diff[ISCSI_COUNTERS_SIZE], label[16];

    gethostname(g_hostname,256);

    /* signal handlers */
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL); 

#ifdef CONFIG_MPI
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&g_myid);
    MPI_Comm_size(MPI_COMM_WORLD,&g_num_procs);
#endif

 repeat:

    /* initialize args here */
    strcpy(iscsi_config_file, DFLT_ISCSI_CONFIG);
    next_to_output = 0;
    next_to_go = 0;
    repeat = 0;
    if (!phase||fresh) {
        digit_optind = 0;
        g_qdep_x=0; g_wr_x=0; g_wrsz_x=0; g_rdsz_x=0; 
        g_wrnd_x=0; g_rrnd_x=0; g_think_x=0; g_str_x=0;
        iscsi_config_file_x = 0;
        g_extern_pid = -1;
        g_tid = -1;
        g_lun = 0;
        g_reset = 0;
        g_verbose = 1;
        g_concurrent = 0;
        g_prepend = 0;  
        g_append = 0;  
        g_cap = 0;
        repeat = 0;

        /* set defaults here */
        memset(&s, 0, sizeof(SAMPLE_T));
        s.cap = DFLT_CAP;
        s.seed = DFLT_SEED;
        s.samples = DFLT_SAMPLES;
        s.skip = DFLT_SKIP;
        s.iters = DFLT_ITERS;
        s.warm = DFLT_WARM;
        s.warm_io = -1;
        s.warm_mb = -1;
        s.test = DFLT_TEST;
        s.disp = DFLT_DISP;
        s.i_counters = DFLT_I_COUNTERS;
        s.t_counters = DFLT_T_COUNTERS;
        s.allow_sparse = DFLT_ALLOW_SPARSE;
        s.no_create = DFLT_NO_CREATE;
        s.barrier = DFLT_BARRIER;

    } else {
        /* setting these keeps the randomly selected (or specified) values
         * from the uprevious phase */
        g_qdep_x=1; g_wr_x=1; g_wrsz_x=1; g_rdsz_x=1; 
        g_wrnd_x=1; g_rrnd_x=1; g_think_x=1; g_str_x=1;
        memset(&s.c, 0, sizeof(SAMPLE_COUNTERS_T));
    }

    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        
        static struct option long_options[] = {
            {             "qdep", 1, 0, '0'},
            {               "wr", 1, 0, '1'},
            {             "wrsz", 1, 0, '2'},
            {             "rdsz", 1, 0, '3'},
            {             "wrnd", 1, 0, '4'},
            {             "rrnd", 1, 0, '5'},
            {            "think", 1, 0, '6'},
            {              "str", 1, 0, '7'},
            {              "tid", 1, 0, '8'},
            {              "lun", 1, 0, '9'},
            {           "config", 1, 0, 'a'},
            {          "outfile", 1, 0, 'b'},
            {              "cap", 1, 0, 'c'},
            {           "device", 1, 0, 'd'},
            {             "term", 1, 0, 'e'},
            {             "file", 1, 0, 'f'},
            {      "extern_mark", 1, 0, 'g'},
            {             "help", 0, 0, 'h'},
            {            "mount", 1, 0, 'j'},
            {             "disp", 1, 0, 'k'},
            {            "flush", 1, 0, 'l'},
            {       "flush_args", 1, 0, 'n'},
            {         "breather", 1, 0, 'o'},
            {           "extern", 1, 0, 'p'},
            {      "extern_args", 1, 0, 'q'},
#if 0
            {             "dcap", 1, 0, 'r'},
#endif

            {         "continue", 0, 0, 'z'},
            {              "new", 0, 0, 'A'},
            {               "id", 1, 0, 'B'},
            {           "direct", 0, 0, 'C'},
            {        "integrity", 0, 0, 'D'},
            {        "no_create", 0, 0, 'E'},
            {          "barrier", 1, 0, 'F'},
            {             "wrsk", 1, 0, 'G'},
            {             "rdsk", 1, 0, 'H'},
            {           "replay", 1, 0, 'I'},
            {          "warm_io", 1, 0, 'J'},
            {          "warm_mb", 1, 0, 'K'},
            {             "warm", 1, 0, 'L'},
            {             "test", 1, 0, 'M'},
            {             "seed", 1, 0, 'N'},
            {          "samples", 1, 0, 'O'},
            {             "skip", 1, 0, 'P'},
            {            "iters", 1, 0, 'Q'},
            {  "extern_shutdown", 1, 0, 'R'},
            {        "wr_stride", 1, 0, 'S'},
            {        "rd_stride", 1, 0, 'T'},
            {       "no_fs_sync", 0, 0, 'U'},
            {  "i_counters_loop", 1, 0, 'V'},
            {  "i_counters_save", 1, 0, 'W'},
            {  "t_counters_loop", 1, 0, 'X'},
            {  "t_counters_save", 1, 0, 'Y'},
            { "integrity_offset", 1, 0, 'Z'},
#if 0
            {           "filter", 0, &s.filter, 1},
#endif
            {       "i_counters", 0, &s.i_counters, 1},
            {       "t_counters", 0, &s.t_counters, 1},
            {             "mock", 0, &s.mock, 1},
            {     "allow_sparse", 0, &s.allow_sparse, 1},
#ifdef CONFIG_MPI
            {       "concurrent", 0, &g_concurrent, 1},
            {           "append", 0, &g_append, 1},
            {          "prepend", 0, &g_prepend, 1},
            {     "no_warm_sync", 0, &g_no_warm_sync, 1},
            {          "no_sync", 0, &g_no_sync, 1},
            {   "interleaved_wr", 0, &g_interleaved_wr, 1},
            {   "interleaved_rd", 0, &g_interleaved_rd, 1},
            {       "scale_disp", 0, &g_scale_disp, 1},
            {  "extern_mastered", 0, &g_extern_mastered, 1},
#endif
            {  "extern_suppress", 0, &g_extern_suppress, 1},
#if 0
            {  "timing_accurate", 0, &s.timing_accurate, 1},
#endif
            {          "version", 0, 0, 'v'},
            {0, 0, 0, 0}
        };

    next: c = getopt_long (argc, argv, "h", long_options, &option_index);

        if (c == 0) goto next;
        if (c == -1) break;

#define CHECK(WHICH, MIN, MAX)                                          \
        if (atoi(optarg)>MAX) {fprintf(stderr, "max %s is %i\n", WHICH, MAX); \
            exit(1);}                                                   \
        else if (atoi(optarg)<MIN) {fprintf(stderr, "min %s is %i\n",   \
                                            WHICH, MIN); exit(1);}

        /* arguments that all nodes should see */
        switch(c) {
        case 'B': /* --id */
            current_id = atoi(optarg);
            goto next;
        case 'z':  /* --continue */
            current_id = -1;
            repeat = 1; fresh = 0;
            goto done;
        case 'A': /* --new */
            current_id = -1;
            repeat = 1; fresh = 1;
            goto done;
        }

        /* if we're using paritioned flags, then only the <current_id>
         * process needs to pay attention to the following flags */
        if ((current_id!=-1)&&(g_myid!=current_id)) goto next;

        switch (c) {
        case '0':
            CHECK("qdep", MIN_QDEP, MAX_QDEP);
            s.qdep = atoi(optarg);  g_qdep_x = 1; break;
        case '1':
            CHECK("wr", MIN_WR, MAX_WR);
            s.wr = atoi(optarg);    g_wr_x = 1; break;
        case '2':
            s.wrsz = atoi(optarg);  g_wrsz_x = 1; break;
        case '3':
            s.rdsz = atoi(optarg);  g_rdsz_x = 1; break;
        case '4':
            CHECK("wrnd", MIN_WRND, MAX_WRND);
            s.wrnd = atoi(optarg);  g_wrnd_x = 1; break;
        case '5':
            CHECK("rrnd", MIN_RRND, MAX_RRND);
            s.rrnd = atoi(optarg);  g_rrnd_x = 1; break;
        case '6':
            s.think = atoi(optarg); g_think_x = 1; break;
        case '7':
            s.str = atoi(optarg); g_str_x = 1; break;
        case '8':
            g_tid = atoi(optarg);
            break;
        case '9':
            g_lun = atoi(optarg); break;
        case 'a':
            strcpy(iscsi_config_file, optarg);
            iscsi_config_file_x = 1;
            break;
        case 'b':
            strcpy(outfile, optarg);
            break;
        case 'c':
            s.cap = atoi(optarg);
            break;
        case 'd':
            strcpy(s.device, optarg);
            s.mode |= MODE_DEVICE;
            break;
        case 'e':
            s.term = atoi(optarg);
            break;
        case 'f':
            strcpy(s.file, optarg);
            s.mode |= MODE_FILE;
            break;
        case 'g':
            strcpy(g_extern_warm_mark, optarg);
            break;
        case 'h':
            usage(argv);
            return 0;
        case 'j':
            strcpy(s.mkfs, optarg);
            s.mode |= MODE_MOUNT;
            break;
        case 'k':
            s.disp = atoi(optarg);
            break;
        case 'l':
            strcpy(s.flush, optarg);
            break;
        case 'n':
            strcpy(s.flush_args, optarg);
            break;
        case 'o':
            s.breather = atoi(optarg);
            break;
        case 'p':
            strcpy(s.external, optarg);
            break;
        case 'q':
            strcpy(s.external_args, optarg);
            break;
        case 'r':
            g_cap = atoi(optarg);
            break;
        case 'V':
            s.i_counters_loop = atoi(optarg);
            break;
        case 'W':
            strcpy(s.i_counters_save, optarg);
            break;
        case 'X':
            s.t_counters_loop = atoi(optarg);
            break;
        case 'Y':
            strcpy(s.t_counters_save, optarg);
            break;
        case 'C':
            s.direct = 1;
            break;
        case 'E':
            s.no_create = 1;
            break;
        case 'F':
            if (atoi(optarg)<=0) {
                fprintf(stderr, "--barrier argument must be > 0\n");
                return -1;
            }
            s.barrier = atoi(optarg);
            break;
#if 0
        case 'G':
            if (sscanf(optarg, "%i,%i,%i,%i,%i,%i,%i,%i,%i,%i", 
                       &s.wrsk_dist[0], &s.wrsk_dist[1], &s.wrsk_dist[2], 
                       &s.wrsk_dist[3], &s.wrsk_dist[4],
                       &s.wrsk_dist[5], &s.wrsk_dist[6], &s.wrsk_dist[7], 
                       &s.wrsk_dist[8], &s.wrsk_dist[9])!=10) {
                fprintf(stderr, "invalid wrsk distribution: \"%s\"\n", optarg);
                fprintf(stderr, 
                        "must provide 10 comma-delimited percentiles\n");
                return -1;
            }
            break;
        case 'H':
            if (sscanf(optarg, "%i,%i,%i,%i,%i,%i,%i,%i,%i,%i", 
                       &s.rdsk_dist[0], &s.rdsk_dist[1], &s.rdsk_dist[2], 
                       &s.rdsk_dist[3], &s.rdsk_dist[4],
                       &s.rdsk_dist[5], &s.rdsk_dist[6], &s.rdsk_dist[7], 
                       &s.rdsk_dist[8], &s.rdsk_dist[9])!=10) {
                fprintf(stderr, "invalid rdsk distribution: \"%s\"\n", optarg);
                fprintf(stderr, 
                        "must provide 10 comma-delimited percentiles\n");
                return -1;
            }
            break;
#endif
        case 'I':
            if (strchr(optarg, '%')) {
                sprintf(s.tracefile, optarg, g_hostname);
            } else {
                strcpy(s.tracefile, optarg);
            }
            break;
        case 'J':
            s.warm_io = atoi(optarg);
            break;
        case 'K':
            s.warm_mb = atoi(optarg);
            break;
        case 'L':
            s.warm = atoi(optarg);
            break;
        case 'M':
            s.test = atoi(optarg);
            break;
        case 'N':
            s.seed = atoi(optarg);
            break;
        case 'O':
            s.samples = atoi(optarg);
            break;
        case 'P':
            s.skip = atoi(optarg);
            break;
        case 'Q':
            s.iters = atoi(optarg);
            break;
        case 'R':
            strcpy(s.external_shutdown, optarg);
            break;
        case 'S':
            s.wr_stride = atoi(optarg);
            break;
        case 'T':
            s.rd_stride = atoi(optarg);
            break;
        case 'U':
            g_no_fs_sync = 1;
            break;
        case 'v':
            printf("%s\n", PACKAGE_STRING);
            return 0;
        case 'D':
            g_integrity = 1;
            break;
        case 'Z':
            g_integrity_offset = atoi(optarg);
            break;
        default:
            fprintf(stderr, "--help for usage\n");
            return -1;
        }
    }
    if (optind < argc) {
        printf ("unknown options (--help for usage): ");
        while (optind < argc) printf ("%s ", argv[optind++]);
        printf ("\n");
        return 1;
    }

 done:

    //if (s.mock) goto top;
 
    /* check args and set mode */ 

    if ((s.wrsz > MAX_REQUEST_SIZE)||(s.rdsz > MAX_REQUEST_SIZE)) {
        fprintf(stderr, "request size cannot exceed %i KB\n", MAX_REQUEST_SIZE);
        return 1;
    }
    if (s.mode&MODE_MOUNT) {
        if (!((s.mode&MODE_DEVICE)&&((s.mode&MODE_FILE)||
                                     (strlen(s.external))))) {
            fprintf(stderr, 
                    "--mount requires --device, and --file or --extern\n");
            return 1;
        }
    } else if ((s.mode&MODE_FILE)&&(s.mode&MODE_DEVICE)) {
        fprintf(stderr, 
                "--file and --device are mutually exclusive without --mount\n");
        return 1;
    } else if ((s.i_counters_loop||s.i_counters)&&(g_tid==-1)) {
        fprintf(stderr, "--i_counters also require --tid\n");
        return 1;
    } 

    if (!s.mode) {
        s.mode |= MODE_ISCSI;
        if (s.direct) {
            fprintf(stderr, "Error: --direct only valid  with --device or --file\n");
            return 1;
        }
    }

    /* when using --term and --extern, i_counters must be specified */
    if (strlen(s.external)) {
        if ((s.term||s.warm_mb>0||s.warm_io>0)&&!s.i_counters) {
            fprintf(stderr, 
                    "  --extern requires --i_counters when using --term, "
                    "--warm_mb or --warm_io\n");
            return 1;
        }
    }
    
    /* make sure some mode was specified */
    if ((s.mode&MODE_ISCSI)&&(g_tid==-1)) {
        fprintf(stderr, "Specify one of four I/O modes:\n\n");
        fprintf(stderr, "  1) To a file (use --file)\n");
        fprintf(stderr, "  2) To a device in /dev (use --device)\n");
        fprintf(stderr, "  3) To a file mounted on a specific device "
                "(use --file, --device, and --mount)\n");
        fprintf(stderr, "  4) To an iSCSI device via user-level "
                "(use --config, --tid, and --lun)\n");
        fprintf(stderr, "\nRun fitness -h for usage.\n");
        return 1;
    }
        
#ifdef CONFIG_MPI
    /* if we're warming for a certain number of ops or mbytes,
     * then disable after-warm synchronization for mpi jobs */
    if (s.warm_mb!=-1 || s.warm_io!=-1) {
        g_no_warm_sync = 1;
    }

    if (s.qdep>1 && s.barrier!=-1) {
        fprintf(stderr, "--barrier %i requires --qdep 1\n", s.barrier);
        fprintf(stderr, "--help for usage\n");
        return -1; 
    }
    if (!g_no_warm_sync && s.barrier!=-1) {
        fprintf(stderr, "--barrier (%i) requires --no_warm_sync\n", s.barrier);
        fprintf(stderr, "--help for usage\n");
        return -1; 
    }
    if (phase == 0) {
        if (strlen(outfile)&&g_prepend) {
            char tmp[FILENAME_LEN];
            strcpy(tmp, outfile);
            sprintf(outfile, "%i.%s", g_myid, tmp);
        }
        if (strlen(outfile)&&g_append) {
            sprintf(outfile+strlen(outfile), ".%s", g_hostname);
        }
    }

    if (g_scale_disp) {
        s.disp *= g_myid;
    }

    /* Unless --concurrent was specified, we wait our turn */
    if (g_concurrent) {
        g_verbose = (g_myid==0)?1:0;
    } else {
        WAIT_TURN;
    }

#endif

    /* make sure we can open our output file */
    if (strlen(outfile)) {
        if (s.skip) {
            if ((stream=fopen(outfile, "a+"))==NULL) {
                TRACE_ERROR("error opening output file \"%s\"\n", outfile);
                return -1;
            }
        } else {
            if ((stream=fopen(outfile, "w+"))==NULL) {
                TRACE_ERROR("error opening output file \"%s\"\n", outfile);
                return -1;
            }
        }
    }

    /* initialize the iSCSI libary if necessary */
    if ((g_tid != -1) && !phase) {
        //fprintf(stderr, "config file is %s\n", iscsi_config_file);
        if ((tids=initiator_init(iscsi_config_file, 0, AF_INET))==-1) {
            fprintf(stderr, "initiator_init() failed\n");
            return 1;
        }
        if (g_tid>tids-1) {
            fprintf(stderr, "invalid tid %i (must be < %i)\n", g_tid, tids);
            return 1;
        }
    }

    /* If i_counters_loop was specified, we loop forever and output the
     * counters to stdout. This is used by iscsi-monitor. */
    if (s.i_counters_loop||s.t_counters_loop) grab_counters(g_tid, g_lun);

    /* check that the disk is big enough if a tid was specified */
    if (g_tid != -1) {
        if (read_cap(g_tid, g_lun, &max_lba, &g_blocklen)!=0) {
            fprintf(stderr, "read_cap() failed\n");
            return 1;
        } 
        g_cap = (uint64_t)(max_lba+1)*g_blocklen;
        if ((s.disp<<20)+(s.cap<<20)>g_cap) {
            fprintf(stderr, "working set (%"PRIu64" MB) + displacement "
                    "(%"PRIu64" MB) must be <= disk capacity (%"PRIu64" MB)\n", 
                    s.cap, s.disp, g_cap>>20);
            return 1;
        }
    } else {
#if 0
        if (!g_cap) {
            fprintf(stderr, 
                    "You must set the device capacity (in MB) using --dcap\n");
            return 1;
        }
#endif
    }

    /* check if using scsi generic mode */
    if (strstr(s.device, "/dev/sg")) {
        g_use_sg=1;
        if (s.direct) {
            fprintf(stderr, "Error: --direct cannot be used with SG interface\n");
            return 1;
        }
        printf("SCSI generic mode\n");
    } else {
        if ((s.mode&MODE_DEVICE) && s.direct) {
            fprintf(stderr, "\nWarning: "
                    "--direct may preclude NCQ when used with --device.\n"
                    "         Try using SG interace without --direct option."
                    " (e.g., --device /dev/sg<foo>).\n\n");
        }
    }

    /* generate workload samples */
    skip = s.skip;
    samples = s.samples;
 top: skipped = skip;

    if (fresh) {
        /* srand(s.seed*(g_myid+1)); */
        srand(s.seed);
    } else {
        FPRINTF(stderr, "WARN: not reseeding\n");
    }

    for (sample=0; sample<samples+skipped+filtered; sample++) {

        if (!samples) break;
        
        SAMPLE;
       
#if 0 
        /* Apply any filters here. */
        if (s.filter) {
            if (!skip) {
                if  (s.wr != 75) {
                    filter = 1;
                    filtered++;
                } else {
                    filter = 0;
                }
            }
        }
#endif
        
        if (!skip && !filter) {
            int last_xfer, xfer;
            
            for (iter=0; iter<s.iters; iter++) {
                next_to_output = 0;
                g_end_of_program = 0;
                g_end_of_file = 0;
                g_start_warming = strlen(g_extern_warm_mark)?0:1;
                if (s.mock) {
                    if (!sample&&!iter) {
                        FPRINTF(stream, 
                                "%3s %1s %5s %5s %3s %5s %5s %4s %5s %5s\n", 
                                "s", "i", "cap", "qdep", "wr", "wrnd", 
                                "rrnd", "str", "wrsz", "rdsz");
                    }
                    FPRINTF(stream, 
                            "%3i %1i %5"PRIu64" %5i %3i %5i %5i %4i %5i %5i\n", 
                            sample, iter, s.cap, s.qdep, s.wr, s.wrnd, 
                            s.rrnd, s.str, s.wrsz, s.rdsz);     
                    continue;
                }
                
                /* initialize */
                memset(&(s.c), 0, sizeof(SAMPLE_COUNTERS_T));
                
                /* set beginning write and read offsets */
                if (!strlen(s.external)) {
                    if (!fresh) {  /* start where previous phase left off */
                        s.c.rd_offset = last_rd_offset;
                        s.c.wr_offset = last_wr_offset;
                    } else {       /* start fresh */
#ifdef CONFIG_MPI
                        s.c.rd_offset = (s.disp<<20) + 
                            g_interleaved_rd?((s.rdsz<<10)*g_myid):0;
                        s.c.wr_offset = (s.disp<<20) + 
                            g_interleaved_wr?((s.wrsz<<10)*g_myid):0;
#else
                        s.c.rd_offset = (s.disp<<20);
                        s.c.wr_offset = (s.disp<<20);
#endif
                    }
                    ISCSI_LOCK_INIT_ELSE(&s.c.lock, return 1);
                }
                last_xfer = 0;
                
                if (phase == 0) {
                    /* mount file system */
                    if (strlen(s.mkfs)) {
                        /* create file system */
                        if (!s.no_create) {
                            unsigned blocksize = 4096;

                            sprintf(cmd, 
                                    "echo y | /sbin/mkfs -b %u %s %"PRIu64" "
                                    ">/dev/null 2>&1", blocksize, s.device, 
                                    ((uint64_t)((s.cap*1.10))<<20)/blocksize);
                            FPRINTF(stderr, "%s: creating file system (%s)\n", 
                                    g_hostname, cmd);               
                            if (system(cmd)!=0) {
                                fprintf(stderr, 
                                        "%s: failed to create fs on %s\n", 
                                        g_hostname, s.device);
                                exit(1);
                            }
                        }
                        
                        /* mount file system */
                        sprintf(cmd, "sudo mount -t ext2 %s %s", s.device, 
                                s.mkfs);
                        FPRINTF(stderr, "%s: mounting file system (%s)\n",
                                g_hostname, cmd);
                        if (system(cmd)!=0) {
                            fprintf(stderr, 
                                    "%s: failed to mount fs on %s\n", 
                                    g_hostname, s.device);                  
                            exit(1);
                        }
                    }
                    
                    if (strlen(s.external)) {
                        int rc = 0;
                        
#ifdef CONFIG_MPI
                        if (!g_no_sync && g_concurrent) {
                            MPI_Barrier(MPI_COMM_WORLD);
                        }
                        
                        /* spawn worker to run the external command.  
                           If --extern_master was specified, then only the
                           master node (rank 0) starts up the command. This
                           is useful for using fitness to measure the
                           performance of other MPI jobs */

                        if (!g_extern_mastered || g_myid==0) {
#endif
                            fprintf(stderr, "%s: running extern (%s %s)\n", 
                                    g_hostname, s.external, s.external_args);
                            START_WORKER_ELSE("extern_worker", 0, 
                                              extern_worker[0], 
                                              extern_worker_proc_t, NULL, 
                                              return -1);
#ifdef CONFIG_MPI
                        }
#endif
                        goto warm;
                    }
                    
                    /* open file */
                    if (strlen(s.file)) {
                        int fid;
                        char buffer[512];
                        int flags = O_RDWR|O_LARGEFILE;
                        
                        /* optionally create it */
                        if (!s.no_create) flags |= O_CREAT;
                        if ((fid=open(s.file, flags, 0666))==-1) {
                            fprintf(stderr, "failed to open %s (errno %i)\n", 
                                    s.file, errno);
                            return 1;
                        }
                        
                        /* fill file unless --allow_sparse specified */
                        if (!s.no_create) {
                            if (s.allow_sparse) {
                                FPRINTF(stderr, 
                                        "creating %"PRIu64" MB file (sparse)\n",
                                        s.cap);
                                if (lseek64(fid, (s.cap<<20)-1, SEEK_SET)!=
                                    ((s.cap<<20)-1)) {
                                    fprintf(stderr, 
                                            "creating lseek64 failed\n");
                                    return 1;
                                }
                                if (write(fid, buffer, 1)!=1) {
                                    fprintf(stderr, 
                                            "creating sparse write failed\n");
                                    return 1;
                                }
                            } else { 
                                void *buffer;
                                unsigned i;
                                uint64_t offset;
                                
                                if ((buffer=malloc(1<<20))==NULL) {
                                    fprintf(stderr, "out of memory\n");
                                    return 1;
                                }
                                FPRINTF(stderr, 
                                        "creating %"PRIu64" MB file [", s.cap);
                                for (i=0; i<s.cap; i++) {
                                    offset = (uint64_t) i<<20;
                                    if (lseek64(fid, offset, SEEK_SET)!=
                                        offset) {
                                        fprintf(stderr, "lseek64 failed\n");
                                        return 1;
                                    }
                                    if (write(fid, buffer, 1<<20)!=1<<20) {
                                        fprintf(stderr, 
                                                "creating write failed\n");
                                        return 1;
                                    }
                                    if (i%100==0) {
                                        FPRINTF(stderr, "%3i%%]\b\b\b\b\b", 
                                                (int) (100*i/s.cap));
                                    }
                                }
                                FPRINTF(stderr, "100%%]\n");
                                free(buffer);
                            }
                            close(fid);  
                            
                            /* if we wrote the file, let's unmount and flush
                               the target cache */
                            if (((strlen(s.file)&&!s.allow_sparse))&&
                                strlen(s.flush)) {
                                sprintf(cmd, "sudo umount %s", s.mkfs); 
                                FPRINTF(stderr, 
                                        "unmounting file system (%s)\n", cmd);
                                if (system(cmd)!=0) {
                                    fprintf(stderr, 
                                            "failed to unmount %s\n", s.mkfs);
                                    exit(1);
                                }
                                sprintf(cmd, "%s %s", s.flush, s.flush_args); 
                                FPRINTF(stderr, "flushing target (%s)\n", cmd);
                                system(cmd);
                                sprintf(cmd, "sudo mount -t ext2 %s %s", 
                                        s.device, s.mkfs); 
                                FPRINTF(stderr, "remounting file system (%s)\n",
                                        cmd);
                                system(cmd);
                            }
                        } else {
                            close(fid);  
                        }
                    }
                }
                
#ifdef CONFIG_MPI
                if (!g_no_sync && g_concurrent) {
                    MPI_Barrier(MPI_COMM_WORLD);
                }
#endif

            warm: 

                /* wait for external program to reach a certain point 
                   before warming */
                if (strlen(g_extern_warm_mark)) {
                    FPRINTF(stderr, "waiting for \"%s\"\n", g_extern_warm_mark);
                    while (!g_start_warming) {
                        ISCSI_MSLEEP(100);
                    }
                }
                
                if (!g_myid && (s.warm || s.term)) {
                    if (g_detail) {
                        FPRINTF(stderr, "%s: sample %i iter %i: ", g_hostname, 
                                sample, iter);
                    } 
                }
                
                /* warm */
                gettimeofday(&t_start, 0);
                
                /* initiator counters */

                if (s.i_counters) I_COUNTERS_GET(g_tid, g_lun,
                                                 g_i_counters_start);

                /* target counters */
                if (s.t_counters) T_COUNTERS_GET(g_tid, g_lun, 
                                                 g_t_counters_start);
                
                /* open I/O target (file, device, or iscsi direct) 
                   for each worker */
                if (phase == 0) {
                    unsigned flags = O_RDWR|O_CREAT|(s.direct?O_DIRECT:0)
                        |O_LARGEFILE;
                    
                    for (i=0; i<s.qdep; i++) {
                        if (strlen(s.file)) {
                            if ((g_fid[i]=open(s.file, flags, 0666))==-1) {
                                fprintf(stderr, 
                                        "failed to open %s (errno %i) for \
worker %i\n", 
                                        s.file, errno, i);
                                return -1;
                            }
                        } else if (strlen(s.device)) {
                            if ((g_fid[i]=open(s.device, flags, 0666))==-1) {
                                fprintf(stderr, 
                                        "worker failed to open %s (errno %i)\
for worker %i\n", 
                                        s.device, errno, i);
                                return -1;
                            }
                        } else {
                            g_fid[i] = -1;
                        }
                    }
                }
                
                /* start up worker threads to do I/O */
                if (!strlen(s.external)) {
                    for (k=0; k<s.qdep; k++) 
                        START_WORKER_ELSE("worker", k, worker[k], 
                                          worker_proc_t, 
                                          NULL, return -1);
                }

                /* warming period */
                if (s.warm!=0 || s.term!=0) {
                    do {
                        if (s.term) {
                            ISCSI_MSLEEP(1);
                        } else {
                            ISCSI_SLEEP(1); gettimeofday(&t_stop, 0);
                            s.c.warm = toSeconds(t_stop)-toSeconds(t_start);
                        }
                        
                        /* if i_counters is specified, then our warming
                           period is relative to the initiator counters */
                        if (s.i_counters) {
                            uint64_t bytes_sent, bytes_recv, bytes, ops;
                            
                            I_COUNTERS_GET(g_tid, g_lun, counters_stop);
                            OUTPUT_STATS(g_i_counters_start, counters_stop,
                                         NULL, counters_diff, 
                                         OUTPUT_TYPE_DATA|OUTPUT_TYPE_STRING);
                            sscanf(strstr(counters_diff, "i_TODEV"), 
                                   "i_TODEV %"PRIu64"", &bytes_sent);
                            sscanf(strstr(counters_diff, "i_FROMDEV"), 
                                   "i_FROMDEV %"PRIu64"", &bytes_recv);
                            sscanf(strstr(counters_diff, "i_OPS "), 
                                   "i_OPS %"PRIu64"", &ops);
                            xfer = (bytes_sent+bytes_recv)>>20;
                            s.c.ops = ops;
                        } else {
                            xfer = s.c.bytes>>20;
                        }
                        if (1 || xfer>last_xfer) {
                            if (!s.term) {
                                FPRINTF(stderr, 
                                        "(warming %3.0lf) %7i %sMB"
                                        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
                                        "\b\b\b\b\b\b\b\b\b\b",
                                        s.warm-s.c.warm, xfer, 
                                        s.i_counters?"i_":"f_");
                            } else {
                                FPRINTF(stderr, 
                                        "%7i of %7"PRIu64" %sMB"
                                        "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
                                        "\b\b\b\b\b\b\b", 
                                        xfer, s.term, s.i_counters?"i_":"f_");
                            }
                            last_xfer = xfer;
                        }
                        
#ifdef CONFIG_MPI
                        /* If an external process was started by the master
                           node, we want to know if it exited. We can probe
                           MPI to see if a message is waiting for us. If so,
                           we can set g_end_of_program to 1. */
                        
                        if (g_extern_mastered && g_myid) {
                            MPI_Status status;
                            int flag;
                            
                            if (MPI_Iprobe(0, MPI_ANY_TAG, MPI_COMM_WORLD, 
                                           &flag, &status) != MPI_SUCCESS) {
                                fprintf(stderr, "MPI_Iprobe() failed\n");
                                exit(0);
                            }
                            g_end_of_program = flag;
                        }
#endif
                        
                    } while (((s.term && (xfer<s.term))||
                              (!s.term && (s.c.warm<s.warm) && 
                               ((s.warm_mb==-1)||(xfer<s.warm_mb))&&
                               ((s.warm_io==-1)||(s.c.ops<s.warm_io))))
                             && !g_end_of_file && !g_end_of_program);

                    FPRINTF(stderr, "done warming ");
                    if ((s.term)&&(xfer>=s.term)) {
                        FPRINTF(stderr, "(%u of %"PRIu64" MB) -- terminating",
                                xfer, s.term);
                    }
                    if ((s.warm_io!=-1)&&(s.c.ops>=s.warm_io)) {
                        FPRINTF(stderr, "(%"PRIu64" of %u IOs)", 
                                s.c.ops, s.warm_io);
                    }
                    if ((s.warm_mb!=-1)&&(xfer>=s.warm_mb)) {
                        FPRINTF(stderr, "(%u of %u MB)", xfer, s.warm_mb);
                    }
                    if (s.c.warm>s.warm) {
                        FPRINTF(stderr, "(%.1lf of %i secs)", s.c.warm, s.warm);
                    }
                    if (g_end_of_file) {
                        FPRINTF(stderr, "(EOF @ %u MB, %"PRIu64" ops)", 
                                xfer, s.c.ops);
                    }
                    if (g_end_of_program) {
                        FPRINTF(stderr, "(EOP @ %u MB, %"PRIu64" ops)", 
                                xfer, s.c.ops);
                    }
                    FPRINTF(stderr, "                    \n");
                } else {
                    goto test;
                }
                
                /* counters from warming period */
            done_warming:
                s.c.ops_warm = s.c.ops;
                s.c.lat_warm = s.c.lat;
                s.c.bytes_warm = s.c.bytes_actual;
                gettimeofday(&t_stop, 0);
                
                /* initiator counters */
                if (s.i_counters) I_COUNTERS_GET(g_tid, g_lun, 
                                                 g_i_counters_stop);

                /* target counters */
                if (s.t_counters) T_COUNTERS_GET(g_tid, g_lun, 
                                                 g_t_counters_stop);
                
                /* stop workers if we're in terminate mode or
                   have reached end of trace */
                if (s.term || g_end_of_file || g_end_of_program) {
                    if (!strlen(s.external)) {
                        for (k=0; k<s.qdep; k++) 
                            /* do we need this? */
                            worker[k].state|= WORKER_STATE_STOPPING;
                        for (k=0; k<s.qdep; k++) 
                            STOP_WORKER_ELSE(worker[k], NO_SIGNAL, return -1);
                        ISCSI_LOCK_DESTROY_ELSE(&s.c.lock, return -1);
                    } else {
                        int i;
                        
#ifdef CONFIG_MPI                       
                        if (!g_extern_mastered || g_myid==0) {
                            STOP_WORKER_ELSE(extern_worker[0], 
                                             kill(g_extern_pid, SIGINT)||1,
                                             return -1);
                        }
                        if (g_end_of_program && g_extern_mastered && 
                            g_myid==0) {
                            for (i=1; i<g_num_procs; i++) {
                                MPI_Send(&i, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
                            }
                        }
#else
                        if (strlen(s.external)) {
                            cmd[1024];

                            if (strlen(s.external_shutdown)) {
                                fprintf(stderr, 
                                        "post-warm shutdown via \"%s\"\n", 
                                        s.external_shutdown);
                                system(s.external_shutdown);
                            } else {
                                sprintf(cmd, "iscsi-kill-tree %i\n", 
                                        g_extern_pid);
                            }
                            STOP_WORKER_ELSE(extern_worker[0], system(cmd)||1,
                                             return -1);
                        }
#endif
                    }
                }

                /* wait turn output to console */
                if (g_concurrent) WAIT_TURN; 

                /* fitness stats from warming period */
                s.c.warm = toSeconds(t_stop)-toSeconds(t_start);
                if (g_detail) {
                    FPRINTF(stderr, "%s: sample %i iter %i: ", 
                            g_hostname, sample, iter);
                }
                FPRINTF(stderr, "(%s) %.2f MB in %.2f sec %.2f MB/sec ", 
                        s.term?"term":"warm", s.c.bytes_warm/1048576.0,
                        s.c.warm, (s.c.bytes_warm/s.c.warm)/1048576.0);
                
                /* initiator stats from warming period */
                if (s.i_counters) {
                    char diff[ISCSI_COUNTERS_SIZE];
                    double i_secs, i_bw;
                    
                    OUTPUT_STATS(g_i_counters_start, g_i_counters_stop, 
                                 "iscsi-diff-initiator-counters", 
                                 diff, OUTPUT_TYPE_STRING);
                    sscanf(strstr(diff, "bw"), "bw %lf", &i_bw);
                    sscanf(strstr(diff, "secs"), "secs %lf", &i_secs);      
                    FPRINTF(stderr, "[initiator: %.2lf secs %.2lf MB/sec]", 
                            i_secs, i_bw);
                    memcpy(g_i_counters_start, g_i_counters_stop, 
                           ISCSI_COUNTERS_SIZE);
                }
                if (s.t_counters) {
                    memcpy(g_t_counters_start, g_t_counters_stop, 
                           ISCSI_COUNTERS_SIZE);
                }
                
                FPRINTF(stderr, "\n");
        
                /* notify next to output */
                NOTIFY_AND_WAIT;
                
                /* quit if in "terminate" mode or reached the end of trace */
                if (g_end_of_file) {
                    goto output;
                }
                if (s.term || g_end_of_program) goto cleanup;
                
                /* Done warming, start testing */
            test: last_xfer = 0;

                if (g_detail) {
                    FPRINTF(stderr, "%s: sample %i iter %i: ", g_hostname, 
                            sample, iter);
                }
                
                gettimeofday(&t_start, 0);
                
                do {
                    ISCSI_SLEEP(1); gettimeofday(&t_stop, 0);
                    s.c.test = toSeconds(t_stop)-toSeconds(t_start);
                    if (s.i_counters) {
                        uint64_t bytes_sent, bytes_recv, bytes;
                
                        I_COUNTERS_GET(g_tid, g_lun, counters_stop);
                        OUTPUT_STATS(g_i_counters_start, counters_stop, 
                                     NULL, counters_diff, 
                                     OUTPUT_TYPE_DATA|OUTPUT_TYPE_STRING);
                        sscanf(strstr(counters_diff, "TODEV"), 
                               "TODEV %"PRIu64"", &bytes_sent);
                        sscanf(strstr(counters_diff, "FROMDEV"), 
                               "FROMDEV %"PRIu64"", &bytes_recv);
                        xfer = (bytes_sent+bytes_recv)>>20;
                    } else {
                        xfer = (s.c.bytes-s.c.bytes_warm)>>20;
                    }
                    if (1 || xfer>last_xfer) {
                        FPRINTF(stderr, 
                                "(testing %3.0lf) %6i MB\b\b\b\b\b\b\b\b\b\b"
                                "\b\b\b\b\b\b\b\b\b\b\b\b\b", 
                                s.test-s.c.test, xfer);
                        last_xfer = xfer;
                    }
                    
#ifdef CONFIG_MPI
                    /* If an external process was started by the master
                       node, we want to know if it exited. We can probe
                       MPI to see if a message is waiting for us. If so,
                       we can set g_end_of_program to 1. */
                    
                    if (g_extern_mastered && g_myid) {
                        MPI_Status status;
                        int flag;
                        
                        if (MPI_Iprobe(0, MPI_ANY_TAG, MPI_COMM_WORLD, 
                                       &flag, &status) != MPI_SUCCESS) {
                            fprintf(stderr, "MPI_Iprobe() failed\n");
                            exit(0);
                        }
                        g_end_of_program = flag;
                    }
#endif
                    
                } while (s.c.test<s.test && !g_end_of_file && 
                         !g_end_of_program);

#ifdef CONFIG_MPI
                if (g_end_of_program && g_extern_mastered && g_myid==0) {
                    int i;
                    
                    for (i=1; i<g_num_procs; i++) {
                        MPI_Send(&i, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
                    }
                }
#endif

                FPRINTF(stderr, "done testing %5s         \n", 
                        g_end_of_file?"(EOF)":
                        (g_end_of_program?"(EOP)":"    "));
                
                /* get all ending counters */
                s.c.bytes_test = s.c.bytes - s.c.bytes_warm;
                s.c.ops_test = s.c.ops - s.c.ops_warm;
                s.c.lat_test = s.c.lat - s.c.lat_warm;
                
                /* initiator counters */
                if (s.i_counters) I_COUNTERS_GET(g_tid, g_lun, 
                                                 g_i_counters_stop);

                /* target counters */
                if (s.t_counters) T_COUNTERS_GET(g_tid, g_lun, 
                                                 g_t_counters_stop);
                
                /* stop workers */
                g_verbose = 1;
                if (!strlen(s.external)) {
                    for (k=0; k<s.qdep; k++) worker[k].state|=
                                                 WORKER_STATE_STOPPING;
                    for (k=0; k<s.qdep; k++) STOP_WORKER_ELSE(worker[k], 
                                                              NO_SIGNAL, 
                                                              return -1);
                }
                ISCSI_LOCK_DESTROY_ELSE(&s.c.lock, return -1);
                
                /* output header (only for first sample) */
            output:             if (!skip&&!sample&&!iter) {

                    fprintf(stream, 
                            "%3s %1s %5s %5s %3s %5s %5s %4s %5s %5s %6s \
%6s %6s %8s %8s", 
                            "s", "i", "cap", "qdep", "wr", "wrnd", "rrnd", 
                            "str", "wrsz", "rdsz", "warm", "test", "bw",
                            "iops", "lat");
                    if (s.i_counters) {
                        OUTPUT_STATS(g_i_counters_start, g_i_counters_stop, 
                                     "iscsi-diff-initiator-counters", 
                                     stream, OUTPUT_TYPE_HEADER|
                                     OUTPUT_TYPE_STREAM);
                    }
                    if (s.t_counters) {
                        OUTPUT_STATS(g_t_counters_start, g_t_counters_stop, 
                                     "iscsi-diff-target-counters", 
                                     stream, 
                                     OUTPUT_TYPE_HEADER|OUTPUT_TYPE_STREAM);
                    }
                    fprintf(stream, "\n"); fflush(stream);
                }

                if (g_end_of_file) {
                    fprintf(stream, "%3i %1i EOF\n", sample, iter); 
                    fflush(stream);
                    goto cleanup;
                }
                
                /* fitness, initiator and target stats to file */
                if (!strlen(s.external)) {
                    fprintf(stream, "%3i %1i %5"PRIu64" %5i %3i %5i %5i %4i "
                            "%5i %5i %6.1lf %6.1lf %6.2lf %8.0lf %8.2lf", 
                            sample, iter, s.cap, s.qdep, s.wr, s.wrnd, 
                            s.rrnd, s.str, s.wrsz, s.rdsz, s.c.warm,    
                            s.c.test,                            /* f_secs */
                            (s.c.bytes_test/s.c.test)/1048576.0, /* f_bw   */
                            (s.c.ops_test/s.c.test),             /* f_iops */
                            (s.c.lat_test/s.c.ops_test)/1000.0); /* f_lat  */
                } else {
                    fprintf(stream, 
                            "%3i %1i %5"PRIu64" %5i %3i %5i %5i %4i %5i %5i "
                            "%6.1lf %6.1lf %6.2lf %8.0lf %8.2lf", sample, iter,
                            (uint64_t) 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0.0, 
                            0.0, 0.0);  
                }

                if (s.i_counters) {
                    OUTPUT_STATS(g_i_counters_start, g_i_counters_stop, 
                                 "iscsi-diff-initiator-counters",
                                 stream, OUTPUT_TYPE_DATA|OUTPUT_TYPE_STREAM);
                }
                if (s.t_counters) {
                    OUTPUT_STATS(g_t_counters_start, g_t_counters_stop, 
                                 "iscsi-diff-target-counters",
                                 stream, OUTPUT_TYPE_DATA|OUTPUT_TYPE_STREAM);
                }
                fprintf(stream, "\n"); fflush(stream);
        
                /* wait turn output to console */
                if (g_concurrent) WAIT_TURN; 
                
                /* fitness, initiator and target stats to stderr */             
                if (g_detail) {
                    FPRINTF(stderr, "%s: sample %i iter %i: ", 
                            g_hostname, sample, iter);
                }
                FPRINTF(stderr, "(test) %.2f MB in %.2f sec %.2f MB/sec ", 
                        s.c.bytes_test/1048576.0, s.c.test,
                        strlen(s.external)?0:
                        (s.c.bytes_test/s.c.test)/1048576.0);

                if (s.i_counters) {
                    char diff[ISCSI_COUNTERS_SIZE];
                    double i_secs, i_bw;
                    
                    OUTPUT_STATS(g_i_counters_start, g_i_counters_stop, 
                                 "iscsi-diff-initiator-counters", 
                                 diff, OUTPUT_TYPE_STRING);
                    sscanf(strstr(diff, "bw"), "bw %lf", &i_bw);
                    sscanf(strstr(diff, "secs"), "secs %lf", &i_secs);
                    FPRINTF(stderr, "[initiator: %.2lf secs %.2lf MB/sec]", 
                            i_secs, i_bw);
                    memcpy(g_i_counters_start, g_i_counters_stop, 
                           ISCSI_COUNTERS_SIZE);
                }

                FPRINTF(stderr, "\n");

                /* stop external process (if any) */
#ifdef CONFIG_MPI
                if (strlen(s.external) && (!g_extern_mastered || g_myid==0)) {
#else
                    if (strlen(s.external)) {
#endif

                        if (strlen(s.external_shutdown)) {
                            fprintf(stderr, 
                                    "post-test external shutdown via \"%s\"\n",
                                    s.external_shutdown);
                            system(s.external_shutdown);
                            STOP_WORKER_ELSE(extern_worker[0], NO_SIGNAL, 
                                             return -1);
                        } else {
                            cmd[1024];

                            sprintf(cmd, "iscsi-kill-tree %i\n", g_extern_pid);
                            STOP_WORKER_ELSE(extern_worker[0], 
                                             system(cmd)||1, return -1);
                        }
                    }

                    /* notify next to output */
                    NOTIFY_AND_WAIT;

                cleanup:

                    /* close+sync+unmount+flush unless "repeating" */
                    if (!repeat) {      

                        /* close all file handles */
                        for (i=0; i<s.qdep; i++) {
                            if (strlen(s.file)||strlen(s.device)) {
                                close(g_fid[i]);
                            }
                        }

                        /* (S)ync disks */

                        if (g_detail) {
                            FPRINTF(stderr, "%s: sample %i iter %i: ", 
                                    g_hostname, sample, iter);
                        }
 			if (!g_no_fs_sync) {
                        	FPRINTF(stderr, "[S   ]\b\b\b\b");
                        	system("sync");  
			} else {
                        	FPRINTF(stderr, "[X   ]\b\b\b\b");
			}

                        /* (U)nmount file system */
                        if (strlen(s.mkfs)) {
                            char command[256];

                            sprintf(command, "sudo umount %s", s.mkfs);
                            if (system(command)!=0) {
                                FPRINTF(stderr, "failed to umount %s", s.mkfs);
                                exit(1);
                            }
                            FPRINTF(stderr, "U");
                        
                        } else {
                            FPRINTF(stderr, "-");
                        }              

                        /* (F)lush target cache for next iteration */
                        if (strlen(s.flush)&&(!strlen(s.mkfs)||
                                              strlen(s.external))) {
                            FPRINTF(stderr, "F");
                            sprintf(cmd, "%s %s\n", s.flush, s.flush_args);
                            system(cmd);
                        } else {
                            FPRINTF(stderr, "-");
                        }

                        /* Take a breather */
                        for (k=s.breather; k>0; k--) {
                            FPRINTF(stderr, "%i\b", k);
                            sleep(1);
                        }
                        FPRINTF(stderr, "0]");
                    } 

#ifdef CONFIG_MPI
                    if (!g_no_sync && g_concurrent) {
                        FPRINTF(stderr, " [sync]");
                        MPI_Barrier(MPI_COMM_WORLD);
                    }
#endif
                    FPRINTF(stderr, "\n");

                } /* end iter loop */     

                /* Each iteration of the outer loop creates a random workload
                   sample.  In order to keep RNG predictable w/ a given seed,
                   we start each sample from the outer loop and skip over the
                   ones already completed.  This is necessary given that the
                   worker threads also use the rand() function. */
                skip = sample+1; samples--; goto top; 
            } else {
                if (!filter) skip--;
            }
        } /* end sample loop */

        if (repeat) {
            last_wr_offset = s.c.wr_offset;
            last_rd_offset = s.c.rd_offset;
            phase++;
            goto repeat;
        }

    finish:

#ifdef CONFIG_MPI
        if (!g_concurrent) {
            NOTIFY_AND_WAIT;
        } 
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Finalize();
#endif

        return 0;
    }

#else

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <errno.h>

static int sg_cmd_ioctl(int g_fid, char *cmd, int cmd_len, unsigned char *data, 
                        unsigned len, int direction, int timeout);

int main(int c, char *argv[]) {
        char cdb[10];
	unsigned offset=123*512;
	unsigned char buffer[4096];
	int writing=1;
	unsigned len=4096;
        int fid;

	printf("Hello, world!\n");
        if ((fid=open("/dev/sg4", O_RDWR))==-1) {
            fprintf(stderr, "error opening file\n");
        }
        memset(cdb, 0, 10);
        if (writing) cdb[0] = 0x2a;
        else cdb[0] = 0x28;                
        if (offset%512) fprintf(stderr, "WARN: offset is not a multiple of 512\n");
        *((unsigned *)(cdb+2)) = htonl(offset/512);
        *((unsigned short *)(cdb+7)) = htons(len/512);
        if (sg_cmd_ioctl(fid, cdb, 10, buffer, len, writing?SG_DXFER_TO_DEV:SG_DXFER_FROM_DEV, 100000)!=0) {
            fprintf(stderr, "SCSI generic I/O failed\n");
        }
	return 0;
}
#endif

static int sg_cmd_ioctl(int g_fid, char *cmd, int cmd_len, unsigned char *data, 
                        unsigned data_len, int direction, int timeout) {
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
    if ((rc=ioctl(g_fid, SG_IO, &sg_hdr))!=0) {
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
