
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
 * Intel iSCSI Driver
 */

#ifndef _DRIVER_H_
#define _DRIVER_H_

/*
 * Driver configuration
 */

#define CONFIG_DRIVER_MAX_LUNS 16
#define CONFIG_DRIVER_PROBE     0
#define CONFIG_DRIVER_RAMDISK   1

#define DRIVER_STATS_RESET 1

typedef struct iscsi_driver_stats_t {
    int outstanding;        /* number of outstanding commands */
    int outstanding_wr;     /* number of outstanding writes */
    int outstanding_rd;     /* number of outstanding reads */
    int outstanding_bidi;   /* number of outstanding bidi */
    int outstanding_other;  /* number of outstanding bidi */
    uint64_t succeeded;
    uint64_t failed;
    uint64_t aborted;
    uint64_t latencies;       /* for calculating average latency */
    uint64_t latencies_wr;    /* for calculating average write latency */
    uint64_t latencies_rd;    /* for calculating average read latency */
    uint64_t latencies_bidi;  /* for calculating average bidi latency */
    uint64_t latencies_other; /* for calculating average other latency */
    uint64_t depths;          /* for calculating average queue depth */
    uint64_t depths_wr;       /* for calculating average write queue depth */
    uint64_t depths_rd;       /* for calculating average read queue depth */
    uint64_t depths_bidi;     /* for calculating average bidi queue depth */
    uint64_t depths_other;    /* for calculating average other queue depth */

    /* These are for calculating spatial randomness.  Jump are in units of blocks */
    uint64_t last_lba, last_wr_lba, last_rd_lba;
    uint64_t read_jumps;  
    uint64_t write_jumps;
    uint64_t jumps;

    uint64_t bytes_tx, q_bytes_tx;
    uint64_t bytes_rx, q_bytes_rx;
    uint64_t ops_wr, ops_rd, ops_bidi, ops_other;
    unsigned device_resets, bus_resets, host_resets;    
    unsigned min_response, max_response;
    unsigned avg_response_10; 
    unsigned avg_response_10_count;
    unsigned avg_response_100; 
    unsigned avg_response_100_count;
    unsigned avg_response_1000; 
    unsigned avg_response_1000_count;
    unsigned flags;
    uint32_t min_lba_a, min_lba_b;
    uint32_t max_lba_a, max_lba_b;
} ISCSI_DRIVER_STATS_T;
#endif /* _DRIVER_H_ */
