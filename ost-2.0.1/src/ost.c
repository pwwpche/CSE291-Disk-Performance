
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
#include <net/sock.h> /* for in_interrupt() */
#else
#include <inttypes.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include "ost.h"

/*
 * Device counters
 */
int ost_init_counters(OST_DEVICE_COUNTERS_T *counters, char *name) {
    strcpy(counters->name, name);
    OST_SPIN_LOCK_INIT_ELSE(&counters->lock, return -1);
    ost_reset_counters(counters);

    return 0;
}

void ost_reset_counters(OST_DEVICE_COUNTERS_T *counters) {
    counters->ops = 0;
    counters->ops_wr = 0;
    counters->ops_rd = 0;
    counters->ops_bidi = 0;
    counters->ops_other = 0;
#if 0 /* these should eventually reset themselves to zero */
    counters->outstanding = 0;
    counters->outstanding_wr = 0;
    counters->outstanding_rd = 0;
    counters->outstanding_bidi = 0;
    counters->outstanding_other = 0;
#endif
    counters->depths = 0;
    counters->depths_wr = 0;
    counters->depths_rd = 0;
    counters->depths_bidi = 0;
    counters->depths_other = 0;
    counters->lats = 0;
    counters->lats_wr = 0;
    counters->lats_rd = 0;
    counters->lats_bidi = 0;
    counters->lats_other = 0;
    counters->last_lba = 0;
    counters->last_lba_wr = 0;
    counters->last_lba_rd = 0;
    counters->jumps = 0;
    counters->jumps_wr = 0;
    counters->jumps_rd = 0;
    counters->blocks = 0;
    counters->blocks_wr = 0;
    counters->blocks_rd = 0;
    counters->bytes = 0;
    //counters->bytes_todev = 0;
    //counters->bytes_fromdev = 0;
}

void ost_print_counters(OST_DEVICE_COUNTERS_T *counters, char *ptr) {
    uint64_t usecs;
    char name[128];
#ifdef __KERNEL__
    usecs=(jiffies*1000000)/HZ;
#else
    struct timeval time;

    gettimeofday(&time, NULL);
    usecs = time.tv_sec*1000000+time.tv_usec;
#endif

    sprintf(name, "%s[%s]", counters->name, "request");
    sprintf(ptr+strlen(ptr), 
            "%20s: usecs %"PRIu64
            " ops %"PRIu64
            " blocks %"PRIu64
            " writes %"PRIu64
            " writeblocks %"PRIu64
            " reads %"PRIu64
            " readblocks %"PRIu64
            "\n",
            name,
            usecs,
            counters->ops, counters->blocks, 
            counters->ops_wr, counters->blocks_wr, 
            counters->ops_rd, counters->blocks_rd);
    
    sprintf(name, "%s[%s]", counters->name, "latency");
    sprintf(ptr+strlen(ptr), 
            "%20s: usecs %"PRIu64
            " depths %"PRIu64
            " depths_wr %"PRIu64
            " depths_rd %"PRIu64
            " lats %"PRIu64
            " lats_wr %"PRIu64
            " lats_rd %"PRIu64"\n",
            name,
            usecs,
            counters->depths, counters->depths_wr, counters->depths_rd,
            counters->lats, counters->lats_wr, counters->lats_rd);

    sprintf(name, "%s[%s]", counters->name, "spatial");
    sprintf(ptr+strlen(ptr), 
            "%20s: usecs %"PRIu64
            " jumps %"PRIu64
            " jumps_wr %"PRIu64
            " jumps_rd %"PRIu64"\n",
            name,
            usecs,
            counters->jumps, counters->jumps_wr, counters->jumps_rd);
}

int ost_update_counters_pre_via_cdb(OST_DEVICE_COUNTERS_T *counters, unsigned char *cdb) {
    unsigned lba = 0;
    unsigned len = 0;

    if ((cdb[0] == 0x2a)||(cdb[0]==0x28)) {
        ((unsigned char *)&lba)[0] = cdb[5];
        ((unsigned char *)&lba)[1] = cdb[4];
        ((unsigned char *)&lba)[2] = cdb[3]; 
        ((unsigned char *)&lba)[3] = cdb[2];
        ((unsigned char *)&len)[0] = cdb[8];
        ((unsigned char *)&len)[1] = cdb[7];
        return ost_update_counters_pre(counters, (cdb[0] == 0x2a), lba, len);
    } else {
        OST_SPIN_LOCK_ELSE(&counters->lock, return -1);
        counters->ops++;
        counters->depths += ++counters->outstanding;
        counters->ops_other++;
        counters->depths_other += ++counters->outstanding_other;
        OST_SPIN_UNLOCK_ELSE(&counters->lock, return -1);
        return 0;
    }
}

int ost_update_counters_pre(OST_DEVICE_COUNTERS_T *counters, int writing, uint64_t lba, unsigned len) {
    OST_SPIN_LOCK_ELSE(&counters->lock, return -1);
    counters->depths += ++counters->outstanding;
    counters->jumps += abs(lba-counters->last_lba);
    counters->last_lba = lba+len;
    if (writing) {
        counters->depths_wr += ++counters->outstanding_wr;
        counters->jumps_wr += abs(lba-counters->last_lba_wr);
        counters->last_lba_wr = lba+len;
    } else {
        counters->depths_rd += ++counters->outstanding_rd;
        counters->jumps_rd += abs(lba-counters->last_lba_rd);
        counters->last_lba_rd = lba+len;
    }
    OST_SPIN_UNLOCK_ELSE(&counters->lock, return -1);

    return 0;

}

int ost_update_counters_post_via_cdb(OST_DEVICE_COUNTERS_T *counters, unsigned char *cdb, 
                                     unsigned lat, int locked) {
    if ((cdb[0] == 0x2a)||(cdb[0]==0x28)) {
        unsigned len;

        ((unsigned char *)&len)[0] = cdb[8];
        ((unsigned char *)&len)[1] = cdb[7];
        return ost_update_counters_post(counters, (cdb[0] == 0x2a), len, lat, locked);
    } else {
        if (!locked)
            OST_SPIN_LOCK_ELSE(&counters->lock, return -1);
        counters->outstanding--;
        counters->lats += lat;
        counters->lats_other += lat;
        counters->outstanding_other--;
        if (!locked)
            OST_SPIN_UNLOCK_ELSE(&counters->lock, return -1);

        return 0;
    }
}

int ost_update_counters_post(OST_DEVICE_COUNTERS_T *counters, int writing, unsigned len, unsigned lat, int locked) {
    if (!locked) 
        OST_SPIN_LOCK_ELSE(&counters->lock, return -1);
    counters->ops++;
    counters->outstanding--;
    counters->blocks += len;
    counters->lats += lat;
    if (writing) {
        counters->ops_wr++;
        counters->lats_wr += lat;
        counters->outstanding_wr--;
        counters->blocks_wr += len;
    } else {
        counters->ops_rd++;
        counters->lats_rd += lat;
        counters->outstanding_rd--;
        counters->blocks_rd += len;
    }
    if (!locked) 
        OST_SPIN_UNLOCK_ELSE(&counters->lock, return -1);
    
    return 0;
}

int ost_destroy_counters(OST_DEVICE_COUNTERS_T *counters) {
    OST_SPIN_LOCK_DESTROY_ELSE(&counters->lock, return -1);

    return 0;
}

/*
 * Debugging output
 */
char g_ost_debug_buff[8192];

static OST_SPIN_LOCK_T g_ost_debug_lock;
static int g_ost_debug_initialized = 0;
int ost_debug_init(void) {
    if (g_ost_debug_initialized) return 0;
    OST_SPIN_LOCK_INIT_ELSE(&g_ost_debug_lock, return -1);
    g_ost_debug_initialized = 1;
    return 0;
}

int ost_debug_shutdown(void) {
    if (!g_ost_debug_initialized) return 0;
    OST_SPIN_LOCK_DESTROY_ELSE(&g_ost_debug_lock, return -1);
    g_ost_debug_initialized = 0;
    return 0;
}

int ost_debug_lock(void) {
    if (!g_ost_debug_initialized) return 0;
    OST_SPIN_LOCK_ELSE(&g_ost_debug_lock, return -1);
    return 0;
}

int ost_debug_unlock(void) {
    if (!g_ost_debug_initialized) return 0;
    OST_SPIN_UNLOCK_ELSE(&g_ost_debug_lock, return -1);
    return 0;
}

/*
 * Locks
 */
int ost_lock_init(OST_LOCK_T *l) {
#ifdef __KERNEL__
    //init_MUTEX(l);    
    sema_init(l, 1); 
#else
    if (pthread_mutex_init(l, NULL)!=0) {
        return -1;
    }
#endif
    return 0;
}

int ost_lock_ni(OST_LOCK_T *l) {
#ifdef __KERNEL__
    if (in_interrupt()) {
#if 0
        if (!atomic_read(&l->count)) {
            OST_TRACE_ERROR("semaphore will block within an interrupt!\n");
            return -1;
        }
#endif
    }
    down(l);
    return 0;
#else
    return ost_lock(l); 
#endif
}

int ost_lock(OST_LOCK_T *l) {
#ifdef __KERNEL__
    int attempt = 0;

    if (in_interrupt()) {
#if 0
        if (!atomic_read(&l->count)) {
            OST_TRACE_ERROR("semaphore will block within an interrupt!\n");
            return -1;
        }
#endif
    }
    while (down_interruptible(l)!=0) {
        OST_TRACE_ERROR("pid %i: semaphore not acquired (attempt %i)\n", 
                        OST_GETPID, attempt);
        OST_SLEEP(1);
        if (attempt > 10) {
            OST_TRACE_ERROR("giving up on semaphore after %i attempts\n", attempt);
            return -1;
        }
        attempt++;
    }

    return 0;
#else
    return pthread_mutex_lock(l);
#endif
}

int ost_unlock(OST_LOCK_T *l) {
#ifdef __KERNEL__
    up(l);
    return 0;
#else
    return pthread_mutex_unlock(l);
#endif
}

int ost_lock_destroy(OST_LOCK_T *l) {
#ifdef __KERNEL__
    return 0;
#else
    return pthread_mutex_destroy(l);
#endif
}

/*
 * Spinlocks
 */
int ost_spin_lock_init(OST_SPIN_LOCK_T *l) {
#ifdef __KERNEL__
    l->lock = SPIN_LOCK_UNLOCKED;
    return 0;
#else
    return ost_lock_init(l);
#endif
}

int ost_spin_lock(OST_SPIN_LOCK_T *l) {
#ifdef __KERNEL__
    spin_lock_irqsave(&l->lock, l->flags);
    return 0;
#else
    return ost_lock(l);
#endif
}

int ost_spin_unlock(OST_SPIN_LOCK_T *l) {
#ifdef __KERNEL__
    spin_unlock_irqrestore(&l->lock, l->flags);
    return 0;
#else
    return ost_unlock(l);
#endif
}

int ost_spin_lock_destroy(OST_SPIN_LOCK_T *l) {
#ifdef __KERNEL__
    return 0;
#else  
    return ost_lock_destroy(l);
#endif
}
