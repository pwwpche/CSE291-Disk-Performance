
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
 * Intel SCSI device driver for iSCSI
 */

#include <linux/module.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,12)
#include <linux/config.h>
#endif
#include <linux/types.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/kernel.h>

#define PRIu64 "llu"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,18)
#error "Kernels older than 2.4.18 are no longer supported"
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#error "Kernels version 2.5 is not supported"
#endif
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#ifdef CONFIG_OSD_BIDI
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,12)
#include <scsi/scsi_request.h>
#endif
#endif
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#include <linux/dma-mapping.h>
#else
#include <linux/blk.h>
#include <scsi.h>
#include <linux/in.h>
#include <hosts.h>
#include <sd.h>
#endif
#include <asm/uaccess.h>
#include "driver.h"
#include "debug.h"
#include "util.h"
#include "initiator.h"
#include "config.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
#define USE_SG(PTR) (scsi_sg_count(PTR))
#define REQUEST_BUFFLEN(PTR) (scsi_bufflen(PTR))
#define REQUEST_BUFF(PTR) (scsi_in(PTR))
#define REQUEST_SGLIST(PTR) (scsi_sglist(PTR))
#else
#define USE_SG(PTR) ((PTR)->use_sg)
#define REQUEST_BUFFLEN(PTR) ((PTR)->request_bufflen) 
#define REQUEST_BUFF(PTR) ((PTR)->request_buffer) 
#define REQUEST_SGLIST(PTR) (scsi_sglist(PTR))
#endif

/*
 * Local RAMDISK
 */
#if CONFIG_DRIVER_RAMDISK == 1
void *g_ramdisk_ptr = NULL;
uint64_t g_ramdisk_num_blocks = 0;
unsigned g_ramdisk_blocklen = 512;
#endif

/*
 * Function prototypes
 */

int iscsi_report_target(uint64_t tid);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static int iscsi_bios_param(struct scsi_device *, struct block_device *, 
                            sector_t, int *);
static int iscsi_proc_info (struct Scsi_Host *, char *, char **, off_t, 
                            int, int);
static int iscsi_slave_configure(struct scsi_device *dev);
#define SCSI_HOST_TEMPLATE struct scsi_host_template
#else
static int iscsi_bios_param(Disk *, kdev_t, int *);
static int iscsi_proc_info (char *, char **, off_t, int, int, int);
static int iscsi_command(struct scsi_cmnd *SCpnt);
static void iscsi_select_queue_depths(struct Scsi_Host *, struct scsi_device *);
#define SCSI_HOST_TEMPLATE Scsi_Host_Template
#endif

static int iscsi_detect(SCSI_HOST_TEMPLATE *);
static int iscsi_release(struct Scsi_Host *);
static int iscsi_ioctl(struct scsi_device *dev, int cmd, void *arg);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
static int iscsi_queuecommand_lck(struct scsi_cmnd *, 
                                  void (*done)(struct scsi_cmnd *));
static DEF_SCSI_QCMD(iscsi_queuecommand);
#else
static int iscsi_queuecommand(struct scsi_cmnd *, 
                              void (*done)(struct scsi_cmnd *));
#endif
static int iscsi_abort_handler (struct scsi_cmnd *SCpnt);
static int iscsi_device_reset_handler (struct scsi_cmnd *);
static int iscsi_bus_reset_handler (struct scsi_cmnd *);
static int iscsi_host_reset_handler(struct scsi_cmnd *);

static int iscsi_driver_init(void);
static int iscsi_driver_shutdown(void);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#define REQUEST_LOCK host->host_lock
#define TO_DEVICE DMA_TO_DEVICE
#define FROM_DEVICE DMA_FROM_DEVICE
#define CHANNEL(PTR) PTR->device->channel
#define TID(PTR) PTR->device->id
#define LUN(PTR) PTR->device->lun
#else
#define REQUEST_LOCK &io_request_lock
#define TO_DEVICE SCSI_DATA_WRITE
#define FROM_DEVICE SCSI_DATA_READ
#define CHANNEL(PTR) PTR->channel
#define TID(PTR) PTR->target
#define LUN(PTR) PTR->lun
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static struct scsi_host_template driver_template = {
    .module                 = THIS_MODULE,
    .proc_name              = "iscsi",
    .name                   = PACKAGE_STRING,
    .queuecommand           = iscsi_queuecommand,
    .slave_configure        = iscsi_slave_configure,
    .ioctl                  = iscsi_ioctl,
    .bios_param             = iscsi_bios_param,
    .proc_info              = iscsi_proc_info,
    .can_queue              = CONFIG_INITIATOR_QUEUE_DEPTH,
    .this_id                = -1,
    .sg_tablesize           = SG_ALL,
    .max_sectors            = SCSI_DEFAULT_MAX_SECTORS,
    .cmd_per_lun            = 1,
#if 0
    .use_clustering         = ENABLE_CLUSTERING,
    .unchecked_isa_dma      = 1,
#endif
    .eh_abort_handler       = iscsi_abort_handler,
    .eh_device_reset_handler= iscsi_device_reset_handler,
    .eh_bus_reset_handler   = iscsi_bus_reset_handler,
    .eh_host_reset_handler  = iscsi_host_reset_handler,
};

/*
 * More globals
 */

static int __init iscsi_init(void) {
    if (iscsi_malloc_atomic_init()!=0) {
        TRACE_ERROR("iscsi_malloc_atomic_init() failed\n");
        return -1;
    }
    if (iscsi_detect(&driver_template)==0) {
        TRACE_ERROR("iscsi_detect() failed\n");
        return -1;
    }
    return 0;
}
static void __exit iscsi_exit(void) {
    iscsi_release(NULL);
    iscsi_malloc_atomic_shutdown();
}
module_init(iscsi_init);
module_exit(iscsi_exit);
#else
static Scsi_Host_Template driver_template = {
    .proc_name              = "iscsi",
    .name                   = PACKAGE_STRING,
    .detect                 = iscsi_detect,
    .release                = iscsi_release,
    .queuecommand           = iscsi_queuecommand,
    .ioctl                  = iscsi_ioctl,
    .command                = iscsi_command,
    .select_queue_depths    = iscsi_select_queue_depths,
    .bios_param             = iscsi_bios_param,
    .proc_info              = iscsi_proc_info,
    .can_queue              = CONFIG_INITIATOR_QUEUE_DEPTH,
    .this_id                = -1,
    .sg_tablesize           = SG_ALL,
    .cmd_per_lun            = 1,
    .use_clustering         = ENABLE_CLUSTERING,
    .max_sectors            = 512,
    .use_new_eh_code        = 1,
    .eh_abort_handler       = iscsi_abort_handler,
    .eh_device_reset_handler= iscsi_device_reset_handler,
    .eh_bus_reset_handler   = iscsi_bus_reset_handler,
    .eh_host_reset_handler  = iscsi_host_reset_handler,
    .unchecked_isa_dma      = 0,
};
#include "scsi_module.c"
#endif

MODULE_AUTHOR("Intel Corporation, <http://www.intel.com>");
MODULE_LICENSE("Dual BSD/GPL");

struct trace_record {
    struct timeval arrival;
    struct timeval completion;
    uint64_t offset;
    uint32_t len;
    int write;
    uint64_t tid;
    uint64_t lun;
    struct trace_record *next;
};

/*
 * Macros
 */

#define DIFF_USEC(T1,T2)                        \
    ((T2.tv_sec*1000000 + T2.tv_usec)-          \
     (T1.tv_sec*1000000 + T1.tv_usec))
#define DIFF_MSEC(T1,T2) (DIFF_USEC(T1,T2)/1000)                      
#define DIFF_SEC(T1,T2) (DIFF_MSEC(T1,T2)/1000)                      

#define UPDATE_AVG_RESPONSE(AVG,AVG_COUNT,WINDOW,TIME)  \
    if (AVG_COUNT<(WINDOW-1)) {                         \
        AVG = (AVG*AVG_COUNT+TIME)/(AVG_COUNT+1);       \
        AVG_COUNT++;                                    \
    } else {                                            \
        AVG = (AVG*(WINDOW-1)+TIME)/WINDOW;             \
        AVG_COUNT = WINDOW;                             \
    }

/*
 * Globals
 */

static INITIATOR_CMD_T *g_cmd;
static ISCSI_QUEUE_T g_cmd_q;
static struct iovec **g_iov;
static ISCSI_QUEUE_T g_iovec_q;
static int g_driver_num_targets = 0;
static ISCSI_SPIN_LOCK_T 
g_lock[CONFIG_INITIATOR_MAX_TARGETS][CONFIG_DRIVER_MAX_LUNS];
static ISCSI_LOCK_T g_proc_lock;
static ISCSI_DRIVER_STATS_T 
g_stats[CONFIG_INITIATOR_MAX_TARGETS][CONFIG_DRIVER_MAX_LUNS];
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static char *config = IPS_CONFIG_FILE; MODULE_PARM(config, "s");
static int verbose = 0; MODULE_PARM(verbose, "i");
static int family = AF_INET; MODULE_PARM(family, "i");
#else
static char *config = IPS_CONFIG_FILE; module_param(config, charp, 444);
static int verbose = 0; module_param(verbose, int, 444);
static int family = AF_INET; module_param(family, int, 444);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static int trace = 0; MODULE_PARM(trace, "i");
#else
static int trace = 0; module_param(trace, int, 444);
#endif
static ISCSI_SPIN_LOCK_T g_trace_lock;
struct trace_record *trace_head = NULL;
struct trace_record *trace_tail = NULL;

struct Scsi_Host *g_ptr;

/* 
 * Internal functions
 */

#define DI_ERROR {                                                      \
        for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {                \
            if (g_cmd[i].ptr != NULL) iscsi_free_atomic(g_cmd[i].ptr);  \
        }                                                               \
        for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {                \
            if (g_iov[i] != NULL) iscsi_free_atomic(g_iov[i]);          \
        }                                                               \
        iscsi_free_atomic(g_iov);                                       \
        iscsi_free_atomic(g_cmd);                                       \
        return -1;                                                      \
    }

/*
 * Statistics routines (stats maintained for each lun)
 */

static int stats_reset(void) {
    int i, j;

    for (i=0; i<CONFIG_INITIATOR_MAX_TARGETS; i++) {
        for (j=0; j<CONFIG_DRIVER_MAX_LUNS; j++) {
            unsigned outstanding = g_stats[i][j].outstanding;
            unsigned outstanding_wr = g_stats[i][j].outstanding_wr;
            unsigned outstanding_rd = g_stats[i][j].outstanding_rd;
            unsigned outstanding_bidi = g_stats[i][j].outstanding_bidi;
            unsigned outstanding_other = g_stats[i][j].outstanding_other;
            unsigned q_bytes_tx = g_stats[i][j].q_bytes_tx;
            unsigned q_bytes_rx = g_stats[i][j].q_bytes_tx;

            ISCSI_SPIN_LOCK_ELSE(&g_lock[i][j], return -1);
            memset(&g_stats[i][j], 0, sizeof(ISCSI_DRIVER_STATS_T));
            g_stats[i][j].min_response = 0xffffffff;
#if 0
            g_stats[i][j].min_lba_a = 0xffffffff;
            g_stats[i][j].min_lba_b = 0xffffffff;
#endif
            g_stats[i][j].outstanding = outstanding;
            g_stats[i][j].outstanding_wr = outstanding_wr;
            g_stats[i][j].outstanding_rd = outstanding_rd;
            g_stats[i][j].outstanding_bidi = outstanding_bidi;
            g_stats[i][j].outstanding_other = outstanding_other;
            g_stats[i][j].q_bytes_tx =  q_bytes_tx;
            g_stats[i][j].q_bytes_rx =  q_bytes_rx;
            ISCSI_SPIN_UNLOCK_ELSE(&g_lock[i][j], return -1);
        }
    }
    return 0;
}

static int stats_init(void) {
    int i, j;

    for (i=0; i<CONFIG_INITIATOR_MAX_TARGETS; i++) {
        for (j=0; j<CONFIG_DRIVER_MAX_LUNS; j++) {
            ISCSI_SPIN_LOCK_INIT_ELSE(&g_lock[i][j], return -1);
        }
    }
    return stats_reset();
}

static int stats_destroy(void) {
    int i, j;

    for (i=0; i<CONFIG_INITIATOR_MAX_TARGETS; i++) {
        for (j=0; j<CONFIG_DRIVER_MAX_LUNS; j++) {
            ISCSI_SPIN_LOCK_DESTROY_ELSE(&g_lock[i][j], return -1);
        }
    }
    return 0;
}

static int iscsi_driver_init(void) {
    int i;

    if ((g_cmd=iscsi_malloc_atomic(sizeof(INITIATOR_CMD_T)*
                                   CONFIG_INITIATOR_QUEUE_DEPTH))==NULL) {
        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        return -1;
    }
    if ((g_iov=iscsi_malloc_atomic(sizeof(struct iovec*)*
                                   CONFIG_INITIATOR_QUEUE_DEPTH))==NULL) {
        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        iscsi_free_atomic(g_cmd);
        return -1;
    }
    for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {
        g_iov[i] = NULL;
        g_cmd[i].ptr = NULL;
    }
    for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {
        if ((g_iov[i]=iscsi_malloc_atomic(sizeof(struct iovec)*SG_ALL))==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
            DI_ERROR;
        }
    }
    QUEUE_INIT_ELSE(&g_cmd_q, CONFIG_INITIATOR_QUEUE_DEPTH, DI_ERROR);
    QUEUE_INIT_ELSE(&g_cmd_q, CONFIG_INITIATOR_QUEUE_DEPTH, DI_ERROR);
    QUEUE_INIT_ELSE(&g_iovec_q, CONFIG_INITIATOR_QUEUE_DEPTH, DI_ERROR);
    for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {
        if ((g_cmd[i].ptr = 
             iscsi_malloc_atomic(sizeof(ISCSI_SCSI_CMD_T)))==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
            DI_ERROR;
        }
        g_cmd[i].type = ISCSI_SCSI_CMD;
        QUEUE_INSERT_ELSE(&g_cmd_q, &g_cmd[i], DI_ERROR);
        QUEUE_INSERT_ELSE(&g_iovec_q, g_iov[i], DI_ERROR);
    }
    PRINT("iscsi: target configuration file is \"%s\"\n", config);
    if ((g_driver_num_targets=initiator_init(config, verbose, family))<0) {
        TRACE_ERROR("initiator_init() failed\n");
        DI_ERROR;
    }

#if CONFIG_DRIVER_RAMDISK == 1
    if (g_ramdisk_num_blocks) {
	g_driver_num_targets++;
	if (iscsi_report_target(g_driver_num_targets-1)!=0) {
	    PRINT("iscsi_report_target() failed\n");
	}
    }
#endif

    if (!g_driver_num_targets && !g_ramdisk_num_blocks) {
        PRINT("no targets - shutting down\n");
        if (initiator_shutdown()!=0) {
            TRACE_ERROR("initiator_shutdown() failed\n");
            return -1;
        }
    }
    if (stats_init()!=0) {
        TRACE_ERROR("stats_init() failed\n");
        return -1;
    }
    ISCSI_LOCK_INIT_ELSE(&g_proc_lock, return -1);

    if (trace) {
        ISCSI_SPIN_LOCK_INIT_ELSE(&g_trace_lock, return -1);
        PRINT("\nI/O Tracing has been enabled. To get the traces:\n");
        PRINT("  1. echo \"dump-trace\" > /proc/scsi/iscsi/<host num>\n");
        PRINT("  2. cat /proc/scsi/iscsi/<host num> > tracefile\n\n");
    }

    return 0;
}

static int iscsi_driver_shutdown(void) {
    int i=0;

    if (trace) {
        struct trace_record *ptr, *tmp;

        /* free trace records */
        ptr = trace_head;
        while (ptr) {
            tmp = ptr;
            ptr = ptr->next;
            iscsi_free_atomic(tmp);
            i++;
        }
        ISCSI_SPIN_LOCK_DESTROY_ELSE(&g_trace_lock, return -1);
    }

    ISCSI_LOCK_DESTROY_ELSE(&g_proc_lock, return -1);
    stats_destroy();
    if (initiator_shutdown()!=0) {
        TRACE_ERROR("initiator_shutdown() failed\n");
        return -1;
    }
    while (iscsi_queue_remove(&g_iovec_q)) {}
    iscsi_queue_destroy(&g_iovec_q);
    while (iscsi_queue_remove(&g_cmd_q)) {}
    iscsi_queue_destroy(&g_cmd_q);
    for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {
        iscsi_free_atomic(g_iov[i]);
        iscsi_free_atomic(g_cmd[i].ptr);
    }
    iscsi_free_atomic(g_cmd);
    iscsi_free_atomic(g_iov);

#if CONFIG_DRIVER_RAMDISK == 1
    if (g_ramdisk_num_blocks) {
	vfree(g_ramdisk_ptr);
    }
#endif

    return 0;
}

/*
 * Public
 */

int iscsi_report_target(uint64_t tid) {
    HAND fh = HAND_INIT;
    struct Scsi_Host *ptr = g_ptr;
    char buffer[1024];
    mm_segment_t oldfs;
    uint64_t lun = 0;

    ptr->max_id = g_driver_num_targets;

    if ((fh = OPEN("/proc/scsi/scsi", O_WRONLY, 0))==NULL) {
        TRACE_ERROR("failed to open /proc/scsi/scsi\n");
        return -1;
    }

    for (lun=0; lun<1; lun++) {
        sprintf(buffer, "scsi add-single-device %i 0 %llu %llu\n", 
                ptr->host_no, tid, lun);
        oldfs= get_fs ();
        set_fs (KERNEL_DS);
        fh->f_op->write (fh, buffer, strlen(buffer), &fh->f_pos);
        set_fs (oldfs);
    } 
    CLOSE(fh, NULL);

    return 0;
}

static int iscsi_detect(SCSI_HOST_TEMPLATE *tptr) {
    struct Scsi_Host *host;

    DEBUG_INIT;

    if (!strlen(config)) {
        PRINT("*** usage: insmod intel_iscsi.o ***\n");
        return 0;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    spin_unlock(REQUEST_LOCK);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    g_ptr = host = scsi_host_alloc(tptr, 0);
    if (!host) {
        TRACE_ERROR("scsi_host_alloc() failed\n");
        return 0;
    }
    if (scsi_add_host(host, NULL)) {
        TRACE_ERROR("scsi_add_host() failed\n");
        return 0;
    }    
#else
    g_ptr = host = scsi_register(tptr, 0);
#endif

    if (iscsi_driver_init()!=0) {
        TRACE_ERROR("iscsi_driver_init() failed\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        spin_lock(REQUEST_LOCK);
#endif
        return 0; 
    }

#if (CONFIG_DRIVER_PROBE)
    host->max_id = g_driver_num_targets;
#else
    host->max_id = 0;
#endif

    host->max_channel = 0;
    host->max_lun = CONFIG_DRIVER_MAX_LUNS;
    host->max_cmd_len = 255;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    spin_lock(REQUEST_LOCK);
#endif

#if 0
    host->unique_id = host->host_no;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    //scan_scsi_host(host);
#endif

    return 1; 
}

int iscsi_release(struct Scsi_Host *host) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    scsi_remove_host(g_ptr);
    scsi_host_put(g_ptr);
#endif
    iscsi_driver_shutdown();
    DEBUG_SHUTDOWN;
    PRINT("iscsi: host released\n");
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
int iscsi_command(struct scsi_cmnd *SCpnt) {
    TRACE(TRACE_DEBUG, 0, 
          "0x%p: op 0x%x, chan %i, target %i, lun %i, bufflen %i, sg %i\n", 
          SCpnt, SCpnt->cmnd[0], CHANNEL(SCpnt), TID(SCpnt), LUN(SCpnt), 
          REQUEST_BUFFLEN(SCpnt), USE_SG(SCpnt));
    TRACE_ERROR("NOT IMPLEMENTED\n");
    return -1;
}
#endif

int iscsi_done(void *ptr) {
    INITIATOR_CMD_T *cmd = (INITIATOR_CMD_T *) ptr;
    ISCSI_SCSI_CMD_T *scsi_cmd = (ISCSI_SCSI_CMD_T *) cmd->ptr;
    struct scsi_cmnd *SCpnt = (struct scsi_cmnd *) cmd->callback_arg;

    if (!SCpnt) return -1;
    SCpnt->result = !cmd->status?scsi_cmd->status:2;

    if (SCpnt->result != 0) {
        PRINT("iscsi: tid %i lun %i op 0x%x ** result %i ** aborted %i "
              "(iscsi tag 0x%x)\n", SCpnt->device->id, SCpnt->device->lun, 
              SCpnt->cmnd[0], SCpnt->result, cmd->aborted, cmd->tag);
    } else {
        TRACE_CLEAN(TRACE_SCSI, 1, "iscsi_done (pid %i): serial %lu tid "
                    "%i lun %i op 0x%x result %i aborted %i (iscsi tag "
                    "0x%x)\n", OST_GETPID, SCpnt->serial_number, TID(SCpnt), 
                    LUN(SCpnt), SCpnt->cmnd[0], SCpnt->result, cmd->aborted, 
                    cmd->tag);
    }

    /* update stats */
    ISCSI_SPIN_LOCK_ELSE(&g_lock[TID(SCpnt)][LUN(SCpnt)], return -1);
    do_gettimeofday(&scsi_cmd->stop_time);
    if  (DIFF_USEC(scsi_cmd->start_time, scsi_cmd->stop_time) > 
         g_stats[TID(SCpnt)][LUN(SCpnt)].max_response)
        g_stats[TID(SCpnt)][LUN(SCpnt)].max_response = 
            DIFF_USEC(scsi_cmd->start_time, scsi_cmd->stop_time);
    if  (DIFF_USEC(scsi_cmd->start_time, scsi_cmd->stop_time) < 
         g_stats[TID(SCpnt)][LUN(SCpnt)].min_response)
        g_stats[TID(SCpnt)][LUN(SCpnt)].min_response = 
            DIFF_USEC(scsi_cmd->start_time, scsi_cmd->stop_time);
    g_stats[TID(SCpnt)][LUN(SCpnt)].latencies += 
        DIFF_USEC(scsi_cmd->start_time, scsi_cmd->stop_time);
   
#if 0 
    UPDATE_AVG_RESPONSE(g_stats[TID(SCpnt)][LUN(SCpnt)].avg_response_10, 
                        g_stats[TID(SCpnt)][LUN(SCpnt)].avg_response_10_count, 
                        10,DIFF_USEC(scsi_cmd->start_time, 
                                     scsi_cmd->stop_time));
    UPDATE_AVG_RESPONSE(g_stats[TID(SCpnt)][LUN(SCpnt)].avg_response_100, 
                        g_stats[TID(SCpnt)][LUN(SCpnt)].avg_response_100_count, 
                        100,DIFF_USEC(scsi_cmd->start_time, 
                                      scsi_cmd->stop_time));
    UPDATE_AVG_RESPONSE(g_stats[TID(SCpnt)][LUN(SCpnt)].avg_response_1000, 
                        g_stats[TID(SCpnt)][LUN(SCpnt)].avg_response_1000_count,
                        1000,DIFF_USEC(scsi_cmd->start_time, 
                                       scsi_cmd->stop_time));
#endif

    g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding--;

    if (!cmd->aborted) {
        if (SCpnt->result!=0) g_stats[TID(SCpnt)][LUN(SCpnt)].failed++;
        else g_stats[TID(SCpnt)][LUN(SCpnt)].succeeded++;
        if (scsi_cmd->todev) {
            g_stats[TID(SCpnt)][LUN(SCpnt)].bytes_tx += REQUEST_BUFFLEN(SCpnt);
            g_stats[TID(SCpnt)][LUN(SCpnt)].q_bytes_tx -= 
                REQUEST_BUFFLEN(SCpnt);
            g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_wr--;
            g_stats[TID(SCpnt)][LUN(SCpnt)].latencies_wr += 
                DIFF_USEC(scsi_cmd->start_time, scsi_cmd->stop_time);
        }
        if ((scsi_cmd->fromdev)&&(scsi_cmd->todev)) {
#ifdef CONFIG_OSD_BIDI
            g_stats[TID(SCpnt)][LUN(SCpnt)].bytes_rx += 
                SCpnt->bidi_request_bufflen;
            g_stats[TID(SCpnt)][LUN(SCpnt)].q_bytes_rx -= 
                SCpnt->bidi_request_bufflen;
            g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_bidi--;
            g_stats[TID(SCpnt)][LUN(SCpnt)].latencies_bidi += 
                DIFF_USEC(scsi_cmd->start_time, scsi_cmd->stop_time);
#endif
        } else if (scsi_cmd->fromdev) {
            g_stats[TID(SCpnt)][LUN(SCpnt)].bytes_rx += REQUEST_BUFFLEN(SCpnt);
            g_stats[TID(SCpnt)][LUN(SCpnt)].q_bytes_rx -=
                REQUEST_BUFFLEN(SCpnt);
            g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_rd--;
            g_stats[TID(SCpnt)][LUN(SCpnt)].latencies_rd += 
                DIFF_USEC(scsi_cmd->start_time, scsi_cmd->stop_time);
        } else {
            g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_other--;
            g_stats[TID(SCpnt)][LUN(SCpnt)].latencies_other += 
                DIFF_USEC(scsi_cmd->start_time, scsi_cmd->stop_time);
        }
    }  
    ISCSI_SPIN_UNLOCK_ELSE(&g_lock[TID(SCpnt)][LUN(SCpnt)], return -1);
    if (SCpnt->host_scribble) {
        unsigned char *hs;
        hs = SCpnt->host_scribble;
        SCpnt->host_scribble = NULL; // for abort
        QUEUE_INSERT_ELSE(&g_iovec_q, hs, return -1);
    }
    QUEUE_INSERT_ELSE(&g_cmd_q, cmd, return -1);
    cmd->callback_arg = NULL;  // needed for abort
    SCpnt->scsi_done(SCpnt);
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
int iscsi_queuecommand_lck(struct scsi_cmnd *SCpnt, 
                           void (*done)(struct scsi_cmnd *)) {
#else
int iscsi_queuecommand(struct scsi_cmnd *SCpnt, 
                       void (*done)(struct scsi_cmnd *)) {
#endif
    INITIATOR_CMD_T *cmd;
    ISCSI_SCSI_CMD_T *scsi_cmd;
    unsigned char *data;
    unsigned length, trans_len, bidi_trans_len;
    int input, output;
    unsigned char *ahs_ptr;
    struct Scsi_Host *host = SCpnt->device->host;
    struct trace_record *record = NULL;

#if CONFIG_DRIVER_RAMDISK == 1
    if (TID(SCpnt) == (g_driver_num_targets-1) && g_ramdisk_num_blocks) {
	SCpnt->result = 1;
	switch (SCpnt->cmnd[0]) {
	case TEST_UNIT_READY:
	    //PRINT("got op TEST_UNIT_READY for ramdisk\n", SCpnt->cmnd[0]);
	    //PRINT("iscsi_queuecommand: got TEST_UNIT_READY for ramdisk (op 0x%x)\n", SCpnt->cmnd[0]);
	    SCpnt->result = 0;
	    break;
	case INQUIRY:
	    //PRINT("got op 0x%x (INQUIRY)for ramdisk (SG count %i, bufflen %u)\n", 
	    //	  SCpnt->cmnd[0], scsi_sg_count(SCpnt), REQUEST_BUFFLEN(SCpnt));
	    if ((USE_SG(SCpnt)>=1)&&(SCpnt->cmnd[4]>=36)) {
		struct scatterlist *sg = (struct scatterlist *) REQUEST_SGLIST(SCpnt);
		int i = 0;
		char *data = NULL;
		int len;
		
		scsi_for_each_sg(SCpnt, sg, scsi_sg_count(SCpnt), i) {
		    data = page_address(sg_page(sg)) + sg->offset;
		    len = sg->length;
		    if (len<36) {
			//PRINT("short INQUIRY buffer (expected at least %i, got %i)\n", 36, len);
			goto ramdisk_callback;
		    } 
		}
		memset(data, 0, SCpnt->cmnd[4]);                // Clear allocated buffer
		data[0] = 0;                                    // Peripheral Device Type
		data[2] |= 0x02;                                // ANSI-approved version
		data[4] = SCpnt->cmnd[4]-4;                     // Additional length
		data[7] |= 0x40;                                // WBus32 
		data[7] |= 0x20;                                // WBus16
		data[7] |= 0x02;                                // Tagged Command Queueing
		strcpy((char *)(data+8), "Intel   " );          // Vendor
		strcpy((char *)(data+16), "SCSI RAMDISK    " ); // Product ID
		sprintf((char *)(data+32), "%s", "XXXX");       // Product Revision
		SCpnt->result = 0;
	    } else { 
		TRACE_ERROR("error processing ramdisk INQUIRY command (USE_SG=%i, cmnd[4]=%i)\n",
			    USE_SG(SCpnt), SCpnt->cmnd[4]);
		SCpnt->result = 1;
	    }
	    break;
	case READ_CAPACITY:
	    if (USE_SG(SCpnt)==1) {
		struct scatterlist *sg = (struct scatterlist *) REQUEST_SGLIST(SCpnt);
		int i = 0;
		char *data = NULL;
		int len;
		
		scsi_for_each_sg(SCpnt, sg, scsi_sg_count(SCpnt), i) {
		    data = page_address(sg_page(sg)) + sg->offset;
		    len = sg->length;
		    if (len!=8) {
			TRACE_ERROR("short READ_CAPACITY buffer (expected 8 got %i)\n", len);
			goto ramdisk_callback;
		    }
		}
		*((unsigned *) data) = HTONL(g_ramdisk_num_blocks-1);  // Max LBA
		*((unsigned *) (data+4)) = HTONL(g_ramdisk_blocklen);  // Block len
		SCpnt->result = 0;
	    } else {
		TRACE_ERROR("error processing ramdisk READ_CAPACITY command\n");
		SCpnt->result = 1;
	    }
	    break;
	case WRITE_10:
	    if (USE_SG(SCpnt)) {
		struct scatterlist *sg = (struct scatterlist *) REQUEST_SGLIST(SCpnt);
		int i = 0;
		void *data = NULL;
                uint64_t lba = NTOHL(*(uint32_t*)(SCpnt->cmnd+2));
                //uint64_t len = NTOHS(*(uint16_t *)(SCpnt->cmnd+7));
		unsigned offset = 0;
		
		//PRINT("RAMDISK WRITE_10(lba %u len %u)\n", lba, len);
		scsi_for_each_sg(SCpnt, sg, scsi_sg_count(SCpnt), i) {
		    data = page_address(sg_page(sg)) + sg->offset;
		    //PRINT("write_10: page %p offset %u length %u (ram ptr %p)\n", 
		    //	  page_address(sg_page(sg)), sg->offset, sg->length,
		    //	  g_ramdisk_ptr+lba*g_ramdisk_blocklen);
		    memcpy(g_ramdisk_ptr+lba*g_ramdisk_blocklen+offset, data, sg->length);
		    offset+=sg->length;
		}
		SCpnt->result = 0;
	    } else {
		TRACE_ERROR("error processing ramdisk WRITE_10 command\n");
		SCpnt->result = 1;
	    }
	    break;
	case READ_10:
	    if (USE_SG(SCpnt)) {
		struct scatterlist *sg = (struct scatterlist *) REQUEST_SGLIST(SCpnt);
		int i = 0;
		void *data = NULL;
                uint64_t lba = NTOHL(*(uint32_t*)(SCpnt->cmnd+2));
                //uint64_t len = NTOHS(*(uint16_t *)(SCpnt->cmnd+7));
		unsigned offset = 0;
		
		//PRINT("RAMDISK READ_10(lba %u len %u)\n", lba, len);		
		scsi_for_each_sg(SCpnt, sg, scsi_sg_count(SCpnt), i) {
		    data = page_address(sg_page(sg)) + sg->offset;
		    //PRINT("read_10: page %p offset %u length %u (ram ptr %p)\n", 
		    //	  page_address(sg_page(sg)), sg->offset, sg->length,
		    //	  g_ramdisk_ptr+lba*g_ramdisk_blocklen);
		    memcpy(data, g_ramdisk_ptr+lba*g_ramdisk_blocklen+offset, sg->length);
		    offset += sg->length;
		}
		SCpnt->result = 0;
	    } else {
		TRACE_ERROR("error processing ramdisk READ_10 command\n");
		SCpnt->result = 1;
	    }
	    SCpnt->result = 0;
	    break;
	case VERIFY:
	    SCpnt->result = 0;
	    break;
	case MODE_SENSE:
	    SCpnt->result = 0;
	    break;
	case MODE_SENSE_10:
	    SCpnt->result = 0;
	    break;
	default:
	    TRACE_ERROR("opcode 0x%x not implemented for ramdisk\n", SCpnt->cmnd[0]);
	    SCpnt->result = 1;
	}
	/* callback the midlayer and treure*/
    ramdisk_callback: 
	done(SCpnt);
	return 0;
    }
#endif
    
#if 0
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32)
    TRACE_ERROR("scsi_sg_count is %i\n", scsi_sg_count(SCpnt));
    TRACE_ERROR("scsi_sglist is %p\n", scsi_sglist(SCpnt));
    TRACE_ERROR("host is %p\n", host);
    TRACE_ERROR("dma_dev is %p\n", host->dma_dev); /* not in 2.6.24 */
    TRACE_ERROR("USE_SG is %i\n", USE_SG(SCpnt));
#endif
#endif

#if 0    
    /* Tagged command queuing is handled from within this SCSI driver. */
    if ((SCpnt->device->tagged_supported)&&(SCpnt->device->tagged_queue)) {
        SCpnt->tag = SCpnt->device->current_tag++;        
    }
#endif    

    spin_unlock(REQUEST_LOCK);

    trans_len = bidi_trans_len = length = output = input = 0;

    /* update stats */
    ISCSI_SPIN_LOCK_ELSE(&g_lock[TID(SCpnt)][LUN(SCpnt)], return -1);
    g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding++;
    g_stats[TID(SCpnt)][LUN(SCpnt)].depths += 
        g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding;

    if ((SCpnt->cmnd[0]!=TEST_UNIT_READY)&&(REQUEST_BUFFLEN(SCpnt))) {
        uint32_t lba = 0;
        uint32_t len = 0;

        if (trace) {
            printk("WE SHOULD NOT BE TRACING\n");
            if ((record=
                 iscsi_malloc_atomic(sizeof(struct trace_record)))==NULL) {
                TRACE_ERROR("error allocating trace record\n");
            }
            memset(record, 0, sizeof(struct trace_record));
            ISCSI_SPIN_LOCK_ELSE(&g_trace_lock, return -1);
            if (trace_head==NULL) {
                trace_head = trace_tail = record;
            } else {
                trace_tail->next = record;
                trace_tail = record;
            }
            ISCSI_SPIN_UNLOCK_ELSE(&g_trace_lock, return -1);
        }

        if (SCpnt->sc_data_direction == TO_DEVICE) {
            output = 1; input = 0;
            length = trans_len = REQUEST_BUFFLEN(SCpnt);
            g_stats[TID(SCpnt)][LUN(SCpnt)].q_bytes_tx += trans_len;
            g_stats[TID(SCpnt)][LUN(SCpnt)].ops_wr++;
            g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_wr++;
            if (SCpnt->cmnd[0]==WRITE_6) {
                lba = NTOHL(*((unsigned *)SCpnt->cmnd))&0x001fffff;
                len = SCpnt->cmnd[4];
#if 0
                printk(KERN_ERR "WRITE_6(lba %u len %u tag %u)\n", 
                       lba, len, SCpnt->request->bio->bi_tag);
#endif
            } else if (SCpnt->cmnd[0]==WRITE_10) {
                lba = NTOHL(*(uint32_t*)(SCpnt->cmnd+2));
                len = NTOHS(*(uint16_t *)(SCpnt->cmnd+7));
#if 0
                printk(KERN_ERR "WRITE_10(lba %u len %u tag %u)\n", 
                       lba, len, SCpnt->request->bio->bi_tag);
                SCpnt->cmnd[6] = SCpnt->request->bio->bi_tag;
#endif
            }

            if (trace) {
                do_gettimeofday(&record->arrival); 
                record->tid=TID(SCpnt); 
                record->lun = LUN(SCpnt);
                record->offset = lba; 
                record->len = len; 
                record->write = 1;
            }

            g_stats[TID(SCpnt)][LUN(SCpnt)].write_jumps += 
                abs(lba-g_stats[TID(SCpnt)][LUN(SCpnt)].last_wr_lba);
            g_stats[TID(SCpnt)][LUN(SCpnt)].jumps += 
                abs(lba-g_stats[TID(SCpnt)][LUN(SCpnt)].last_lba);
            g_stats[TID(SCpnt)][LUN(SCpnt)].last_lba = 
                g_stats[TID(SCpnt)][LUN(SCpnt)].last_wr_lba = lba+len;
            g_stats[TID(SCpnt)][LUN(SCpnt)].depths_wr += 
                g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_wr;
        } else if (SCpnt->sc_data_direction == FROM_DEVICE) {
            length = output = 0; input = 1;
            trans_len = REQUEST_BUFFLEN(SCpnt);
            g_stats[TID(SCpnt)][LUN(SCpnt)].q_bytes_rx += trans_len;
            g_stats[TID(SCpnt)][LUN(SCpnt)].ops_rd++;
            g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_rd++;
            if (SCpnt->cmnd[0]==READ_6) {
                lba = NTOHL(*((unsigned *)SCpnt->cmnd))&0x001fffff;
                len = SCpnt->cmnd[4];
#if 0
                printk(KERN_ERR "READ_6(lba %u len %u tag %u)\n", 
                       lba, len, SCpnt->request->bio->bi_tag);
#endif
            } else if (SCpnt->cmnd[0]==READ_10) {
                lba = NTOHL(*(uint32_t *)(SCpnt->cmnd+2));
                len = NTOHS(*(uint16_t *)(SCpnt->cmnd+7));
#if 0
                printk(KERN_ERR "READ_10(lba %u len %u tag %u)\n", 
                       lba, len, SCpnt->request->bio->bi_tag);
                SCpnt->cmnd[6] = SCpnt->request->bio->bi_tag;
#endif
            }

            if (trace) {
                do_gettimeofday(&record->arrival); 
                record->tid=TID(SCpnt); 
                record->lun = LUN(SCpnt);
                record->offset = lba; 
                record->len = len; 
                record->write = 0;
            }

            g_stats[TID(SCpnt)][LUN(SCpnt)].read_jumps += 
                abs(lba-g_stats[TID(SCpnt)][LUN(SCpnt)].last_rd_lba);
            g_stats[TID(SCpnt)][LUN(SCpnt)].jumps += 
                abs(lba-g_stats[TID(SCpnt)][LUN(SCpnt)].last_lba);
            g_stats[TID(SCpnt)][LUN(SCpnt)].last_lba = 
                g_stats[TID(SCpnt)][LUN(SCpnt)].last_rd_lba = lba+len;
            g_stats[TID(SCpnt)][LUN(SCpnt)].depths_rd += 
                g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_rd;
        } else {
            input = output = 1;
            trans_len = REQUEST_BUFFLEN(SCpnt);
#ifdef CONFIG_OSD_BIDI
            bidi_trans_len = SCpnt->bidi_request_bufflen = 
                SCpnt->sc_request->sr_bidi_bufflen;
#endif
            g_stats[TID(SCpnt)][LUN(SCpnt)].q_bytes_tx += trans_len;
            g_stats[TID(SCpnt)][LUN(SCpnt)].q_bytes_rx += bidi_trans_len;
            g_stats[TID(SCpnt)][LUN(SCpnt)].ops_bidi++;
            g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_bidi++;
            g_stats[TID(SCpnt)][LUN(SCpnt)].depths_bidi += 
                g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_bidi;
        }
    } else {
        g_stats[TID(SCpnt)][LUN(SCpnt)].ops_other++;
        g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_other++;
        g_stats[TID(SCpnt)][LUN(SCpnt)].depths_other += 
            g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_other;
    } 
    ISCSI_SPIN_UNLOCK_ELSE(&g_lock[TID(SCpnt)][LUN(SCpnt)], return -1);

    /* Convert scatterlist to iovec (what tcp uses) */
    if (USE_SG(SCpnt)) {
        struct scatterlist *sg = (struct scatterlist *) REQUEST_SGLIST(SCpnt);
        struct iovec *iov;
        int i=0;

        QUEUE_REMOVE_ELSE(&g_iovec_q, iov, spin_lock(REQUEST_LOCK); return -1);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
        scsi_for_each_sg(SCpnt, sg, scsi_sg_count(SCpnt), i) {
            iov[i].iov_base = page_address(sg_page(sg)) + sg->offset;
            iov[i].iov_len = sg->length;
        }
#else
        for (i=0; i<USE_SG(SCpnt); i++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
            iov[i].iov_base = page_address(sg[i].page) + sg[i].offset;
#else
            iov[i].iov_base = sg[i].address;
#endif
            iov[i].iov_len = sg[i].length;
        }
#endif
        data = SCpnt->host_scribble = (unsigned char *)iov;        
    } else {
        data = (unsigned char *) REQUEST_BUFF(SCpnt);
        SCpnt->host_scribble = NULL;
    }

    /* Get and initialize cmd structure */
    QUEUE_REMOVE_ELSE(&g_cmd_q, cmd, spin_lock(REQUEST_LOCK); return -1);
    scsi_cmd = (ISCSI_SCSI_CMD_T *) cmd->ptr;
    memset(scsi_cmd, 0, sizeof(ISCSI_SCSI_CMD_T));
    scsi_cmd->send_data = output?data:0;
    scsi_cmd->send_sg_len = output?USE_SG(SCpnt):0;
    scsi_cmd->recv_data = input?data:NULL;
    scsi_cmd->recv_sg_len = input?USE_SG(SCpnt):0;
    scsi_cmd->fromdev = input;
    scsi_cmd->todev = output;
    scsi_cmd->length = length;
    scsi_cmd->lun = LUN(SCpnt);
    scsi_cmd->trans_len = trans_len;
    scsi_cmd->bidi_trans_len = bidi_trans_len;
    scsi_cmd->cdb = SCpnt->cmnd; 
    scsi_cmd->tag = cmd->tag;    
    do_gettimeofday(&scsi_cmd->start_time);
    ahs_ptr = scsi_cmd->ahs;

    /* AHS for expected bidi transfer length */
    if (input && output) {
        TRACE(TRACE_SCSI, 1, 
              "creating AHS for bidi transfer length (%u bytes)\n", 
              bidi_trans_len);
        memset(ahs_ptr, 0, 8);
        *((uint16_t *)ahs_ptr) = HTONS(8);                  // AHS length
        ahs_ptr[2] = 0x02;                                  // Type
        *((uint32_t *)(ahs_ptr+4)) = HTONL(bidi_trans_len); // Bidi trans length
        scsi_cmd->ahs_len += 8;
        ahs_ptr += 8;
    }

    /* AHS for CDBs larger than 16 bytes */
    if (SCpnt->cmd_len>16) {
        TRACE(TRACE_SCSI, 1, "creating AHS for extended CDB (%i bytes)\n", 
              SCpnt->cmd_len);
        if (SCpnt->cmd_len>CONFIG_ISCSI_MAX_AHS_LEN) {
            TRACE_ERROR("SCpnt->cmd_len (%u) > CONFIG_ISCSI_MAX_AHS_LEN (%u)\n",
                        SCpnt->cmd_len, CONFIG_ISCSI_MAX_AHS_LEN);
            spin_lock(REQUEST_LOCK);    
            return -1;
        }
        memset(ahs_ptr, 0, 4);
        *((uint16_t *)(ahs_ptr)) = HTONS(SCpnt->cmd_len-16); // AHS length
        ahs_ptr[2] = 0x01;                                   // Type
        memcpy(ahs_ptr+4, SCpnt->cmnd+16, SCpnt->cmd_len-16);// Copy in CDB
        scsi_cmd->ahs_len += SCpnt->cmd_len-16;
        ahs_ptr += SCpnt->cmd_len-16;
    }

    SCpnt->scsi_done = done;     // The midlayer's callback called in iscsi_done
    SCpnt->result = 0x02;        // Default to a check condition
    if (initiator_enqueue(ISCSI_SCSI_CMD, scsi_cmd,  TID(SCpnt), cmd, 
                          iscsi_done, SCpnt)!=0) {
        TRACE_ERROR("initiator_enqueue() failed\n");
        if (SCpnt->cmd_len>16) iscsi_free_atomic(scsi_cmd->ahs);
        spin_lock(REQUEST_LOCK);
        return -1;
    }
    SCpnt->tag = cmd->tag;
    spin_lock(REQUEST_LOCK);

    TRACE_CLEAN(TRACE_SCSI, 1, 
                "iscsi_qcmd (pid %i): serial %lu iscsi tag 0x%x tid %i lun %i "
                "op 0x%x len %i sg %i request_buffer %p\n",
                OST_GETPID, SCpnt->serial_number, cmd->tag, TID(SCpnt), 
                LUN(SCpnt), SCpnt->cmnd[0], REQUEST_BUFFLEN(SCpnt), 
                USE_SG(SCpnt), REQUEST_BUFF(SCpnt));

    return 0;
}

int iscsi_ioctl (struct scsi_device *dev, int cmd, void *argp) {      
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static void iscsi_select_queue_depths(struct Scsi_Host *host, 
                                      struct scsi_device *dev) {
    struct scsi_device *device;

    for (device = dev; device; device = device->next) {
        if (device->tagged_supported) {
            device->tagged_queue = 1;
            device->current_tag = 0;
            device->queue_depth = 64;
            /*PRINT("channel %i target %i lun %i queue depth set to %i\n",
              device->channel, device->id, device->lun, device->queue_depth);*/
        } else {
            PRINT("warn: channel %i target %i lun %i does NOT support TCQ?\n",
                  device->channel, device->id, device->lun);
            device->queue_depth = 1;
        }
    }
}
#else
static int iscsi_slave_configure(struct scsi_device *device) {
    /* need this to avoid running out of memory */
    blk_queue_bounce_limit(device->request_queue, BLK_BOUNCE_ANY); 
    blk_queue_dma_alignment(device->request_queue, 0);
    if (device->tagged_supported) {
        device->current_tag = 0;
        device->queue_depth = 64;
    } else {
        PRINT("warning: channel %i target %i lun %i does NOT support TCQ\n",
              device->channel, device->id, device->lun);
        device->queue_depth = 1;
    }
    return 0;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static int iscsi_bios_param(struct scsi_device *disk, struct block_device *dev,
                            sector_t capacity, int *ip) {
    int sectors = capacity;
#else
static int iscsi_bios_param(Disk *disk, kdev_t dev, int *ip) {
    int sectors = disk->capacity;
#endif
    ip[0] = 32;                           // heads
    ip[1] = 63;                           // sectors
    if((ip[2] = sectors >> 11) > 1024) {  // cylinders, test for big disk
	ip[0] = 255;                      // heads
	ip[1] = 63;                       // sectors
	ip[2] = sectors / (255 * 63);     // cylinders
    }
    TRACE(TRACE_DEBUG, 0, "%u sectors, H/S/C: %u/%u/%u\n", sectors, ip[0], 
	  ip[1], ip[2]);
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static int iscsi_proc_info (struct Scsi_Host *host, char *buffer, 
			    char **start, off_t offset, int length, 
			    int writing) {
#else
static int iscsi_proc_info (char *buffer, char **start, off_t offset, 
			    int length, int hostno, int writing) {
#endif
    static char *info = NULL;
    static int returned = 0;
    int i, j, len = 0;
    static int tracing = 0;
    
    if (writing) {
	if (!strncmp(buffer, "reset-all", 9)) {
	    stats_reset();
#if 0
	} else if (!strncmp(buffer, "reset-lba-a", 11)) {
	    for (i=0; i<g_driver_num_targets; i++) {
		for (j=0; j<1; j++) {
		    g_stats[i][j].min_lba_a = 0xffffffff;
		    g_stats[i][j].max_lba_a = 0;
		}
	    }
	} else if (!strncmp(buffer, "reset-lba-b", 11)) {
	    for (i=0; i<g_driver_num_targets; i++) {
		for (j=0; j<1; j++) {
		    g_stats[i][j].min_lba_b = 0xffffffff;
		    g_stats[i][j].max_lba_b = 0;
		}
	    }
#endif
	} else if (!strncmp(buffer, "unjam", 5)) {
	    PRINT("unjamming...\n"); 
	    ISCSI_UNLOCK_ELSE(&g_proc_lock, return -1);
	    iscsi_free_atomic(info);
	    returned = 0;
	    info = NULL;
	} else if (!strncmp(buffer, "dump-trace", 10)) {
	    tracing = 1;
	}
	return length;
    } else if (!tracing) {
	if (!offset) ISCSI_LOCK_ELSE(&g_proc_lock, return -1);
	if (offset) {
	    if (info) {
		if (offset == strlen(info)) {
		    iscsi_free_atomic(info);
		    info = NULL;
		    returned = 0;
		    ISCSI_UNLOCK_ELSE(&g_proc_lock, return -1);
		    return len;
		}
	    } else {
		TRACE_ERROR("info buffer is null!\n");
		return -1;
	    }
	    goto done; 
	}
	if ((info=iscsi_malloc_atomic(65536))==NULL) {
	    TRACE_ERROR("iscsi_malloc_atomic() failed\n");
	    return -1;
	}
	len += sprintf(info, "%s\n\n", driver_template.name);
	len += sprintf(info+strlen(info), "Write this file to reset "
		       "counters (i.e., \"echo reset-all > "
		       "/proc/scsi/iscsi/*\").\n\n");
	for (i=0; i<g_driver_num_targets; i++) {
	    for (j=0; j<1; j++) {
		if (len>32768) {
		    TRACE_ERROR("buffer full\n");
		    goto done;
		}
		len += sprintf(info+strlen(info), 
			       "       outstanding[%i][%i]:  %u\n", 
			       i, j, g_stats[i][j].outstanding);
		len += sprintf(info+strlen(info), 
			       "    outstanding_wr[%i][%i]:  %u\n", 
			       i, j, g_stats[i][j].outstanding_wr);
		len += sprintf(info+strlen(info), 
			       "    outstanding_rd[%i][%i]:  %u\n", 
			       i, j, g_stats[i][j].outstanding_rd);
		len += sprintf(info+strlen(info), 
			       "  outstanding_bidi[%i][%i]:  %u\n", 
			       i, j, g_stats[i][j].outstanding_bidi);
		len += sprintf(info+strlen(info), 
			       " outstanding_other[%i][%i]:  %u\n", 
			       i, j, g_stats[i][j].outstanding_other);
		len += sprintf(info+strlen(info), 
			       "     bytes_sending[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].q_bytes_tx);
		len += sprintf(info+strlen(info), 
			       "   bytes_receiving[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].q_bytes_rx);
		len += sprintf(info+strlen(info), 
			       "               ops[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].succeeded);
		len += sprintf(info+strlen(info), 
			       "            ops_wr[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].ops_wr);
		len += sprintf(info+strlen(info), 
			       "            ops_rd[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].ops_rd);
		len += sprintf(info+strlen(info), 
			       "          ops_bidi[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].ops_bidi);
		len += sprintf(info+strlen(info), 
			       "         ops_other[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].ops_other);
		len += sprintf(info+strlen(info), 
			       "        ops_failed[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].failed);
		len += sprintf(info+strlen(info), 
			       "       ops_aborted[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].aborted);
		len += sprintf(info+strlen(info), 
			       "     device_resets[%i][%i]:  %u\n", 
			       i, j, g_stats[i][j].device_resets);
		len += sprintf(info+strlen(info), 
			       "        bus_resets[%i][%i]:  %u\n", 
			       i, j, g_stats[i][j].bus_resets);
		len += sprintf(info+strlen(info), 
			       "       host_resets[%i][%i]:  %u\n", 
			       i, j, g_stats[i][j].host_resets);
		len += sprintf(info+strlen(info), 
			       "        bytes_sent[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].bytes_tx);
		len += sprintf(info+strlen(info), 
			       "    bytes_received[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].bytes_rx);
		len += sprintf(info+strlen(info), 
			       "      op_latencies[%i][%i]:  %llu usec\n", 
			       i, j, g_stats[i][j].latencies);
		len += sprintf(info+strlen(info), 
			       "   op_latencies_wr[%i][%i]:  %llu usec\n", 
			       i, j, g_stats[i][j].latencies_wr);
		len += sprintf(info+strlen(info), 
			       "   op_latencies_rd[%i][%i]:  %llu usec\n", 
			       i, j, g_stats[i][j].latencies_rd);
		len += sprintf(info+strlen(info), 
			       " op_latencies_bidi[%i][%i]:  %llu usec\n", 
			       i, j, g_stats[i][j].latencies_bidi);
		len += sprintf(info+strlen(info), 
			       "op_latencies_other[%i][%i]:  %llu usec\n", 
			       i, j, g_stats[i][j].latencies_other);
		len += sprintf(info+strlen(info), 
			       "         op_depths[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].depths);
		len += sprintf(info+strlen(info), 
			       "      op_depths_wr[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].depths_wr);
		len += sprintf(info+strlen(info), 
			       "      op_depths_rd[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].depths_rd);
		len += sprintf(info+strlen(info), 
			       "    op_depths_bidi[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].depths_bidi);
		len += sprintf(info+strlen(info), 
			       "   op_depths_other[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].depths_other);
#if 0
		len += sprintf(info+strlen(info), 
			       "         min_lba_a[%i][%i]:  %u\n", 
			       i, j, g_stats[i][j].min_lba_a);
		len += sprintf(info+strlen(info), 
			       "         max_lba_a[%i][%i]:  %u\n", 
			       i, j, g_stats[i][j].max_lba_a);
		len += sprintf(info+strlen(info), 
			       "         min_lba_b[%i][%i]:  %u\n", 
			       i, j, g_stats[i][j].min_lba_b);
		len += sprintf(info+strlen(info), 
			       "         max_lba_b[%i][%i]:  %u\n", 
			       i, j, g_stats[i][j].max_lba_b);
#endif
		len += sprintf(info+strlen(info), 
			       "          op_jumps[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].jumps);
		len += sprintf(info+strlen(info), 
			       "       op_jumps_wr[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].write_jumps);
		len += sprintf(info+strlen(info), 
			       "       op_jumps_rd[%i][%i]:  %llu\n", 
			       i, j, g_stats[i][j].read_jumps);
		len += sprintf(info+strlen(info), 
			       " min_response_time[%i][%i]:  %u usec\n",
			       i, j, g_stats[i][j].min_response);
		len += sprintf(info+strlen(info), 
			       " max_response_time[%i][%i]:  %u usec\n",
			       i, j, g_stats[i][j].max_response);
#if 0
		len += sprintf(info+strlen(info), 
			       " avg_response_time[%i][%i]:  %u usec "
			       "(last %i)\n", i, j, 
			       g_stats[i][j].avg_response_10, 
			       g_stats[i][j].avg_response_10_count);
		len += sprintf(info+strlen(info), 
			       " avg response time[%i][%i]:  %u usec "
			       "(last %i)\n", i, j, 
			       g_stats[i][j].avg_response_100, 
			       g_stats[i][j].avg_response_100_count);
		len += sprintf(info+strlen(info), 
			       " avg response time[%i][%i]:  %u usec "
			       "(last %i)\n\n", i, j, 
			       g_stats[i][j].avg_response_1000, 
			       g_stats[i][j].avg_response_1000_count);
#endif
	    }
	}
	if ((len += initiator_info(info+strlen(info)))==-1) {
	    TRACE_ERROR("initiator_info() failed\n");
	    if (info != NULL) iscsi_free_atomic(info);
	    return -1;
	}
    done: 
	*start = buffer;
	len =  MIN(length, strlen(info)-returned);
	memcpy(buffer, info+offset, len);
	returned +=len;
	return len;
    } else if (trace) {
	struct trace_record *ptr, *tmp;
	char record[1024];

	buffer[0] = '\0';
	ISCSI_SPIN_LOCK_ELSE(&g_trace_lock, return -1);
	ptr=trace_head;
	if (!ptr) tracing = 0;
	while (ptr) {
	    sprintf(record, "%lu %lu %c %llu %u\n", 
		    ptr->arrival.tv_sec, ptr->arrival.tv_usec, 
		    ptr->write?'w':'r', ptr->offset, ptr->len);
	    if (strlen(buffer)+strlen(record)>length) {
		break;
	    }
	    sprintf(buffer+strlen(buffer), "%s", record);
	    tmp = ptr;
	    ptr = ptr->next;
	    trace_head = ptr;
	    iscsi_free_atomic(tmp);
	}
	trace_head = ptr;
	if (!ptr) {
	    TRACE_ERROR("reached end, turning off tracing\n");
	    trace_tail = NULL;
	}
	ISCSI_SPIN_UNLOCK_ELSE(&g_trace_lock, return -1);
	*start = buffer;
	return strlen(buffer);
    }
    return 0;
}

/*
 * Error handling routines (this code is buggy)
 */

int iscsi_abort_handler (struct scsi_cmnd *SCpnt) {
    int i;

    g_stats[TID(SCpnt)][LUN(SCpnt)].aborted++;
    for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {
	if (g_cmd[i].callback_arg == SCpnt) {
	    PRINT("iscsi: scsi abort (op 0x%x, iscsi tag 0x%x)\n", 
		  SCpnt->cmnd[0], g_cmd[i].tag);
	    if (initiator_abort(&g_cmd[i])!=0) {
		TRACE_ERROR("error aborting 0x%x\n", g_cmd[i].tag);
		return FAILED;
	    }
	    return SUCCESS;
	}
    }
    PRINT("iscsi: scsi abort (op 0x%x, iscsi tag ?)\n", SCpnt->cmnd[0]);
    return FAILED;
}

int iscsi_device_reset_handler (struct scsi_cmnd *SCpnt) {
    PRINT("iscsi: scsi device reset (op 0x%x, tag 0x%x)\n", 
	  SCpnt->cmnd[0], SCpnt->tag);
    initiator_session_reset(TID(SCpnt));
    g_stats[TID(SCpnt)][LUN(SCpnt)].device_resets++;
    return SUCCESS;
}

int iscsi_bus_reset_handler (struct scsi_cmnd *SCpnt) {
    PRINT("iscsi: scsi bus reset (op 0x%x, tag 0x%x)\n", 
	  SCpnt->cmnd[0], SCpnt->tag);
    g_stats[TID(SCpnt)][LUN(SCpnt)].bus_resets++;
    return SUCCESS;
}

int iscsi_host_reset_handler(struct scsi_cmnd *SCpnt) {
#if 0
    struct Scsi_Host *host = SCpnt->device->host;
    spin_unlock_irq(REQUEST_LOCK);
    spin_lock_irq(REQUEST_LOCK);
#endif
    PRINT("iscsi: scsi host reset (op 0x%x, tag 0x%x)\n", 
	  SCpnt->cmnd[0], SCpnt->tag);
    g_stats[TID(SCpnt)][LUN(SCpnt)].host_resets++;
    g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding = 0;
    g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_wr = 0;
    g_stats[TID(SCpnt)][LUN(SCpnt)].outstanding_rd = 0;
    g_stats[TID(SCpnt)][LUN(SCpnt)].q_bytes_tx = 0;
    g_stats[TID(SCpnt)][LUN(SCpnt)].q_bytes_rx = 0;
    if (initiator_shutdown()!=0) {
	TRACE_ERROR("initiator_shutdown() failed\n");
	return FAILED;
    }
    if (initiator_init(config, verbose, family)==-1) {
	TRACE_ERROR("initiator_init() failed\n");
	return FAILED;
    }
    return SUCCESS;
}

