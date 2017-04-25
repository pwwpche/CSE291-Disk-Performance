
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
 * Linux SCSI upper layer driver for OSD
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/kernel.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,18)
#error "Kernels older than 2.4.18 are no longer supported"
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#error "Kernel version 2.5 is not supported"
#endif
#endif

/*
 * Header Files
 */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/bio.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/idr.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/kref.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_driver.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/scsi_request.h>
#include <scsi/scsicam.h>
#include <scsi_logging.h>

#include "osd.h"
#include "debug.h"

#else   /* 2.4 */

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/hdreg.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/blk.h>
#include <linux/blkpg.h>
#include "scsi.h"
#include "hosts.h"
#include "so.h"
#include <scsi/scsi_ioctl.h>
#include "constants.h"
#include <scsi/scsicam.h>       
#include <linux/genhd.h>
#include "util.h"
#include "debug.h"
#include "osd.h"
#include "so.h"

#define DEVICE_NR(device)       MINOR(device)   /* blk.h */

#endif

/*
 * Licensing
 */

MODULE_AUTHOR("Intel Corporation, <http://www.intel.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("OSD (so) driver");

/*
 * Constants
 */
#define SCSI_OSD_MAJOR          232                  /* major.h */
#define MAJOR_NR                SCSI_OSD_MAJOR       /* hosts.h */
#define DEVICE_NAME             "scsiosd"            /* blk.h   */
#define SCSI_OSDS_PER_MAJOR     256
#define MAX_RETRIES             10
#define SO_TIMEOUT              (5*HZ)

#define SO_SECTOR_SIZE   512
#define SO_BLOCK_SIZE    1024
#define OSD_OPCODE      0x7f

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static int pack_osd_cdb(osd_args_t *args, struct scsi_cmnd *SCpnt, 
                        unsigned int block, unsigned int this_count)
{
    struct request *rq = SCpnt->request;
    //PRINT("pack_osd_cdb (%li)\n", jiffies);
        
    switch(rq_data_dir(rq))
#else
        static int pack_osd_cdb(osd_args_t *args, Scsi_Cmnd *SCpnt, 
                                unsigned int block, unsigned int this_count)
        {
            switch(SCpnt->request.cmd)
#endif   
                {
                case WRITE:

                    TRACE(TRACE_SO, 0, "Issuing an OSD write\n");

                    args->opcode = OSD_OPCODE;       /* 0x7f */
                    args->add_cdb_len = CONFIG_OSD_CDB_LEN-7;
                    args->service_action = OSD_WRITE;
                    args->length = this_count * SO_SECTOR_SIZE;
                    args->offset = block * SO_SECTOR_SIZE;
                    OSD_ENCAP_CDB(args, SCpnt->cmnd);
                    SCpnt->cmd_len = CONFIG_OSD_CDB_LEN;
                    SCpnt->result = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
                    SCpnt->sc_data_direction = DMA_TO_DEVICE;
#else
                    SCpnt->sc_data_direction = SCSI_DATA_WRITE;
#endif
                    break;

                case READ:
                    TRACE(TRACE_SO, 0, "Issuing an OSD read\n");

                    args->opcode = OSD_OPCODE;  /* 0x7f */
                    args->add_cdb_len = CONFIG_OSD_CDB_LEN-7;
                    args->service_action = OSD_READ;
                    args->length = this_count * SO_SECTOR_SIZE;
                    args->offset = block * SO_SECTOR_SIZE;
                    OSD_ENCAP_CDB(args, SCpnt->cmnd);
                    SCpnt->cmd_len = CONFIG_OSD_CDB_LEN;                
                    SCpnt->result = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
                    SCpnt->sc_data_direction = DMA_FROM_DEVICE;
#else
                    SCpnt->sc_data_direction = SCSI_DATA_READ;
#endif
                    break;
                default:
                    printk("so: Unknown command\n");
                    return -1;  /* Error */
                }
            return 0;   /* success */
        }


    /* -- 2.6 part of so.c starts here -- */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)

    /*
     * This is limited by the number of OSDs per major
     */
#define SO_MAX_DISKS    SCSI_OSDS_PER_MAJOR

    /*
     * Number of allowed retries
     */
#define SO_MAX_RETRIES 6

    static void osd_release(struct kref *kref);

    struct osd {
        struct scsi_driver *driver;     /* always &sd_template */
        struct scsi_device *device;
        struct kref     kref;
        struct gendisk  *disk;
        unsigned int    openers;        /* protected by BKL for now, yuck */
        sector_t        capacity;       /* size in 512-byte sectors */
        u32             index;
        u8              media_present;
        u8              write_prot;
        unsigned        WCE : 1;        /* writeback cache enable */
        unsigned        RCD : 1;        /* unused - read cache disable */
    };

    static DEFINE_IDR(so_index_idr);
    static DEFINE_SPINLOCK(so_index_lock);

    /* This semaphore is used to mediate the 0->1 reference get in the
     * face of object destruction (i.e. we can't allow a get on an
     * object after last put) */
    static DECLARE_MUTEX(so_ref_sem);

    static int so_revalidate_disk(struct gendisk *disk);
    static void so_rw_intr(struct scsi_cmnd * SCpnt);

    static int so_probe(struct device *);
    static int so_remove(struct device *);
    static void so_shutdown(struct device *dev);
    static void so_rescan(struct device *);
    static int so_init_command(struct scsi_cmnd *);

    static struct scsi_driver so_template = {
        .owner                  = THIS_MODULE,
        .gendrv = {
            .name               = "so",
            .probe              = so_probe,
            .remove             = so_remove,
            .shutdown   = so_shutdown,
        },
        .init_command           = so_init_command,
        .rescan                 = so_rescan,
#if 0
        .end_flush              = so_end_flush,
        .issue_flush            = so_issue_flush,
        .prepare_flush          = so_prepare_flush,
#endif

    };

#define to_osd(obj) container_of(obj,struct osd,kref)

    static inline struct osd *osd(struct gendisk *disk)
    {
        return container_of(disk->private_data, struct osd, driver);
    }

    static struct osd *osd_get(struct gendisk *disk)
    {
        struct osd *sokp = NULL;

        down(&so_ref_sem);
        if (disk->private_data == NULL)
            goto out;
        sokp = osd(disk);
        kref_get(&sokp->kref);
        if (scsi_device_get(sokp->device))
            goto out_put;
        up(&so_ref_sem);
        return sokp;

    out_put:
        kref_put(&sokp->kref, osd_release);
        sokp = NULL;
    out:
        up(&so_ref_sem);
        return sokp;
    }

    static void osd_put(struct osd *sokp)
    {
        struct scsi_device *sdev = sokp->device;

        down(&so_ref_sem);
        kref_put(&sokp->kref, osd_release);
        scsi_device_put(sdev);
        up(&so_ref_sem);
    }

    /**
     *  so_init_command - build an osd (read or write) command from
     *  information in the request structure.
     *  @SCpnt: pointer to mid-level's per scsi command structure that
     *  contains request and into which the scsi command is written
     *
     *  Returns 1 if successful and 0 if error (or cannot be done now).
     **/
    static int so_init_command(struct scsi_cmnd * SCpnt)
    {
        unsigned int this_count, timeout;
        struct gendisk *disk;
        sector_t block;
        struct scsi_device *sdp = SCpnt->device;
        struct request *rq = SCpnt->request;
        osd_args_t args;

        //PRINT("so_init_command (%li)\n", jiffies);

        timeout = sdp->timeout;

        /* TBD: SG_IO support? Need to add support for the SG_IO ioctl */ 

        /*
         * we only do REQ_CMD and REQ_BLOCK_PC
         */
        if (!blk_fs_request(rq))
            return 0;

        disk = rq->rq_disk;
        block = rq->sector * sdp->sector_size / SO_SECTOR_SIZE; /* offset */
        this_count = SCpnt->request_bufflen / SO_SECTOR_SIZE;   /* len */

#ifdef CONFIG_SCSI_LOGGING
        SCSI_LOG_HLQUEUE(1, printk("so_init_command: disk=%s, block=%llu, "
                                   "count=%d\n", disk->disk_name,
                                   (uint64_t)block, this_count));

        if (!sdp || !scsi_device_online(sdp)) {
            SCSI_LOG_HLQUEUE(2, printk("Finishing %ld sectors\n", 
                                       rq->nr_sectors));
            SCSI_LOG_HLQUEUE(2, printk("Retry with 0x%p\n", SCpnt));
            return 0;
        }
#endif

        if (sdp->changed) {
            /*
             * quietly refuse to do anything to a changed disc until 
             * the changed bit has been reset
             */
            return 0;
        }

        TRACE(TRACE_SO, 0, "%s : block=%llu\n", disk->disk_name, 
              (uint64_t)block);

        memset(&args, 0, sizeof(osd_args_t));

        /* TBD: this should change for osdfs support */
        args.GroupID = 0;
        args.UserID = 0;

        if(pack_osd_cdb(&args, SCpnt, block, this_count) != 0) {
            TRACE(TRACE_SO, 0, 
                  "Error packing SCSI osd command.\n");
            return 0;
        }

        TRACE(TRACE_SO, 0, "%s : %s %d/%ld %d byte blocks.\n", 
              disk->disk_name, (rq_data_dir(rq) == WRITE) ? 
              "writing" : "reading", this_count, rq->nr_sectors, 
              SO_SECTOR_SIZE);

        SCpnt->request_bufflen = SCpnt->bufflen =
            this_count * SO_SECTOR_SIZE;

        SCpnt->timeout_per_command = SO_TIMEOUT;

        /*
         * This is the completion routine we use.  This is matched in terms
         * of capability to this function.
         */
        SCpnt->done = so_rw_intr;

        /*
         * This indicates that the command is ready from our end to be
         * queued.
         */
        return 1;
    }

    /**
     *  so_open - open an OSD  
     *  @inode: only i_rdev member may be used
     *  @filp: only f_mode and f_flags may be used
     *
     *  Returns 0 if successful. Returns a negated errno value in case 
     *  of error.
     *
     *  Note: This can be called from a user context (e.g. fsck(1) )
     *  or from within the kernel (e.g. as a result of a mount(1) ).
     *  In the latter case @inode and @filp carry an abridged amount
     *  of information as noted above.
     **/
    static int so_open(struct inode *inode, struct file *filp)
    {
        struct gendisk *disk = inode->i_bdev->bd_disk;
        struct osd *sokp;
        struct scsi_device *sdev;
        int retval;

        //PRINT("so_open (%li)\n", jiffies);

        if (!(sokp = osd_get(disk)))
            return -ENXIO;

        TRACE(TRACE_SO, 0, "so_open: disk=%s\n", disk->disk_name);

        sdev = sokp->device;

        /*
         * If the device is in error recovery, wait until it is done.
         * If the device is offline, then disallow any access to it.
         */
        retval = -ENXIO;
        if (!scsi_block_when_processing_errors(sdev))
            goto error_out;

        if (sdev->removable || sokp->write_prot)
            check_disk_change(inode->i_bdev);

        /*
         * If the drive is empty, just let the open fail.
         */
        retval = -ENOMEDIUM;
        if (sdev->removable && !sokp->media_present &&
            !(filp->f_flags & O_NDELAY))
            goto error_out;

        /*
         * If the device has the write protect tab set, have the open fail
         * if the user expects to be able to write to the thing.
         */
        retval = -EROFS;
        if (sokp->write_prot && (filp->f_mode & FMODE_WRITE))
            goto error_out;

        /*
         * It is possible that the osd changing stuff resulted in
         * the device being taken offline.  If this is the case,
         * report this to the user, and don't pretend that the
         * open actually succeeded.
         */
        retval = -ENXIO;
        if (!scsi_device_online(sdev))
            goto error_out;

        if (!sokp->openers++ && sdev->removable) {
            if (scsi_block_when_processing_errors(sdev))
                scsi_set_medium_removal(sdev, SCSI_REMOVAL_PREVENT);
        }

        TRACE(TRACE_SO, 0, "so_open returning SUCCESS\n");
        return 0;

    error_out:
        printk("so_open returning ERROR\n");
        osd_put(sokp);
        return retval;  
    }

    /**
     *  so_release - invoked when the (last) close(2) is called on this
     *  scsi osd.
     *  @inode: only i_rdev member may be used
     *  @filp: only f_mode and f_flags may be used
     *
     *  Returns 0. 
     *
     *  Note: may block (uninterruptible) if error recovery is underway
     *  on this disk.
     **/
    static int so_release(struct inode *inode, struct file *filp)
    {
        struct gendisk *disk = inode->i_bdev->bd_disk;
        struct osd *sokp = osd(disk);
        struct scsi_device *sdev = sokp->device;

        TRACE(TRACE_SO, 0, "so_release: disk=%s\n", disk->disk_name);

        if (!--sokp->openers && sdev->removable) {
            if (scsi_block_when_processing_errors(sdev))
                scsi_set_medium_removal(sdev, SCSI_REMOVAL_ALLOW);
        }

        osd_put(sokp);
        return 0;
    }

    static int so_sync_cache(struct scsi_device *sop)
    {

        /* TBD: we could send across a FLUSH PARTITION command */
        PRINT("so_sync_cache (%li)\n", jiffies);
        return 0;
    }

    static void so_rescan(struct device *dev)
    {
        struct osd *sokp = dev_get_drvdata(dev);
        so_revalidate_disk(sokp->disk);
    }

    static struct block_device_operations so_fops = {
        .owner                  = THIS_MODULE,
        .open                   = so_open,
        .release                = so_release,
#if 0
        .ioctl                  = so_ioctl,
        .media_changed          = so_media_changed,
        .revalidate_disk        = so_revalidate_disk,
#endif
    };

    /**
     *  so_rw_intr - bottom half handler: called when the lower level
     *  driver has completed (successfully or otherwise) an scsi OSD command.
     *  @SCpnt: mid-level's per command structure.
     *
     *  Note: potentially run from within an ISR. Must not block.
     **/
    static void so_rw_intr(struct scsi_cmnd * SCpnt)
    {
        int result = SCpnt->result;
        int this_count = SCpnt->bufflen;

        /* 
         * Tell the SCSI middle layer no bytes were processed if there
         * an error.
         * TBD: Handle partial errors
         */
        int good_bytes = (result == 0 ? this_count : 0);
        sector_t block_sectors = 0;     /* 0 means no errors! */
        struct scsi_sense_hdr sshdr;
        int sense_deferred = 0;
        int sense_valid = 0;

        //PRINT("so_rw_intr (%li)\n", jiffies);

        if (result) {
            sense_valid = scsi_command_normalize_sense(SCpnt, &sshdr);
            if (sense_valid)
                sense_deferred = scsi_sense_is_deferred(&sshdr);
        }

#ifdef CONFIG_SCSI_LOGGING
        SCSI_LOG_HLCOMPLETE(1, printk("sd_rw_intr: %s: res=0x%x\n", 
                                      SCpnt->request->rq_disk->disk_name, 
                                      result));
        if (sense_valid) {
            SCSI_LOG_HLCOMPLETE(1, printk("sd_rw_intr: sb[respc,sk,asc,"
                                          "ascq]=%x,%x,%x,%x\n", 
                                          sshdr.response_code,
                                          sshdr.sense_key, sshdr.asc, 
                                          sshdr.ascq));
        }
#endif

        /* TBD: MEDIUM ERRORs that indicate partial success are not handled */

        /*
         * This calls the generic completion function, now that we know
         * how many actual sectors finished, and how many sectors we need
         * to say have failed.
         */
        scsi_io_completion(SCpnt, good_bytes, block_sectors << 9);
    }


    /* TBD: do we need spin up and read capacity for OSDs? -- T10 command 
     * set document does not have a read capacity command. It will be 
     * interesting to figure out what an OSD might do an a invalidate
     * command.
     */

    /*
     * read disk capacity
     */
    static void
        so_read_capacity(struct osd *sokp, char *diskname, 
                         unsigned char *buffer)
    {
        /* 
         * TBD: as of now we set the capacity to a random quantity
         * Ideally, we should figure out a way to ask the OSD target.
         */
        sokp->capacity = (1024000) << 1;        /* in units of sector size */
        sokp->device->sector_size = SO_SECTOR_SIZE;
    }

    /**
     *  so_revalidate_disk - called the first time a new OSD disk is seen,
     *  @disk: struct gendisk we care about
     **/
    static int so_revalidate_disk(struct gendisk *disk)
    {
        struct osd *sokp = osd(disk);
        struct scsi_device *sdp = sokp->device;
        unsigned char *buffer;

        TRACE(TRACE_SO, 0, "so_revalidate_disk: disk=%s\n", disk->disk_name);

        /*
         * If the device is offline, don't try and read capacity or any
         * of the other niceties.
         */
        if (!scsi_device_online(sdp))
            goto out;

        buffer = kmalloc(512, GFP_KERNEL | __GFP_DMA);
        if (!buffer) {
            printk("(so_revalidate_disk:) Memory allocation "
                   "failure.\n");
            goto out;
        }

        /* defaults, until the device tells us otherwise */
        sdp->sector_size = 512;
        sokp->capacity = 0;
        sokp->media_present = 1;
        sokp->write_prot = 0;
        sokp->WCE = 0;  /* write through */
        sokp->RCD = 0;  /* no read ahead */

        so_read_capacity(sokp, disk->disk_name, buffer);

        TRACE(TRACE_SO, 0, "so_revalidate_disk: setting capacity to %u\n", 
              (unsigned int)sokp->capacity);
        set_capacity(disk, sokp->capacity);     
        kfree(buffer);

    out:
        TRACE(TRACE_SO, 0, "so_revalidate_disk: returning SUCCESS\n");
        return 0;
    }

    /**
     *  so_probe - called during driver initialization and whenever a
     *  new OSD device is attached to the system. It is called once
     *  for each OSD device (not just disks) present.
     *  @dev: pointer to device object
     *
     *  Returns 0 if successful (or not interested in this OSD device 
     *  1 when there is an error.
     *
     *  Note: this function is invoked from the scsi mid-level.
     *  This function sets up the mapping between a given 
     *  <host,channel,id,lun> (found in sop) and new device name 
     *  (e.g. /dev/so0). More precisely it is the block device major 
     *  and minor number that is chosen here.
     *
     **/
    static int so_probe(struct device *dev)
    {
        struct scsi_device *sop = to_scsi_device(dev);
        struct osd *sokp;
        struct gendisk *gd;
        u32 index;
        int error;

        error = -ENODEV;

        if (sop->type != TYPE_OSD) 
            goto out;

#ifdef CONFIG_SCSI_LOGGING
        SCSI_LOG_HLQUEUE(3, printk("so_attach: scsi device: <%d,%d,%d,%d>\n", 
                                   sop->host->host_no, sop->channel, 
                                   sop->id, sop->lun));
#endif

        error = -ENOMEM;
        sokp = kmalloc(sizeof(*sokp), GFP_KERNEL);
        if (!sokp)
            goto out;

        memset (sokp, 0, sizeof(*sokp));
        kref_init(&sokp->kref);

        gd = alloc_disk(SCSI_OSDS_PER_MAJOR);
        if (!gd)
            goto out_free;

        if (!idr_pre_get(&so_index_idr, GFP_KERNEL))
            goto out_put;

        spin_lock(&so_index_lock);
        error = idr_get_new(&so_index_idr, NULL, &index);
        spin_unlock(&so_index_lock);

        if (index >= SO_MAX_DISKS)
            error = -EBUSY;
        if (error)
            goto out_put;

        sokp->device = sop;
        sokp->driver = &so_template;
        sokp->disk = gd;
        sokp->index = index;
        sokp->openers = 0;

        if (!sop->timeout)
            sop->timeout = SO_TIMEOUT;

        gd->major = SCSI_OSD_MAJOR; 
        gd->first_minor = index; 
        gd->minors = SCSI_OSDS_PER_MAJOR;
        gd->fops = &so_fops;

        sprintf(gd->disk_name, "so%i", index);
        
        strcpy(gd->devfs_name, sop->devfs_name);

        gd->private_data = &sokp->driver;

        so_revalidate_disk(gd);

        gd->driverfs_dev = &sop->sdev_gendev;
        gd->flags = GENHD_FL_DRIVERFS;
        if (sop->removable)
            gd->flags |= GENHD_FL_REMOVABLE;
        gd->queue = sokp->device->request_queue;

        dev_set_drvdata(dev, sokp);
        add_disk(gd);

        printk(KERN_NOTICE "Attached OSD %sdisk %s at scsi%d, channel %d, "
               "id %d, lun %d\n", sop->removable ? "removable " : "",
               gd->disk_name, sop->host->host_no, sop->channel,
               sop->id, sop->lun);

        return 0;

    out_put:
        put_disk(gd);
    out_free:
        kfree(sokp);
    out:
        return error;
    }

    /**
     *  so_remove - called whenever an osd disk (previously recognized by
     *  so_probe) is detached from the system. It is called (potentially
     *  multiple times) during so module unload.
     *  @sop: pointer to mid level scsi device object
     *
     *  Note: this function is invoked from the scsi mid-level.
     *  This function potentially frees up a device name (e.g. /dev/sdc)
     *  that could be re-used by a subsequent so_probe().
     *  This function is not called when the built-in sd driver is "exit-ed".
     **/
    static int so_remove(struct device *dev)
    {
        struct osd *sokp = dev_get_drvdata(dev);

        del_gendisk(sokp->disk);
        so_shutdown(dev);
        down(&so_ref_sem);
        kref_put(&sokp->kref, osd_release);
        up(&so_ref_sem);

        return 0;
    }

    /**
     *  osd_release - Called to free the osd structure
     *  @kref: pointer to embedded kref
     *
     *  so_ref_sem must be held entering this routine.  Because it is
     *  called on last put, you should always use the osd_get()
     *  osd_put() helpers which manipulate the semaphore directly
     *  and never do a direct kref_put().
     **/
    static void osd_release(struct kref *kref)
    {
        struct osd *sokp = to_osd(kref);
        struct gendisk *disk = sokp->disk;
        
        spin_lock(&so_index_lock);
        idr_remove(&so_index_idr, sokp->index);
        spin_unlock(&so_index_lock);

        disk->private_data = NULL;

        put_disk(disk);

        kfree(sokp);
    }

    /*
     * Send a SYNCHRONIZE CACHE instruction down to the device through
     * the normal SCSI command structure.  Wait for the command to
     * complete.
     */
    static void so_shutdown(struct device *dev)
    {
        struct scsi_device *sop = to_scsi_device(dev);
        struct osd *sokp = dev_get_drvdata(dev);

        if (!sokp)
            return;         /* this can happen */

        if (!sokp->WCE)
            return;

        TRACE(TRACE_SO, 0, 
              "Synchronizing OSD cache for disk %s (Not implemented yet!): \n",
              sokp->disk->disk_name);
        so_sync_cache(sop);
    }   

    /**
     *  init_so - entry point for this driver (both when built in or when
     *  a module).
     *
     *  Note: this function registers this driver with the scsi mid-level.
     **/
    static int __init init_so(void)
    {
        TRACE(TRACE_SO, 0, "init_so: so driver entry point\n");
        
        register_blkdev(SCSI_OSD_MAJOR, "so");

        return scsi_register_driver(&so_template.gendrv);
    }

    /**
     *  exit_so - exit point for this driver (when it is a module).
     *
     *  Note: this function unregisters this driver from the scsi mid-level.
     **/
    static void __exit exit_so(void)
    {
        TRACE(TRACE_SO, 0, "exit_so: exiting so driver\n");

        scsi_unregister_driver(&so_template.gendrv);
        unregister_blkdev(SCSI_OSD_MAJOR, "so");
    }


#else /* 2.4 specific part of so.c starts here */

    /*
     * Globals
     */

    struct hd_struct *so;
    static Scsi_Osd *rscsi_osds;
    static int *so_sizes;
    static int *so_blk_sizes;
    static int *so_blksize_sizes;
    static int *so_hardsizes;
    static int *so_max_sectors;

    /*
     * Function prototypes
     */

    static int so_init(void);
    static void so_finish(void);
    static int so_attach(Scsi_Device *);
    static int so_detect(Scsi_Device *);
    static void so_detach(Scsi_Device *);
    static int so_init_command(Scsi_Cmnd *);
    static void rw_intr(Scsi_Cmnd * SCpnt);

    /* 
     * Globals
     */

    static int so_registered;

    /*
     * Templates
     */

    static struct Scsi_Device_Template so_template = {
    name:               "osd",
    tag:                "so",
    scsi_type:          TYPE_OSD,
    major:              SCSI_OSD_MAJOR,
    blk:                1,
    detect:             so_detect,
    init:               so_init,
    finish:             so_finish,
    attach:             so_attach,
    detach:             so_detach,
    init_command:       so_init_command,
    };

    /*
     * Functions
     */

    static void so_devname(unsigned int index, char *buffer) {
        TRACE(TRACE_SO, 1, "so_devname(%i)\n", index);
        sprintf(buffer, "so%i", index);
    }

    static request_queue_t *so_find_queue(kdev_t dev)
    {
        Scsi_Osd *dpnt;
        int target;

        TRACE(TRACE_SO, 1, "so_find_queue(dev %u)\n", dev);

        target = DEVICE_NR(dev);

        dpnt = &rscsi_osds[target];
        if (!dpnt) {
            TRACE_ERROR("no such device\n");
            return NULL;
        }
        return &dpnt->device->request_queue;
    }

    static int so_init_command(Scsi_Cmnd * SCpnt)
    {
        int block, this_count;
        Scsi_Osd *dpnt;
        char nbuff[6];
        osd_args_t args;
        int index;

        index = MINOR(SCpnt->request.rq_dev);
        so_devname(index, nbuff);

        block = (SCpnt->request.sector*512)/SO_SECTOR_SIZE; 
        this_count = SCpnt->request_bufflen/SO_SECTOR_SIZE;

        memset(&args, 0, sizeof(osd_args_t));

        /*
         * If OSDFS is mounted over OSD, then OSDFS will have set generic_sbp
         * to be the root object id.
         */

        if (!SCpnt->request.bh) {
            TRACE_ERROR("SCpnt->request.bh is NULL\n");
            return -1;
        } 
        if (!SCpnt->request.bh->b_page) {
            TRACE_ERROR("SCpnt->request.bh->b_page is NULL\n");
            return -1;
        }    
        if (!SCpnt->request.bh->b_page->mapping) {
            TRACE_ERROR("page %p has no mapping\n", 
                        SCpnt->request.bh->b_page);
            args.GroupID = 0;
            args.UserID = 0;
        } else { 
            if (!SCpnt->request.bh->b_page->mapping->host) {
                TRACE_ERROR("SCpnt->request.bh->b_page->mapping->host "
                            "is NULL\n");
                return -1;
            } 
            if (!SCpnt->request.bh->b_page->mapping->host->i_sb) {
                TRACE_ERROR("SCpnt->request.bh->b_page->mapping->host->i_sb "
                            "is NULL\n");
                return -1;
            }
            if (SCpnt->request.bh->b_page->mapping->host->i_sb->s_magic == 
                0xabcdef01) {
                args.GroupID = *((unsigned *) SCpnt->request.bh->b_page->
                                 mapping->host->i_sb->u.generic_sbp);
                args.UserID = args.UserID = SCpnt->request.bh->b_page->
                    mapping->host->i_ino;
            } else {
                args.GroupID = 0;
                args.UserID = 0;
            }
        }

        TRACE(TRACE_SO, 1, 
              "so_init_command(MAJ %i MIN %i sect %li -> blk %i, len %u -> "
              "count %i)\n", 
              MAJOR(SCpnt->request.rq_dev), MINOR(SCpnt->request.rq_dev), 
              SCpnt->request.sector, block, SCpnt->request_bufflen, this_count);
        TRACE(TRACE_SO, 1, "GroupID 0x%x UserID 0x%llx\n", 
              args.GroupID, args.UserID);
    
        if (index >= so_template.dev_max) {
            TRACE_ERROR("index (%i) >= dev_max (%u)\n", index, 
                        so_template.dev_max);
        }
        dpnt = &rscsi_osds[index];
        if (!dpnt) {
            TRACE_ERROR("dpnt is NULL\n");
            return 0;
        }
        if (!dpnt->device->online) {
            TRACE_ERROR("device %i is offline\n", index);
            SCpnt->result = 0x2;
            return 0;
        }
        if (block + SCpnt->request.nr_sectors > so[index].nr_sects) {
            TRACE_ERROR("request out of range: %i offset + %li count > "
                        "%li total sectors\n",
                        block, SCpnt->request.nr_sectors, so[index].nr_sects);
            return 0;
        }

        block += so[index].start_sect;
        if (dpnt->device->changed) {
            TRACE_ERROR("SCSI osd has been changed. Prohibiting further I/O\n");
            return 0;
        }

        if(pack_osd_cdb(&args, SCpnt, block, this_count) != 0) {
            TRACE_ERROR("Error while packing SCSI osd command. Prohibiting "
                        "further I/O\n");
            return 0;
        }

        /*
         * We shouldn't disconnect in the middle of a sector, so with a dumb
         * host adapter, it's safe to assume that we can at least transfer
         * this many bytes between each connect / disconnect.
         */

        //SCpnt->transfersize = dpnt->device->sector_size;
        //SCpnt->underflow = this_count*SO_SECTOR_SIZE;
        //SCpnt->allowed = MAX_RETRIES;

        SCpnt->timeout_per_command = SO_TIMEOUT;

        /*
         * This is the completion routine we use.  This is matched in terms
         * of capability to this function.
         */

        SCpnt->done = rw_intr;

        /*
         * This indicates that the command is ready from our end to be
         * queued.
         */

        return 1;
    }

    static int so_open(struct inode *inode, struct file *filp)
    {
        int target;
        Scsi_Device * SDev;
        target = DEVICE_NR(inode->i_rdev);

        TRACE(TRACE_SO, 1, "so_open()\n");

#ifdef CONFIG_SCSI_LOGGING
        SCSI_LOG_HLQUEUE(1, printk("target=%d, max=%d\n", target, 
                                   so_template.dev_max));
#endif

        if (target >= so_template.dev_max || !rscsi_osds[target].device)
            return -ENXIO;      /* No such device */

        /*
         * If the device is in error recovery, wait until it is done.
         * If the device is offline, then disallow any access to it.
         */
        if (!scsi_block_when_processing_errors(rscsi_osds[target].device)) {
            return -ENXIO;
        }
        /*
          Make sure that only one process can do a check_change_osd at
          one time. This is also used to lock out further access when
          the partition table is being re-read.
        */

        while (rscsi_osds[target].device->busy)
            barrier();
        if (rscsi_osds[target].device->removable) {
            check_disk_change(inode->i_rdev);
        }
        SDev = rscsi_osds[target].device;
        /*
         * It is possible that the osd changing stuff resulted in the device
         * being taken offline.  If this is the case, report this to the user,
         * and don't pretend that
         * the open actually succeeded.
         */
        if (!SDev->online) {
            return -ENXIO;
        }
        /*
         * See if we are requesting a non-existent partition.  Do this
         * after checking for osd change.
         */
        if (so_sizes[MINOR(inode->i_rdev)] == 0)
            return -ENXIO;

        if (SDev->removable)
            if (!SDev->access_count)
                if (scsi_block_when_processing_errors(SDev))
                    scsi_ioctl(SDev, SCSI_IOCTL_DOORLOCK, NULL);

        SDev->access_count++;
        if (SDev->host->hostt->module)
            __MOD_INC_USE_COUNT(SDev->host->hostt->module);
        if (so_template.module)
            __MOD_INC_USE_COUNT(so_template.module);
        return 0;
    }

    static int so_release(struct inode *inode, struct file *file)
    {
        int target;
        Scsi_Device * SDev;

        TRACE(TRACE_SO, 1, "so_release()\n");

        target = DEVICE_NR(inode->i_rdev);
        SDev = rscsi_osds[target].device;

        SDev->access_count--;

        if (SDev->removable) {
            if (!SDev->access_count)
                if (scsi_block_when_processing_errors(SDev))
                    scsi_ioctl(SDev, SCSI_IOCTL_DOORUNLOCK, NULL);
        }
        if (SDev->host->hostt->module)
            __MOD_DEC_USE_COUNT(SDev->host->hostt->module);
        if (so_template.module)
            __MOD_DEC_USE_COUNT(so_template.module);
        return 0;
    }

    static int so_ioctl(struct inode * inode, struct file * file, 
                        unsigned int cmd, unsigned long arg) {
        kdev_t dev = inode->i_rdev;

        TRACE(TRACE_SO, 0, "so_ioctl()\n");
        return scsi_ioctl(rscsi_osds[DEVICE_NR(dev)].device , cmd, 
                          (void *) arg);
    }

    static struct block_device_operations so_fops =
        {
        open:                   so_open,
        release:                so_release,
        ioctl:                  so_ioctl,
        };

    /*
     *  If we need more than one SCSI osd major (i.e. more than
     *  16 SCSI osds), we'll have to kmalloc() more gendisks later.
     */

    static struct gendisk so_gendisk =
        {
            SCSI_OSD_MAJOR,     /* Major number */
            "so",                       /* Major name */
            0,                  /* Bits to shift to get real from partition */
            1,                  /* Number of partitions per real */
            NULL,                       /* hd struct */
            NULL,                       /* block sizes */
            0,                  /* number */
            NULL,                       /* internal */
            NULL,                       /* next */
            &so_fops,           /* file operations */
        };

    /*
     * rw_intr is the interrupt routine for the device driver.
     * It will be notified on the end of a SCSI read / write, and
     * will take one of several actions based on success or failure.
     */

    static void rw_intr(Scsi_Cmnd * SCpnt)
    {
        char nbuff[6];
        int good_sectors;
        int block_sectors = 0;

        so_devname(DEVICE_NR(SCpnt->request.rq_dev), nbuff);
        TRACE(TRACE_SO, 1, "rw_intr(/dev/%s, host %d, result 0x%x)\n", 
              nbuff, SCpnt->host->host_no, SCpnt->result);
        if (SCpnt->result != 0) {
            TRACE_ERROR("SCSI OSD command failed\n");
            good_sectors = 0;
        } else {
            good_sectors = SCpnt->bufflen/SO_SECTOR_SIZE;
        }
        scsi_io_completion(SCpnt, good_sectors, block_sectors);
    }

    /*
     * The so_init() function looks at all SCSI OSDs present.
     */

    static int so_init() {
        int i;

        TRACE(TRACE_SO, 0, "so_init()\n");
        if (so_template.dev_noticed == 0) {
            TRACE_ERROR("no OSDs noticed\n");
            return 0;
        }
        if (!rscsi_osds) {
            PRINT("%i osds detected \n", so_template.dev_noticed);
            so_template.dev_max = so_template.dev_noticed;
        }
        if (so_template.dev_max > SCSI_OSDS_PER_MAJOR) {
            TRACE_ERROR("so_template.dev_max (%i) > SCSI_OSDS_PER_MAJOR\n", 
                        so_template.dev_max);
            so_template.dev_max = SCSI_OSDS_PER_MAJOR;
        }
        if (!so_registered) {
            if (devfs_register_blkdev(SCSI_OSD_MAJOR, "so", &so_fops)) {
                printk("Unable to get major %d for SCSI osd\n", SCSI_OSD_MAJOR);
                return 1;
            }
            so_registered++;
        }

        /* No loadable devices yet */
        if (rscsi_osds) return 0;

        /* Device size in 1K blocks */

        if ((so_blk_sizes=kmalloc(so_template.dev_max*sizeof(int), 
                                  GFP_ATOMIC))==NULL) {
            TRACE_ERROR("kmalloc() failed\n");
            goto cleanup_devfs;
        }
        blk_size[SCSI_OSD_MAJOR] = so_blk_sizes;

        /* Block sizes */
        if ((so_blksize_sizes=kmalloc(so_template.dev_max*sizeof(int), 
                                      GFP_ATOMIC))==NULL) {
            TRACE_ERROR("kmalloc() failed\n");
            goto cleanup_so_blk_sizes;
        }
        blksize_size[SCSI_OSD_MAJOR] = so_blksize_sizes;

        /* Hardware sector sizes */
        if ((so_hardsizes=kmalloc(so_template.dev_max*sizeof(int), 
                                  GFP_ATOMIC))==NULL) {
            TRACE_ERROR("kmalloc() failed\n");
            goto cleanup_so_blksize_sizes;
        }
        hardsect_size[SCSI_OSD_MAJOR] = so_hardsizes;

        /* Max sectors per request */
        so_max_sectors = kmalloc((so_template.dev_max << 4) * sizeof(int), 
                                 GFP_ATOMIC);
        if (!so_max_sectors) {
            TRACE_ERROR("kmalloc() failed\n");
            goto cleanup_so_hardsizes;
        }
        max_sectors[SCSI_OSD_MAJOR] = so_max_sectors;

        /* Real devices */
        if ((rscsi_osds = kmalloc(so_template.dev_max * sizeof(Scsi_Osd), 
                                  GFP_ATOMIC))==NULL) {
            TRACE_ERROR("kmalloc() failed\n");
            goto cleanup_so_max_sectors;
        }
        memset(rscsi_osds, 0, so_template.dev_max * sizeof(Scsi_Osd));
        so_gendisk.real_devices = (void *) rscsi_osds;

        /* Partition sizes */
        if ((so_sizes=kmalloc(so_template.dev_max*sizeof(int), 
                              GFP_ATOMIC))==NULL) {
            TRACE_ERROR("kmalloc() failed\n");
            goto cleanup_rscsi_osds;
        }
        memset(so_sizes, 0, so_template.dev_max*sizeof(int));
        so_gendisk.sizes = so_sizes;

        /* Partitions (can't we get rid of this?) */
        if ((so=kmalloc(so_template.dev_max*sizeof(struct hd_struct), 
                        GFP_ATOMIC))==NULL) {
            TRACE_ERROR("kmalloc() failed\n");
            goto cleanup_so_sizes;
        }
        memset(so, 0, so_template.dev_max*sizeof(struct hd_struct));
        so_gendisk.part = so;

        /* ??? */
        if ((so_gendisk.de_arr=kmalloc(SCSI_OSDS_PER_MAJOR*
                                       sizeof(*so_gendisk.de_arr), 
                                       GFP_ATOMIC))==NULL) {
            TRACE_ERROR("kmalloc() failed\n");
            goto cleanup_so;
        }
        memset(so_gendisk.de_arr, 0, SCSI_OSDS_PER_MAJOR * 
               sizeof *so_gendisk.de_arr);

        /* Flags */
        if ((so_gendisk.flags=kmalloc(SCSI_OSDS_PER_MAJOR*
                                      sizeof(*so_gendisk.flags),
                                      GFP_ATOMIC))==NULL) {
            TRACE_ERROR("kmalloc() failed\n");
            goto cleanup_so_gendisk_de_arr;
        }
        memset(so_gendisk.flags, 0, SCSI_OSDS_PER_MAJOR * 
               sizeof *so_gendisk.flags);

        /* Initialize things (assuming 1 GB disk for now) */

        for (i=0; i<so_template.dev_max; i++) {
            so_blk_sizes[i] = 10*1048575;        /* dev size in 1K blocks - 1 */
            so_blksize_sizes[i] = SO_BLOCK_SIZE; /* block size */
            so_hardsizes[i] = SO_SECTOR_SIZE;    /* sector size */
            so_max_sectors[i] = 2048;            /* max sectors per I/O */
            so[i].nr_sects = 10*524288;          /* total number of sectors */
            so_sizes[i] = 10*524288;             /* total number of sectors */
        } 

        if (so_gendisk.next) {
            TRACE_ERROR("Why is this not NULL?\n");
            so_gendisk.next = NULL;
        }

        return 0;

    cleanup_so_gendisk_de_arr:
        kfree(so_gendisk.de_arr);
    cleanup_so:
        kfree(so);
    cleanup_so_sizes:
        kfree(so_sizes);
    cleanup_rscsi_osds:
        kfree(rscsi_osds);
    cleanup_so_max_sectors:
        kfree(so_max_sectors);
    cleanup_so_hardsizes:
        kfree(so_hardsizes);
    cleanup_so_blksize_sizes:
        kfree(so_blksize_sizes);
    cleanup_so_blk_sizes:
        kfree(so_blk_sizes);
    cleanup_devfs:
        devfs_unregister_blkdev(SCSI_OSD_MAJOR, "so");
        so_registered--;
        return 1;
    }

    static void so_finish()
    {
        struct gendisk *gendisk;
        int i;

        TRACE(TRACE_SO, 0, "so_finish()\n");

        blk_dev[SCSI_OSD_MAJOR].queue = so_find_queue;
        add_gendisk(&so_gendisk);

#if 0
        for (gendisk = gendisk_head; gendisk != NULL; gendisk = gendisk->next)
            if (gendisk == &so_gendisk)
                break;
        if (gendisk == NULL) {
            so_gendisk.next = gendisk_head;
            gendisk_head = &so_gendisk;
        }
#endif

        for (i = 0; i < so_template.dev_max; ++i)
            if (rscsi_osds[i].device) {
                register_disk(&so_gendisk, MKDEV(MAJOR(i),MINOR(i)), 
                              1, &so_fops, so_sizes[i]);
            }

        /* No read-ahead right now */

        read_ahead[SCSI_OSD_MAJOR] = 0;

        return;
    }

    static int so_detect(Scsi_Device * SDp)
    {
        char nbuff[6];
        TRACE(TRACE_SO, 0, "so_detect()\n");

        if (SDp->type != TYPE_OSD && SDp->type != TYPE_MOD)
            return 0;

        so_devname(so_template.dev_noticed++, nbuff);
        printk("Detected scsi %sosd %s at scsi%d, channel %d, id %d, lun %d\n",
               SDp->removable ? "removable " : "",
               nbuff,
               SDp->host->host_no, SDp->channel, SDp->id, SDp->lun);

        return 1;
    }

    static int so_attach(Scsi_Device * SDp)
    {
        unsigned int devnum;
        Scsi_Osd *dpnt;
        int i;

        TRACE(TRACE_SO, 0, "so_attach(SDpnt 0x%p, host_no %i)\n", 
              SDp, SDp->host->host_no);
        if (SDp->type != TYPE_OSD && SDp->type != TYPE_MOD)
            return 0;

        if (so_template.nr_dev >= so_template.dev_max) {
            SDp->attached--;
            return 1;
        }
        for (dpnt = rscsi_osds, i = 0; i < so_template.dev_max; i++, dpnt++)
            if (!dpnt->device)
                break;

        if (i >= so_template.dev_max)
            panic("scsi_devices corrupt (so)");

        rscsi_osds[i].device = SDp;
        so_template.nr_dev++;
        so_gendisk.nr_real++;
        devnum = i % SCSI_OSDS_PER_MAJOR;
        so_gendisk.de_arr[devnum] = SDp->de;
        if (SDp->removable)
            so_gendisk.flags[devnum] |= GENHD_FL_REMOVABLE;
        return 0;
    }

    static void so_detach(Scsi_Device * SDp)
    {
        Scsi_Osd *dpnt;
        int i, j;
        int max_p;
        int start;

        TRACE(TRACE_SO, 0, "so_detach()\n");

        for (dpnt = rscsi_osds, i = 0; i < so_template.dev_max; i++, dpnt++)
            if (dpnt->device == SDp) {

                /* If we are disconnecting a osd driver, sync and invalidate
                 * everything */
                max_p = so_gendisk.max_p;
                start = i << so_gendisk.minor_shift;

                for (j = max_p - 1; j >= 0; j--) {
                    int index = start + j;
                    //invalidate_device(MKDEV(MAJOR(index),MINOR(index)), 1);
                    so_gendisk.part[index].start_sect = 0;
                    so_gendisk.part[index].nr_sects = 0;
                    so_sizes[index] = 0;
                }
                devfs_register_partitions (&so_gendisk, MINOR(start), 1);
                /* unregister_disk() */
                dpnt->device = NULL;
                SDp->attached--;
                so_template.dev_noticed--;
                so_template.nr_dev--;
                so_gendisk.nr_real--;
                return;
            }
        return;
    }

    static int __init init_so(void)
    {
        TRACE(TRACE_SO, 0, "init_so()\n");
        so_template.module = THIS_MODULE;
        return scsi_register_module(MODULE_SCSI_DEV, &so_template);
    }

    static void __exit exit_so(void)
    {
        struct gendisk **prev_sogd_link;
        struct gendisk *sogd;
        int removed = 0;

        TRACE(TRACE_SO, 0, "exit_so()\n");

        scsi_unregister_module(MODULE_SCSI_DEV, &so_template);
        devfs_unregister_blkdev(SCSI_OSD_MAJOR, "so");
        so_registered--;
        if (rscsi_osds != NULL) {
            kfree(rscsi_osds);
            kfree(so_sizes);
            kfree(so_blk_sizes);
            kfree(so_blksize_sizes);
            kfree(so_hardsizes);
            kfree((char *) so);

#if 0
            /* Remove &so_gendisk from the linked list */
            prev_sogd_link = &gendisk_head;
            while ((sogd = *prev_sogd_link) != NULL) {
                if (sogd >= &so_gendisk && sogd <= &so_gendisk) {
                    removed++;
                    *prev_sogd_link = sogd->next;
                    continue;
                }
                prev_sogd_link = &sogd->next;
            }

            if (removed != 1)
                printk("%s %d &so_gendisk in osd chain", 
                       removed > 1 ? "total" : "just", removed);
#endif
        }
        del_gendisk(&so_gendisk);
        blksize_size[SCSI_OSD_MAJOR] = NULL;
        blk_size[SCSI_OSD_MAJOR] = NULL;
        hardsect_size[SCSI_OSD_MAJOR] = NULL;
        read_ahead[SCSI_OSD_MAJOR] = 0;
        so_template.dev_max = 0;
    }

#endif

    module_init(init_so);
    module_exit(exit_so);
