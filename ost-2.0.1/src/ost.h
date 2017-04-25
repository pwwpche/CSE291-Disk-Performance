/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLIG OR USING. By
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

#ifndef _OST_H_
#define _OST_H_
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/blkdev.h>
#include <linux/string.h>
#include <linux/smp_lock.h>
#include <linux/sched.h>
#include <linux/socket.h>
#include <linux/version.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#endif

/*
 * Linkage
 */
#ifdef __cplusplus
#define LNKG extern "C"
#else
#define LNKG
#endif

/*
 * Debugging output
 */
#ifdef __KERNEL__
#define OST_PRINT printk
#else
#define OST_PRINT printf
#endif

/*
 * Sleeping
 */
#ifdef __KERNEL__
#define OST_SLEEP_SPIN(N) {unsigned future = jiffies+N*HZ;    \
        while (jiffies<future) OST_SPIN;}
#define OST_SLEEP(N) {set_current_state(TASK_INTERRUPTIBLE);  \
        schedule_timeout(N*HZ);}
#define OST_USLEEP(N) {set_current_state(TASK_INTERRUPTIBLE); \
        schedule_timeout(N*HZ/1000000);}
#else
#define OST_SLEEP_SPIN(T) sleep(T)
#define OST_SLEEP(T) sleep(T)
#define OST_USLEEP(T) usleep(T)
#define OST_MSLEEP(T) usleep(T*1000)
#endif

/*
 * Process and thread IDs
 */
#ifdef __KERNEL__
#define OST_GETPID current->pid
#else
#define OST_GETPID getpid()
#define OST_GETTID syscall(SYS_gettid)
#endif

/*
 * Error reporting
 */

int ost_debug_init(void);
int ost_debug_shutdown(void);
int ost_debug_lock(void);
int ost_debug_unlock(void);

extern char g_ost_debug_buff[8192];

#ifdef __KERNEL__
#define OST_TRACE_ERROR(args...) {                                                \
        ost_debug_lock();                                                         \
        sprintf(g_ost_debug_buff, args);                                          \
        OST_PRINT("%s:%s:%i:pid %i: *** ERROR *** %s",                            \
              __FILE__, __FUNCTION__, __LINE__, OST_GETPID, g_ost_debug_buff);    \
        ost_debug_unlock();                                                       \
    }
#define PRINT_STDERR(args...) {                                                   \
        ost_debug_lock();                                                         \
        sprintf(g_ost_debug_buff, args);                                          \
        OST_PRINT("%s", g_ost_debug_buff);                                        \
        ost_debug_unlock();                                                       \
    }
#else
#define OST_TRACE_ERROR(args...) {                                                \
        ost_debug_lock();                                                         \
        sprintf(g_ost_debug_buff, args);                                          \
        fprintf(stderr, "%s:%s:%i:pid %i: *** ERROR *** %s",                      \
                __FILE__, __FUNCTION__, __LINE__, OST_GETPID, g_ost_debug_buff);  \
        ost_debug_unlock();                                                       \
    }
#define PRINT_STDERR(args...) {                                                   \
        ost_debug_lock();                                                         \
        sprintf(g_ost_debug_buff, args);                                          \
        fprintf(stderr, "%s", g_ost_debug_buff);                                  \
        ost_debug_unlock();                                                       \
    }
#endif

/*
 * Locking
 */
#ifdef __KERNEL__
typedef struct semaphore OST_LOCK_T;
typedef struct {
    spinlock_t lock;
    unsigned long flags;
} OST_SPIN_LOCK_T;
#else
typedef pthread_mutex_t OST_LOCK_T;
typedef OST_LOCK_T OST_SPIN_LOCK_T;
#endif

LNKG int ost_lock_init(OST_LOCK_T *l);
LNKG int ost_lock(OST_LOCK_T *l);
LNKG int ost_lock_ni(OST_LOCK_T *l);
LNKG int ost_unlock(OST_LOCK_T *l);
LNKG int ost_lock_destroy(OST_LOCK_T *l);

int ost_spin_lock_init(OST_SPIN_LOCK_T *l);
int ost_spin_lock(OST_SPIN_LOCK_T *l);
int ost_spin_unlock(OST_SPIN_LOCK_T *l);
int ost_spin_lock_destroy(OST_SPIN_LOCK_T *l);

#define OST_LOCK_INIT_ELSE(L, ELSE)                     \
    if (ost_lock_init(L)!=0) {                          \
        OST_TRACE_ERROR("ost_lock_init() failed\n");    \
        ELSE;                                           \
    }
#define OST_LOCK_ELSE(L, ELSE)                          \
    if (ost_lock(L)!=0) {                               \
        ELSE;                                           \
    }

#define OST_LOCK_NI_ELSE(L, ELSE)                       \
    if (ost_lock_ni(L)!=0) {                            \
        ELSE;                                           \
    }

#define OST_UNLOCK_ELSE(L, ELSE)                        \
    if (ost_unlock(L)!=0) {                             \
        OST_TRACE_ERROR("ost_lock() failed\n");         \
        ELSE;                                           \
    }

#define OST_LOCK_DESTROY_ELSE(L, ELSE)                  \
    if (ost_lock_destroy(L)!=0) {                       \
        OST_TRACE_ERROR("ost_lock_destroy() failed\n"); \
        ELSE;                                           \
    }

#define OST_SPIN_LOCK_INIT_ELSE(L, ELSE)                        \
    if (ost_spin_lock_init(L)!=0) {                             \
        OST_TRACE_ERROR("ost_spin_lock_init() failed\n");       \
        ELSE;                                                   \
    }
#define OST_SPIN_LOCK_ELSE(L, ELSE)                     \
    if (ost_spin_lock(L)!=0) {                          \
        OST_TRACE_ERROR("ost_spin_lock() failed\n");    \
        ELSE;                                           \
    }
#define OST_SPIN_UNLOCK_ELSE(L, ELSE)                   \
    if (ost_spin_unlock(L)!=0) {                        \
        OST_TRACE_ERROR("ost_spin_lock() failed\n");    \
        ELSE;                                           \
    }
#define OST_SPIN_LOCK_DESTROY_ELSE(L, ELSE)                     \
    if (ost_spin_lock_destroy(L)!=0) {                          \
        OST_TRACE_ERROR("ost_spin_lock_destroy() failed\n");    \
        ELSE;                                                   \
    }

/*
 * Counters
 */
typedef struct ost_device_counters_t {
    uint64_t ops;
    uint64_t ops_wr;
    uint64_t ops_rd;
    uint64_t ops_bidi;
    uint64_t ops_other;
    uint64_t outstanding;
    uint64_t outstanding_wr;
    uint64_t outstanding_rd;
    uint64_t outstanding_bidi;
    uint64_t outstanding_other;
    uint64_t depths;
    uint64_t depths_wr;
    uint64_t depths_rd;
    uint64_t depths_bidi;
    uint64_t depths_other;
    uint64_t lats;
    uint64_t lats_wr;
    uint64_t lats_rd;
    uint64_t lats_bidi;
    uint64_t lats_other;
    uint64_t last_lba;
    uint64_t last_lba_wr;
    uint64_t last_lba_rd;
    uint64_t jumps;
    uint64_t jumps_wr;
    uint64_t jumps_rd;
    uint64_t blocks;
    uint64_t blocks_wr;
    uint64_t blocks_rd;
    uint64_t bytes;
    //uint64_t bytes_todev;
    //uint64_t bytes_fromdev;
    OST_SPIN_LOCK_T lock;
    char name[64];
} OST_DEVICE_COUNTERS_T;

int ost_init_counters(OST_DEVICE_COUNTERS_T *counters, char *name);
void ost_reset_counters(OST_DEVICE_COUNTERS_T *counters);
int ost_update_counters_pre_via_cdb(OST_DEVICE_COUNTERS_T *counters, unsigned char *cdb);
int ost_update_counters_pre(OST_DEVICE_COUNTERS_T *counters, int writing, uint64_t lba, unsigned len);
int ost_update_counters_post_via_cdb(OST_DEVICE_COUNTERS_T *counters, unsigned char *cdb, unsigned lat, int locked);
int ost_update_counters_post(OST_DEVICE_COUNTERS_T *counters, int writing, unsigned len, unsigned lat, int locked);
void ost_print_counters(OST_DEVICE_COUNTERS_T *counters, char *buffer);
int ost_destroy_counters(OST_DEVICE_COUNTERS_T *counters);
#endif
