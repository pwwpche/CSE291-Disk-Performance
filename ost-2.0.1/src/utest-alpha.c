
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
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <inttypes.h>
#include "iscsi.h"
#include "initiator.h"
#include "tests.h"
#include "debug.h"
#include "config.h"
#include "research.h"

#define DFLT_ISCSI_CONFIG IPS_CONFIG_FILE

#define WARM_TIME 10
#define TEST_TIME 30 

static int g_main_pid;

static void SigIntHandler(int sigNum) {
    if (g_main_pid != OST_GETPID) return;
    if (initiator_shutdown()==-1) {
        TRACE_ERROR("initiator_shutdown() failed\n");
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    int tids;
    char c;
    struct sigaction act;
    int full = 0;
    int all_targets = 1;
    uint64_t lun = 0;
    uint64_t tid = 0;
    int start, stop;
    int samples = 0;
    int iters = 1;
    int warm = WARM_TIME;
    int test = TEST_TIME;
    int i;
    extern char *optarg;
    char config[256] = DFLT_ISCSI_CONFIG;
    int seed = 0;
    char file[256] = "";
    FILE *stream = stdout;
    int skip = 0;
    int replay = 0;
    int verbose = 0;
    int family = AF_INET;
    unsigned char do_ioctl=0;
    char *do_ioctl_arg=NULL;
    char ioctl_buffer[ISCSI_COUNTERS_SIZE];
    int do_read_block=0;
    uint64_t block_to_read=0;
    char tracefile[256] = "";

    /* process command args */
    while ((c=getopt_long (argc, argv, "ft:hc:s:i:S:o:x:T:W:RvVF:PUCXYr:Z:E", 
                           NULL, NULL))!=-1) {
        switch(c) {
        case 'f':
            full = 1;
            break;
        case 't':
            all_targets = 0;
            tid = atoi(optarg);
            break;
        case 's':
            samples = atoi(optarg);
            replay = 0;
            break;
        case 'i':
            iters = atoi(optarg);
            break;
        case 'c':
            strcpy(config, optarg);
            break;
        case 'S':
            seed = atoi(optarg);
            break;
        case 'x':
            skip = atoi(optarg);
            break;
        case 'o':
            strcpy(file, optarg);
            break;
        case 'W':
            warm = atoi(optarg);
            break;
        case 'T':
            test = atoi(optarg);
            break;
        case 'R':
            samples = 0;
            replay = 1;
            break;
        case 'h':
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -c <configfile> Config file (default %s)\n", 
                   DFLT_ISCSI_CONFIG);
            printf("  -t <tid>        Test all or just specified tid"
                   " (default all)\n"); 
            printf("  -f              Run full tests (default no)\n");
            printf("  -F <family>     Address family (default AF_INET)\n");
            printf("  -v              Output version and exit\n");
#ifdef CONFIG_EXPERIMENTAL
            printf("  -P              Pin\n");
            printf("  -U              Unpin\n");
#endif
            printf("  -E              Device exit (flushes trace records)\n");
            printf("  -X              Tracing ON\n");
            printf("  -Y              Tracing OFF\n");
            printf("  -Z <tracefile>  Set tracing output\n");
            printf("  -C              Output counters on target\n");
            printf("  -r <lba>        Read and print block\n");
            printf("  -V              Verbose\n");
#if 0
            printf("       -o <file>        -- output file for results "
                   " (default stdout)\n");
            printf("       -s <samples>     -- randomly sample\n");
            printf("       -i <iters>       -- iters per sample\n");
            printf("       -S <seed>        -- input into srand\n");
            printf("       -W <warm>        -- warm time (default %i)\n", 
                   WARM_TIME);
            printf("       -T <test>        -- test time (default %i)\n", 
                   TEST_TIME);
            printf("       -x <skip>        -- skip over x samples\n");
            printf("       -R               -- replay from stdin\n");
#endif
            return 0;
        case 'v':
            printf("%s\n", PACKAGE_STRING);
            return 0;
        case 'V':
            verbose = 1;
            break;
        case 'r':
            block_to_read = atoi(optarg);
            do_read_block = 1;
            break;
#ifdef CONFIG_EXPERIMENTAL
        case 'P':
            do_ioctl = ISCSI_IOCTL_MARKERS_SET;
            break;
        case 'U':
            do_ioctl = ISCSI_IOCTL_MARKERS_CLEAR;
            break;
        case 'C':
            do_ioctl = ISCSI_IOCTL_CACHE_COUNTERS_GET;
            do_ioctl_arg = ioctl_buffer;
            break;
#endif
        case 'X':
            do_ioctl = ISCSI_IOCTL_TRACING_ON;
            break;
        case 'Y':
            do_ioctl = ISCSI_IOCTL_TRACING_OFF;
            break;
        case 'E':
            do_ioctl = ISCSI_IOCTL_TARGET_EXIT;
            break;
        case 'Z':
            do_ioctl = ISCSI_IOCTL_TRACING_SET;
            strcpy(tracefile, optarg);
            do_ioctl_arg = tracefile;
            break;
        case 'F':
            family = atoi(optarg);
            break;
        default:
            return 1;
        }  
    }

    /* signal handlers */
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL); 
    signal(SIGINT, SigIntHandler);
    g_main_pid = OST_GETPID;

    /* initialize tracing */
    DEBUG_INIT;

    /* initialize initiator */
    if ((tids=initiator_init(config, verbose, family))==-1) {
        TRACE_ERROR("initiator_init() failed\n");
        return -1;
    }

    if (all_targets) {
        start = 0;
        stop = tids;
    } else {
        start = tid;
        stop = tid+1;
    } 

    if (do_read_block) {
        unsigned char buffer[4096];
        if (iscsi_issue_io(tid, lun, 0, block_to_read*512, 
                           4096, buffer, 512)==-1) {
            TRACE_ERROR("iscsi_issue_io() failed\n");
            return -1;
        }
        printf("BEGIN BLOCK %"PRIu64"\n", block_to_read);
        PRINT_BUFF(TRACE_SCSI, 0, buffer, 4096, 32);
        printf("END BLOCK %"PRIu64"\n", block_to_read);
        return 0;
    }

    if (do_ioctl) {
        for (i=start; i<stop; i++) {
            if (iscsi_ioctl(i, lun, do_ioctl, do_ioctl_arg)!=0) {
                TRACE_ERROR("iscsi_ioctl(0x%x) failed\n", do_ioctl);
                return -1;
            }
            if (do_ioctl == ISCSI_IOCTL_CACHE_COUNTERS_GET) {
                printf("%s", ioctl_buffer);
            }
        }
        return 0;
    }

    if (strlen(file)) {
        if (skip) {
            if ((stream=fopen(file, "a+"))==NULL) {
                TRACE_ERROR("error opening output file \"%s\"\n", file);
                return -1;
            }
        } else {
            if ((stream=fopen(file, "w+"))==NULL) {
                TRACE_ERROR("error opening output file \"%s\"\n", file);
                return -1;
            }
        }
    }

    for (i=start; i<stop; i++) {
        if (test_all(i, lun, full, samples, iters, seed, stream, skip, warm, 
                     test, replay)!=0) {
            TRACE_ERROR("test_all() failed\n");
            return -1;
        }
    }

    /* shutdown initiator */
    if (initiator_shutdown()==-1) {
        TRACE_ERROR("initiator_shutdown() failed\n");
        return -1;
    }

    /* shutdown tracing */
    DEBUG_SHUTDOWN;

    /* close output file */
    if (strlen(file)) fclose(stream);

    return 0;
}
