
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
 * Object-based Storage Device (OSD) Filesystem
 */

/*
 * Questions:
 * 1) which of these functions are re-entrant?
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/init.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/blkdev.h>
#include <scsi.h>
#include "osd.h"
#include "osd_ops.h"
#include "debug.h"
#include "config.h"

/*
 * Licensing
 */

MODULE_AUTHOR("Intel Corporation, <http://www.intel.com>");
MODULE_LICENSE("Dual BSD/GPL");


/*
 * Constants
 */


#define OSDFS_MAGIC  0xabcdef01
#define MAX_INODES   32768
#define MAX_NAME_LEN 1024

/*
 * Types
 */


typedef struct osdfs_link_t {
    char name[MAX_NAME_LEN];
    struct osdfs_link_t* next;
} osdfs_link_t;

typedef struct osdfs_inode_t {
    osdfs_link_t  *link;
} osdfs_inode_t;

typedef struct osdfs_metadata_t {
    uint64_t ObjectID;
    int      used;
} osdfs_metadata_t;


/*
 * Prototypes
 */

static int osdfs_mknod(struct inode *dir, struct dentry *dentry, int mode, 
                       int dev);
static int osdfs_get_block(struct inode *inode, long iblock, 
                           struct buffer_head *bh_result, int create);
static int entry_num(struct inode *dir);

/*
 * Globals
 */

static struct super_operations osdfs_ops;
static struct address_space_operations osdfs_aops;
static struct file_operations osdfs_dir_operations;
static struct file_operations osdfs_file_operations;
static struct inode_operations osdfs_dir_inode_operations;
static ISCSI_LOCK_T g_lock;
static struct inode create_attr;
static uint32_t g_gid = 0x1;
static uint64_t g_super_uid = 0x1;
static uint64_t g_root_uid  = 0x2;
static uint64_t g_next_uid  = 0xabc123;
static int g_no_format = 0; /* we format by default */
static int g_error;

/*
 * SCSI transport function for OSD
 */

int osd_exec_via_scsi(void *dev, osd_args_t *args, OSD_OPS_MEM_T *m) {
    Scsi_Request *SRpnt;
    Scsi_Device *SDpnt;
    unsigned char cdb[256];
    kdev_t kdev = *((kdev_t *) dev);
    void *ptr = NULL;
    int len = 0;

    TRACE(TRACE_OSDFS, 1, "op 0x%x\n", args->service_action);

    if (m->send_sg||m->recv_sg) {
        TRACE_ERROR("scatter/gather not yet implemented!\n");
        return -1;
    }

    SDpnt = blk_dev[MAJOR(kdev)].queue(kdev)->queuedata;
    SRpnt = scsi_allocate_request(SDpnt); 
    if (CONFIG_OSD_CDB_LEN > MAX_COMMAND_SIZE) {
        TRACE_ERROR("CONFIG_OSD_CDB_LEN (%i) > MAX_COMMAND_SIZE (%i)\n", 
                    CONFIG_OSD_CDB_LEN, MAX_COMMAND_SIZE);
        TRACE_ERROR("You must modify linux/drivers/scsi/scsi.h\n");
        TRACE_ERROR("and recompile your kernel\n");
        return -1;
    } 
    SRpnt->sr_cmd_len = CONFIG_OSD_CDB_LEN;
    SRpnt->sr_sense_buffer[0] = 0;
    SRpnt->sr_sense_buffer[2] = 0;
    switch(args->service_action) {
    case OSD_WRITE:
    case OSD_SET_ATTR:
        len = m->send_len;
        ptr = m->send_data;
        SRpnt->sr_data_direction = SCSI_DATA_WRITE;
        break;
    case OSD_CREATE:
        if (args->set_attributes_list_length) {
            SRpnt->sr_data_direction = SCSI_DATA_UNKNOWN;
#ifdef CONFIG_OSD_BIDI
            SRpnt->sr_bidi_bufflen = m->recv_len;
#endif
            if (m->recv_data != m->send_data) {
                TRACE_ERROR("send and recv buffers are not the same\n");
                return -1;
            }
            ptr = m->recv_data;
            len = m->recv_len>m->send_len?m->recv_len:m->send_len;
        } else {
            len = m->recv_len;
            ptr = m->recv_data;
            SRpnt->sr_data_direction = SCSI_DATA_READ;
        }
        break;
    case OSD_CREATE_GROUP:
    case OSD_READ:
    case OSD_GET_ATTR:
        len = m->recv_len;
        ptr = m->recv_data;
        SRpnt->sr_data_direction = SCSI_DATA_READ;
        break;
    case OSD_REMOVE:
    case OSD_REMOVE_GROUP:
        SRpnt->sr_data_direction = 0;
        break;
    default:
        TRACE_ERROR("unsupported OSD service action 0x%x\n", 
                    args->service_action);
        return -1;
    }

    OSD_ENCAP_CDB(args, cdb);

    // Exec SCSI command

    scsi_wait_req(SRpnt, cdb, ptr, len, 5*HZ, 5);
    if (SRpnt->sr_result!=0) {
        TRACE_ERROR("SCSI command failed (result %u)\n", SRpnt->sr_result);
        scsi_release_request(SRpnt);
        SRpnt = NULL;
        return -1;
    } 
    scsi_release_request(SRpnt);
    SRpnt = NULL;

    return 0;
}

/* 
 * Internal OSDFS functions
 */


/* Directory operations */

static inline unsigned long dir_pages(struct inode *inode) {
    return (inode->i_size+PAGE_CACHE_SIZE-1)>>PAGE_CACHE_SHIFT;
}

static inline void osdfs_put_page(struct page *page) {
    kunmap(page);
    page_cache_release(page);
}

static struct page * osdfs_get_page(struct inode *inode, unsigned long n) {
    struct address_space *mapping = inode->i_mapping;
    struct page *page = read_cache_page(mapping, n, 
                                        (filler_t*)mapping->a_ops->readpage, 
                                        NULL);

    TRACE(TRACE_OSDFS, 1, "osdfs_get_page(ino 0x%lx, block %lu, size %llu)\n",
          inode->i_ino, n, inode->i_size);

    if (!IS_ERR(page)) {
        wait_on_page(page);
        kmap(page);
        if (!Page_Uptodate(page))
            goto fail;
        if (!PageChecked(page)) {
            /* SetPageChecked(page); what does this do? */
        }
        if (PageError(page))
            goto fail;
    }
    return page;
 
 fail:
    osdfs_put_page(page);
    return ERR_PTR(-EIO);
}

static int entry_add(kdev_t dev, struct inode *dir_inode, ino_t dir_ino, 
                     ino_t entry_ino, const char *name, int mode) {
    unsigned long num_pages;
    struct page *page = NULL;
    unsigned offset;
    loff_t size = dir_inode->i_size;
    int len = strlen(name)+2+sizeof(ino_t);
    char *ptr;    

    TRACE(TRACE_OSDFS, 0, 
          "adding %s (len %i, ino 0x%lx) to 0x%lx (size %llu)\n", 
          name, len, entry_ino, dir_inode->i_ino, dir_inode->i_size);

    ISCSI_LOCK_ELSE(&g_lock, return -1);

    /* determine page and offset */
    num_pages = dir_pages(dir_inode);   
    offset = size%PAGE_CACHE_SIZE;
    if (offset+len>=PAGE_CACHE_SIZE) {
        TRACE(TRACE_OSDFS, 1, 
              "overflowing page %lu on entry %s (ino 0x%lx, size %llu, "
              "offset %u, len %u)\n", num_pages-1, name, 
              dir_inode->i_ino, size, offset, len);
        offset = 0;
    }
    if (offset == 0) num_pages++;

    /* get page */
    page = osdfs_get_page(dir_inode, num_pages-1);

    if (offset==0) {
        osdfs_get_block(dir_inode, page->index, page_address(page), 1);
        memset(page_address(page), 0, PAGE_CACHE_SIZE);
    }
    lock_page(page);

    /* add entry to end of page */
    ptr = page_address(page)+offset;    
    sprintf(ptr, "%s", name); 
    ptr += strlen(name); *ptr = '\0'; ptr++; 
    memcpy(ptr, &entry_ino, sizeof(ino_t)); ptr += sizeof(ino_t); *ptr = '\0';

    /* commit changes */
    dir_inode->i_mtime = dir_inode->i_ctime = CURRENT_TIME;
    if (S_ISDIR(mode)) {
        dir_inode->i_nlink++;
    }

    page->mapping->a_ops->prepare_write(NULL, page, offset, offset+len);
    page->mapping->a_ops->commit_write(NULL, page, offset, offset+len);
  
    if (IS_SYNC(dir_inode)) {
        if (writeout_one_page(page)!=0) {
            TRACE_ERROR("writeout_one_page() failed\n");
        }       
        if (waitfor_one_page(page)!=0) {
            TRACE_ERROR("writefor_one_page() failed\n");
        }
    }

    UnlockPage(page);
    osdfs_put_page(page);
    ISCSI_UNLOCK_ELSE(&g_lock, return -1);

    return 0;
}

static int entry_del(kdev_t dev, struct inode *dir, ino_t dir_ino, 
                     const char *name, int mode) {
    char *ptr, *limit;
    struct page *page;
    int i;
    int rec_len;

    TRACE(TRACE_OSDFS, 0, "deleting \"%s\" from 0x%lx\n", name, dir_ino);

    ISCSI_LOCK_ELSE(&g_lock, return -1);
    for (i=0; i<dir_pages(dir); i++) {
        page = osdfs_get_page(dir, i);
        lock_page(page);
        ptr = page_address(page);
        limit = ptr+PAGE_CACHE_SIZE;
        while ((ptr<limit)&&(*ptr)) {
            if (!strcmp(ptr, name)) {
                rec_len = strlen(ptr)+2+sizeof(ino_t);          
                TRACE(TRACE_OSDFS, 1, "found \"%s\" in page %i (rec_len %u)\n",
                      name, i, rec_len);
                memcpy(ptr, ptr+rec_len, limit-ptr-rec_len);
                memset(limit-rec_len, 0, rec_len);

                /* commit changes */

                dir->i_mtime = dir->i_ctime = CURRENT_TIME;
                if (i==(dir_pages(dir)-1)) { 
                    dir->i_size -= rec_len;
                    page->mapping->a_ops->commit_write(NULL, page, 0, 
                                                       dir->
                                                       i_size%PAGE_CACHE_SIZE);
                } else {
                    page->mapping->a_ops->commit_write(NULL, page, 0, 
                                                       PAGE_CACHE_SIZE);
                }

                if (IS_SYNC(dir)) {
                    if (writeout_one_page(page)!=0) {
                        TRACE_ERROR("writeout_one_page() failed\n");
                    }
                    if (waitfor_one_page(page)!=0) {
                        TRACE_ERROR("writefor_one_page() failed\n");
                    }
                }

                UnlockPage(page);
                osdfs_put_page(page);

                dir->i_mtime = dir->i_ctime = CURRENT_TIME;
                if (S_ISDIR(mode)) {
                    dir->i_nlink--;
                }
                mark_inode_dirty(dir);

                ISCSI_UNLOCK_ELSE(&g_lock, return -1);
                return 0;
            }
            ptr += strlen(ptr)+1; /* skip over name */
            ptr += sizeof(ino_t)+1; /* skip over ino */
        }
        UnlockPage(page);
        osdfs_put_page(page);
    }

    TRACE(TRACE_OSDFS, 1, "\"%s\" not found\n", name);
    ISCSI_UNLOCK_ELSE(&g_lock, return -1);

    return -1;
}

static int entry_num(struct inode *dir) {
    char *ptr, *limit;
    struct page *page;
    int i;
    int num = 0;

    ISCSI_LOCK_ELSE(&g_lock, return -1);
    for (i=0; i<dir_pages(dir); i++) {
        page = osdfs_get_page(dir, i);
        lock_page(page);
        ptr = page_address(page);
        limit = ptr+PAGE_CACHE_SIZE;
        while ((ptr<limit)&&(*ptr)) {
            ptr += strlen(ptr)+2+sizeof(ino_t);
            num++;
        }
        UnlockPage(page);
        osdfs_put_page(page);
    }
    ISCSI_UNLOCK_ELSE(&g_lock, return -1);

    return num;
}

/* Inode operations */

static void osdfs_set_ops(struct inode *inode) {
    switch (inode->i_mode & S_IFMT) {
    case S_IFREG:
        inode->i_fop = &osdfs_file_operations;
        break;
    case S_IFDIR:
        inode->i_op = &osdfs_dir_inode_operations;
        inode->i_fop = &osdfs_dir_operations;
        break;
    case S_IFLNK:
        inode->i_op = &page_symlink_inode_operations;
        break;
    default:
        TRACE_ERROR("UNKNOWN MODE\n");
    }
    inode->i_mapping->a_ops = &osdfs_aops;
}

static struct inode *osdfs_get_inode(struct super_block *sb, int mode, 
                                     int dev, const char *name, 
                                     uint64_t ObjectID) {
    struct inode *inode;
    ino_t ino = ObjectID;

    TRACE(TRACE_OSDFS, 0, "osdfs_get_inode(\"%s\", mode %i (%s))\n", name, mode,
          S_ISDIR(mode)?"DIR":(S_ISREG(mode)?"REG":"LNK"));

    // iget() gets a free VFS inode and subsequently call 
    // osdfds_read_inode() to fill the inode structure

    if ((inode=iget(sb, ino))==NULL) {
        TRACE_ERROR("iget() failed\n");
        return NULL;
    }

    return inode;
}

/*
 * Super Operations
 */

static void osdfs_read_inode(struct inode *inode) {
    ino_t ino = inode->i_ino;
    kdev_t dev = inode->i_sb->s_dev;
    uint64_t uid = ino;
    void *attr;
    uint16_t len;

    TRACE(TRACE_OSDFS, 0, "osdfs_read_inode(ino 0x%x, major %i, minor %i)\n",
          (unsigned) ino, MAJOR(dev), MINOR(dev));

    /* get object attributes for rest of inode */
    if (ino == g_next_uid) {
        attr = (void *) &create_attr;
    } else {
        if ((attr=iscsi_malloc_atomic(INODE_LEN))==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        }
        if (osd_get_one_attr((void *)&dev, g_gid, uid, 0x30000000, 0x0, 
                             INODE_LEN, &osd_exec_via_scsi, &len, attr)!=0) {
            TRACE_ERROR("osd_get_one_attr() failed\n");
            return;
        }
    }
 
    /* set inode attributes */
    inode->i_size  = ((struct inode *) attr)->i_size;
    inode->i_mode  = ((struct inode *) attr)->i_mode;
    inode->i_nlink = ((struct inode *) attr)->i_nlink;
    inode->i_gid   = ((struct inode *) attr)->i_gid;
    inode->i_uid   = ((struct inode *) attr)->i_uid;
    inode->i_ctime = ((struct inode *) attr)->i_ctime;
    inode->i_atime = ((struct inode *) attr)->i_atime;
    inode->i_mtime = ((struct inode *) attr)->i_mtime;

    /* only used by root inode to store next uid */
    inode->u.generic_ip = ((struct inode *)attr)->u.generic_ip; 

    if (ino != g_next_uid) {
        iscsi_free_atomic(attr);
    }

    osdfs_set_ops(inode);
}

void osdfs_write_inode(struct inode *inode, int sync) {
    ino_t ino = inode->i_ino;
    kdev_t dev = inode->i_sb->s_dev;
    uint64_t uid = ino;
    char buffer[INODE_LEN];

    memset(buffer, 0, INODE_LEN);
    memcpy(buffer, inode, sizeof(struct inode));
    if (sync||!IS_SYNC(inode)) {
        TRACE(TRACE_OSDFS, 0, 
              "osdfs_write_inode(0x%llx, sync %i, IS_SYNC(inode) %i, "
              "state %lu)\n",         
              uid, sync, IS_SYNC(inode), inode->i_state);
        if (osd_set_one_attr((void *)&dev, g_gid, uid, 0x30000000, 0x1, 
                             INODE_LEN, buffer, &osd_exec_via_scsi)!=0) {
            TRACE_ERROR("osd_set_one_attr() failed\n");
            g_error = 1;
        }
    } else {
        TRACE(TRACE_OSDFS, 0, 
              "osdfs_write_inode(0x%llx, sync %i, IS_SYNC(inode) %i, "
              "state %lu) - IGNORING\n",              
              uid, sync, IS_SYNC(inode), inode->i_state);
    }
}

void osdfs_dirty_inode(struct inode *inode) {
    TRACE(TRACE_OSDFS, 0, 
          "osdfs_dirty_inode(ino 0x%x, IS_SYNC(inode) %i, state %lu)\n", 
          (unsigned) inode->i_ino, IS_SYNC(inode), inode->i_state);
    if (IS_SYNC(inode)) {
        osdfs_write_inode(inode, IS_SYNC(inode));
        inode->i_state &= ~I_DIRTY;
    }
}

void osdfs_put_inode(struct inode *inode) {
    TRACE(TRACE_OSDFS, 0, "osdfs_put_inode(0x%x), count %i\n", 
          (unsigned) inode->i_ino, atomic_read(&inode->i_count));
}

void osdfs_delete_inode(struct inode *inode) {
    TRACE(TRACE_OSDFS, 0, "osdfs_delete_inode(ino 0x%lx state %lu, count %i)\n",
          inode->i_ino, inode->i_state, atomic_read(&inode->i_count));
    atomic_dec(&inode->i_count);
    clear_inode(inode);
}

void osdfs_put_super(struct super_block *sb) {
    if (osd_write(&sb->s_dev, g_gid, g_super_uid, 0, 8, &g_next_uid, 0, 
                  &osd_exec_via_scsi) != 0) {
        TRACE_ERROR("osd_write() failed for super object\n");   
    }
}

void osdfs_write_super(struct super_block *sb) {
    TRACE_ERROR("osdfs_write_super() not implemented\n");
}

void osdfs_write_super_lockfs(struct super_block *sb) {
    TRACE_ERROR("osdfs_write_super_lockfs() not implemented\n");
}

void osdfs_unlockfs(struct super_block *sb) {
    TRACE_ERROR("osdfs_unlockfs() not implemented\n");
}

int osdfs_statfs(struct super_block *sb, struct statfs *buff) {
    TRACE(TRACE_OSDFS, 0, "statfs()\n");
    buff->f_type    = OSDFS_MAGIC;
    buff->f_bsize   = PAGE_CACHE_SIZE;
    buff->f_blocks  = 256;
    buff->f_bfree   = 128;
    buff->f_bavail  = 64;
    buff->f_files   = 0;
    buff->f_ffree   = 0;
    buff->f_namelen = MAX_NAME_LEN;

    return 0;
}

int osdfs_remount_fs(struct super_block *sb, int *i, char *c) {
    TRACE_ERROR("osdfs_remount_fs() not implemented\n");

    return -1;
}

void osdfs_clear_inode(struct inode *inode) {
    TRACE(TRACE_OSDFS, 0, "osdfs_clear_inode(ino 0x%lx)\n", inode->i_ino);
}

void osdfs_umount_begin(struct super_block *sb) {
    TRACE_ERROR("osdfs_unmount_begin() not implemented\n");
}    

static struct super_operations osdfs_ops = {
 put_inode: osdfs_put_inode,
 delete_inode: osdfs_delete_inode,
 clear_inode: osdfs_clear_inode,
 put_super: osdfs_put_super,
 dirty_inode: osdfs_dirty_inode,
 read_inode: osdfs_read_inode,
 write_inode: osdfs_write_inode,
 write_super: osdfs_write_super,
 write_super_lockfs: osdfs_write_super_lockfs,
 unlockfs: osdfs_unlockfs,
 statfs: osdfs_statfs,
 remount_fs: osdfs_remount_fs,
 umount_begin: osdfs_umount_begin
};


/*
 * Inode operations for directories
 */


static int osdfs_create(struct inode *dir, struct dentry *dentry, int mode) {

    if (g_error) return -1;

    TRACE(TRACE_OSDFS, 0, "osdfs_create(\"%s\")\n", dentry->d_name.name);
    if (osdfs_mknod(dir, dentry, mode | S_IFREG, 0)!=0) {
        TRACE_ERROR("osdfs_mknod() failed\n");
        return -1;
    } 
    TRACE(TRACE_OSDFS, 1, "file \"%s\" is inode 0x%x\n", 
          dentry->d_name.name, (unsigned) dentry->d_inode->i_ino);

    return 0;
}

static struct dentry * osdfs_lookup(struct inode *dir, struct dentry *dentry) {
    const char *name = dentry->d_name.name;
    struct inode *inode = NULL;
    struct page *page;
    char *ptr, *limit; 
    ino_t ino;
    int i;

    TRACE(TRACE_OSDFS, 0, "osdfs_lookup(\"%s\" in dir ino %lu)\n", 
          name, dir->i_ino);

    /* The vfs caches directory entries that have been successfully looked
     * up, so any additional lookups on them will not affect the atime of
     * the directory */

    if (!IS_NOATIME(dir)&&!IS_NODIRATIME(dir)) {
        dir->i_atime = CURRENT_TIME;
        mark_inode_dirty(dir);
    }

    for (i=dir_pages(dir)-1; i>=0; i--) {
        page = osdfs_get_page(dir, i);
        lock_page(page);
        ptr = page_address(page);
        limit = ptr+PAGE_CACHE_SIZE;
        while ((ptr<limit)&&(*ptr)) {
            if (!strcmp(ptr, name)) {
                ptr += strlen(ptr)+1; /* skip over name */
                memcpy(&ino, ptr, sizeof(ino_t));
                if ((inode=iget(dir->i_sb, ino))==NULL) {
                    TRACE_ERROR("iget() failed\n");
                    goto error;
                }
                TRACE(TRACE_OSDFS, 1, "found \"%s\" (ino 0x%lx)\n", name, ino);
                UnlockPage(page);
                osdfs_put_page(page);
                d_add(dentry, inode);
                return NULL;
            }
            ptr += strlen(ptr)+1; /* skip over name */
            ptr += sizeof(ino_t)+1; /* skip over ino */
        }
        UnlockPage(page);
        osdfs_put_page(page);
    }
    TRACE(TRACE_OSDFS, 1, "\"%s\" not found\n", name);
    return NULL;

 error:
    UnlockPage(page);
    osdfs_put_page(page);
    return NULL;
}

static int osdfs_link(struct dentry *old_dentry, struct inode * dir, 
                      struct dentry * dentry) {
    struct inode *inode = old_dentry->d_inode;
    kdev_t dev = dir->i_sb->s_dev;
    ino_t dir_ino = dir->i_ino;
    ino_t ino = inode->i_ino;
    const char *name = dentry->d_name.name;

    TRACE(TRACE_OSDFS, 0, "osdfs_link(%lu, \"%s\")\n", ino, name);

    if (S_ISDIR(inode->i_mode)) return -EPERM;
    if (entry_add(dev, dir, dir_ino, ino, name, dentry->d_inode->i_mode)!=0) {
        TRACE_ERROR("entry_add() failed\n");
        return -1;
    }
    inode->i_ctime = CURRENT_TIME;
    inode->i_nlink++;
    atomic_inc(&inode->i_count); 
    mark_inode_dirty(inode);
    d_instantiate(dentry, inode);

    return 0; 
}

static int osdfs_unlink(struct inode * dir, struct dentry *dentry) {
    kdev_t dev = dir->i_sb->s_dev;
    struct inode *inode = dentry->d_inode;
    ino_t dir_ino = dir->i_ino;
    ino_t ino = dentry->d_inode->i_ino;
    const char *name = dentry->d_name.name;
    int rc;

    TRACE(TRACE_OSDFS, 0, "osdfs_unlink(\"%s\", ino 0x%x)\n", 
          name, (unsigned) ino);
    switch (inode->i_mode & S_IFMT) {
    case S_IFREG:
    case S_IFLNK:
        break;
    case S_IFDIR:
        if ((rc=entry_num(inode))) {
            TRACE_ERROR("directory 0x%x still has %i entries\n", 
                        (unsigned) ino, rc);
            return -ENOTEMPTY;
        }
        inode->i_nlink--;
        break;
    }

    if (entry_del(dev, dir, dir_ino, name, dentry->d_inode->i_mode)!=0) {
        TRACE_ERROR("entry_del() failed\n");
        return -1;
    }

    if (--inode->i_nlink) {
        TRACE(TRACE_OSDFS, 1, "ino 0x%x still has %i links\n", 
              (unsigned) ino, inode->i_nlink);
        inode->i_ctime = dir->i_ctime;
        mark_inode_dirty(inode);
    } else {
        TRACE(TRACE_OSDFS, 1, 
              "ino 0x%x link count reached 0, removing object\n", 
              (unsigned) ino);
        if (osd_remove((void *)&dev, g_gid, ino, &osd_exec_via_scsi)!=0) {
            TRACE_ERROR("osd_remove() failed\n");
            return -1;
        }
    }

    return 0;
}

static int osdfs_symlink(struct inode * dir, struct dentry *dentry, 
                         const char * symname) {
    struct inode *inode;

    TRACE(TRACE_OSDFS, 0, "osdfs_symlink(\"%s\"->\"%s\")\n", 
          dentry->d_name.name, symname);
    if (osdfs_mknod(dir, dentry,  S_IRWXUGO | S_IFLNK, 0)!=0) {
        TRACE_ERROR("osdfs_mknod() failed\n");
        return -1;
    } 
    inode = dentry->d_inode;
    if (block_symlink(inode, symname, strlen(symname)+1)!=0) {
        TRACE_ERROR("block_symlink() failed\n");
        return -1;
    }
    TRACE(TRACE_OSDFS, 1, "symbolic link \"%s\" is inode %lx\n", 
          dentry->d_name.name, inode->i_ino);

    return 0; 
}

static int osdfs_mkdir(struct inode * dir, struct dentry * dentry, int mode) {

    TRACE(TRACE_OSDFS, 0, "osdfs_mkdir(\"%s\")\n", dentry->d_name.name);
    if (osdfs_mknod(dir, dentry, mode | S_IFDIR, 0)!=0) {
        TRACE_ERROR("osdfs_mkdir() failed\n");
    } 

    return 0;
}

static int osdfs_mknod(struct inode *dir, struct dentry *dentry, int mode, 
                       int dev_in) {
    struct inode *inode = NULL;
    kdev_t dev = dir->i_sb->s_dev;
    const char *name = dentry->d_name.name;

    TRACE(TRACE_OSDFS, 0, "osdfs_mknod(\"%s\", sync %i)\n", 
          dentry->d_name.name, IS_SYNC(dir));

    /* Create object and initialize attributes */

    memset(&create_attr, 0, sizeof(struct inode));
    create_attr.i_mode = mode;
    create_attr.i_uid = current->fsuid;
    create_attr.i_gid = current->fsgid;
    create_attr.i_ctime = create_attr.i_atime = create_attr.i_mtime = 
        CURRENT_TIME;
    if ((mode & S_IFMT) == S_IFDIR) {
        create_attr.i_nlink = 2;
    } else {
        create_attr.i_nlink = 1;
    }

    /* Assign to an inode */

    if ((inode = osdfs_get_inode(dir->i_sb, mode, dev, name, g_next_uid))
        ==NULL) {
        TRACE_ERROR("osdfs_get_inode() failed\n");
        ISCSI_UNLOCK_ELSE(&g_lock, return -1);
        return -ENOSPC;
    }
    d_instantiate(dentry, inode);

    /* create object */

    if (IS_SYNC(dir)) {
        char buffer[INODE_LEN];

        memset(buffer, 0, INODE_LEN);
        memcpy(buffer, &create_attr, sizeof(struct inode));
        if (osd_create_and_set((void *)&dev, g_gid, &osd_exec_via_scsi, 
                               &g_next_uid, 
                               0x30000000, 0x1, INODE_LEN, buffer)!=0) {
            TRACE_ERROR("osd_create_and_set() failed\n");
            ISCSI_UNLOCK_ELSE(&g_lock, return -1);
            return -1;
        }
    } else {
        mark_inode_dirty(inode); /* needed for empty files */
    }

    /* Add entry to parent directory */

    if (inode->i_ino != 1) {    
        if (entry_add(dev, dir, dir->i_ino, inode->i_ino, name, inode->i_mode)
            !=0) {
            TRACE_ERROR("entry_add() failed\n");
            ISCSI_UNLOCK_ELSE(&g_lock, return -1);
            return -1;
        }
    }

    g_next_uid++;

    return 0;
}

static int osdfs_rename(struct inode *old_dir, struct dentry *old_dentry, 
                        struct inode *new_dir, 
                        struct dentry *new_dentry) {
    kdev_t dev = old_dir->i_sb->s_dev;
    ino_t old_dir_ino =  old_dir->i_ino;
    ino_t new_dir_ino = new_dir->i_ino;
    ino_t old_ino = old_dentry->d_inode->i_ino;
    ino_t new_ino;
    const char *old_name = old_dentry->d_name.name;
    const char *new_name = new_dentry->d_name.name;

    new_ino = new_dentry->d_inode?new_dentry->d_inode->i_ino:old_ino;

    TRACE(TRACE_OSDFS, 0, 
          "rename %s (0x%lx) in 0x%lx to %s (0x%lx) in 0x%lx\n", 
          old_name, old_ino, old_dir_ino, new_name, new_ino, new_dir_ino);

    /* Delete old entry from old directory */

    if (entry_del(dev, old_dir, old_dir_ino, old_name, 
                  old_dentry->d_inode->i_mode)!=0) {
        TRACE_ERROR("error deleting old entry \"%s\"\n", old_name);
        return -1;
    }

    /* Unlink new entry from new directory */

    if (new_dentry->d_inode) {
        if (osdfs_unlink(new_dir, new_dentry)!=0) {
            TRACE_ERROR("osdfs_unlink() failed\n");
            return -1;
        }
    } 

    /* Add new entry to new directory (might be the same dir) */

    if (entry_add(dev, new_dir, new_dir_ino, old_ino, new_name, 
                  old_dentry->d_inode->i_mode)!=0) {
        TRACE_ERROR("error adding new entry \"%s\"\n", new_name);
        return -1;
    }

    return 0;
}

static struct inode_operations osdfs_dir_inode_operations = {
 create:   osdfs_create,
 lookup:   osdfs_lookup,
 link:     osdfs_link,
 unlink:   osdfs_unlink,
 symlink:  osdfs_symlink,
 mkdir:    osdfs_mkdir,
 rmdir:    osdfs_unlink,
 mknod:    osdfs_mknod,
 rename:   osdfs_rename,
};


/*
 * File operations (regular files)
 */

int osdfs_sync_inode (struct inode *inode)
{
    osdfs_write_inode (inode, 1);
    return 0;
}

int osdfs_fsync_inode(struct inode *inode, int datasync)
{
    int err;
        
    err  = fsync_inode_buffers(inode);
    err |= fsync_inode_data_buffers(inode);
    if (!(inode->i_state & I_DIRTY))
        return err;
    if (datasync && !(inode->i_state & I_DIRTY_DATASYNC))
        return err;
    err |= osdfs_sync_inode(inode);
    return err ? -EIO : 0;
}

int osdfs_sync_file(struct file * file, struct dentry *dentry, int datasync)
{
    struct inode *inode = dentry->d_inode;
    return osdfs_fsync_inode(inode, datasync);
}

static struct file_operations osdfs_file_operations = {
 read:          generic_file_read,
 write:         generic_file_write,
 mmap:          generic_file_mmap,
 fsync:         osdfs_sync_file,
};

/*
 * File operations (directories)
 */

static int osdfs_readdir(struct file *filp, void *dirent, filldir_t filldir) {
    struct dentry *dentry = filp->f_dentry;
    ino_t ino = dentry->d_inode->i_ino;
    int offset = filp->f_pos;
    char *ptr, *limit;
    struct page *page;
    int i, j;
    int filled = 0;
    int num_entries = entry_num(dentry->d_inode);

    TRACE(TRACE_OSDFS, 0, 
          "osdfs_readdir(\"%s\", ino 0x%x, offset %i, size %llu, state %li)\n", 
          dentry->d_name.name, (unsigned) ino, offset, 
          dentry->d_inode->i_size, dentry->d_inode->i_state);

    if (offset) {
        if (((int)filp->private_data)>num_entries) {
            filp->f_pos = offset -= (((int)filp->private_data)-num_entries);
            filp->private_data = (void *) num_entries;
        }
    } else {
        filp->private_data = (void *) num_entries;
    }

    switch (offset) {

    case 0:
        TRACE(TRACE_OSDFS, 1, "adding \".\" (ino 0x%x)\n", (unsigned) ino);
        if (filldir(dirent, ".", 1, filp->f_pos++, ino, DT_DIR) < 0) {
            TRACE_ERROR("filldir() failed for \".\"??\n");
            return -1;
        }
        filled = 1;
 
    case 1:
        TRACE(TRACE_OSDFS, 1, "adding \"..\" (ino 0x%x)\n", 
              (unsigned) dentry->d_parent->d_inode->i_ino);
        if (filldir(dirent, "..", 2, filp->f_pos++, 
                    dentry->d_parent->d_inode->i_ino, DT_DIR) < 0) {
            TRACE_ERROR("filldir() failed for \"..\"??\n");
            return -1;
        }
        filled = 1;

    default:
        offset -= 2;
        j = 0;
        for (i=0; i<dir_pages(dentry->d_inode); i++) {
            page = osdfs_get_page(dentry->d_inode, i);
            lock_page(page);
            ptr = page_address(page);
            limit = ptr + 
                (i==(dir_pages(dentry->d_inode)-1)?
                 MIN(dentry->d_inode->i_size%PAGE_CACHE_SIZE,PAGE_CACHE_SIZE):
                 PAGE_CACHE_SIZE);
            while ((ptr<limit)&&(*ptr)) {
                if (offset>0) {
                    offset--;
                } else {
                    memcpy(&ino, ptr+strlen(ptr)+1, sizeof(ino_t));
                    if ((ino>0xfff123)||(ino<0xabc123)) {
                        TRACE_ERROR("directory %s (0x%lx) corrupt (page %i, \
entry \"%s\" -> ino 0x%lx)\n", 
                                    dentry->d_name.name, 
                                    dentry->d_inode->i_ino, i, ptr, ino); 
                        UnlockPage(page);
                        return -1;
                    }
                    TRACE(TRACE_OSDFS, 1, "%i: adding \"%s\" (ino 0x%x)\n", 
                          j++, ptr, (unsigned) ino);
                    if (filldir(dirent, ptr, strlen(ptr), filp->f_pos++, 
                                ino, DT_UNKNOWN) < 0) {
                        filp->f_pos--;
                        UnlockPage(page);
                        osdfs_put_page(page);
                        dentry->d_inode->i_atime = CURRENT_TIME;
                        if (!IS_NOATIME(dentry->d_inode)&&
                            !IS_NODIRATIME(dentry->d_inode)) {
                            mark_inode_dirty(dentry->d_inode);
                        }
                        return 0;
                    }
                    filled = 1;

                }               
                ptr += strlen(ptr)+2+sizeof(ino_t); /* skip to next record */
            }
            UnlockPage(page);
            osdfs_put_page(page);
        }          
    }

    /* The vfs is done reading the directory when the offset is greater
     * than the number of directory entries (no entries are filled). It
     * is then that we update the atime. */

    if (!filled) {
        if (!IS_NOATIME(dentry->d_inode)&&!IS_NODIRATIME(dentry->d_inode)) {
            dentry->d_inode->i_atime = CURRENT_TIME;
            mark_inode_dirty(dentry->d_inode);
        }
    }

    return 0;
}

static struct file_operations osdfs_dir_operations = {
 read: generic_read_dir,
 readdir: osdfs_readdir,
 fsync: osdfs_sync_file,
};


/*
 * Address space operations
 */

static int osdfs_get_block(struct inode *inode, long iblock, 
                           struct buffer_head *bh_result, int create) {  

    if (g_error) return -1;

    TRACE(TRACE_OSDFS, 0, 
          "osdfs_get_block(ino 0x%lx, iblock %lu, create %u (state %lx)\n", 
          inode->i_ino, iblock, create, bh_result->b_state);

    bh_result->b_dev = inode->i_dev;
    bh_result->b_blocknr = iblock;
    bh_result->b_state |= (1UL << BH_Mapped);
    if (create) {
        bh_result->b_state |= (1UL << BH_New);
    }
    return 0;
}

static int osdfs_readpage(struct file *file, struct page *page) {
    uint64_t Offset = page->index<<PAGE_CACHE_SHIFT;
    uint64_t Length = 1<<PAGE_CACHE_SHIFT;
    struct inode *inode = page->mapping->host;
    ino_t ino = inode->i_ino;
    int rc;

    if (g_error) return -1;

    Offset +=0; Length +=0; ino +=0;
 
    TRACE(TRACE_OSDFS, 0, 
          "osdfs_readpage(page %p, ino 0x%lx, index %li, Offset %llu, "
          "Length %llu, size %llu)\n", page, ino, page->index, Offset, 
          Length, inode->i_size);
    
    if (!IS_NOATIME(inode)&&!IS_NODIRATIME(inode)) {
        inode->i_atime = CURRENT_TIME;
        mark_inode_dirty(inode);
    }

    return block_read_full_page(page, osdfs_get_block);

    return rc;
}

static int osdfs_writepage(struct page *page) {
    uint64_t Offset = page->index<<PAGE_CACHE_SHIFT;
    uint64_t Length = 1<<PAGE_CACHE_SHIFT;
    struct inode *inode = page->mapping->host;
    ino_t ino = inode->i_ino;

    TRACE_ERROR("OSDFS_WRITEPAGE...\n");

    if (g_error) return -1;

    Offset +=0; Length +=0; ino +=0;

    TRACE(TRACE_OSDFS, 0, 
          "osdfs_writepage(ino 0x%lx, index %li, Offset %llu, Length %llu)\n", 
          ino, page->index, Offset, Length);
    return block_write_full_page(page,osdfs_get_block);

    return 0;
}

static int osdfs_prepare_write(struct file *file, struct page *page, 
                               unsigned from, unsigned to) {
    if (g_error) return -1;
    TRACE(TRACE_OSDFS, 0, 
          "osdfs_prepare_write(ino 0x%lx, page %p, page index %li, offset %u, "
          "to %u)\n", page->mapping->host->i_ino, page, page->index, from, to);
    return block_prepare_write(page, from, to, osdfs_get_block);
}

static int osdfs_bmap(struct address_space *mapping, long block) {
    if (g_error) return -1;
    TRACE(TRACE_OSDFS, 0, "osdfs_bmap(block %li)\n", block);
    return generic_block_bmap(mapping,block, osdfs_get_block);
}

static int osdfs_direct_IO(int rw, struct inode * inode, struct kiobuf * iobuf,
                           unsigned long blocknr, int blocksize) {
    if (g_error) return -1;
    TRACE(TRACE_OSDFS, 0, "osdfs_direct_IO(block %li)\n", blocknr);
    return generic_direct_IO(rw, inode, iobuf, blocknr, blocksize, 
                             osdfs_get_block);
}

static struct address_space_operations osdfs_aops = {
 prepare_write: osdfs_prepare_write,
 readpage: osdfs_readpage,
 writepage: osdfs_writepage,
 sync_page: block_sync_page,
 commit_write: generic_commit_write,
 bmap: osdfs_bmap,
 /* direct_IO: osdfs_direct_IO, */
};


/*
 * Superblock operations
 */


static struct super_block *osdfs_read_super(struct super_block *sb, 
                                            void *data, int silent) {
    char opt[64];
    char *ptr, *ptr2;
    struct inode attr;
    struct inode *inode;

    TRACE(TRACE_OSDFS, 0, 
          "osdfs_read_super(sb %p, s_root %p, major %i minor %i)\n", 
          sb, sb->s_root, MAJOR(sb->s_dev),  MINOR(sb->s_dev)); 

    sb->u.generic_sbp = &g_gid; /* used by the so driver */

    /* Parse options */

    ptr = (char *)data;
    while (ptr&&strlen(ptr)) {
        if ((ptr2=strchr(ptr, ','))) {
            strncpy(opt, ptr, ptr2-ptr);
            opt[ptr2-ptr] = '\0';
            ptr = ptr2+1;
        } else {
            strcpy(opt, ptr);
            ptr = '\0';
        } 
        if (!strncmp(opt, "no_format", 9)) {
            g_no_format = 1;
        } else {
            TRACE_ERROR("unknown option \"%s\"\n", opt);
            return NULL;
        }
    }
 
    /* Initialize superblock */
    sb->s_blocksize      = PAGE_CACHE_SIZE;
    sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
    sb->s_magic          = OSDFS_MAGIC;
    sb->s_op             = &osdfs_ops;

    /* create group object for entire  and user object for root inode */

    if (!g_no_format) {
        char buffer[INODE_LEN];

        /* group object for entire fs */
        if (osd_create_group((void *)&sb->s_dev, &osd_exec_via_scsi, 
                             &g_gid)!=0) {
            TRACE_ERROR("osd_create_group() failed\n");
            return NULL;
        }

        /* super object is 0x1 */
        if (osd_create((void *)&sb->s_dev, g_gid, &osd_exec_via_scsi, 
                       &g_super_uid)!=0) {
            TRACE_ERROR("osd_create() failed\n");
            return NULL;
        }

        /* root directory object is 0x2 */
        if (osd_create((void *)&sb->s_dev, g_gid, &osd_exec_via_scsi, 
                       &g_root_uid)!=0) {
            TRACE_ERROR("osd_create() failed\n");
            return NULL;
        }
        memset(&attr, 0, sizeof(struct inode));
        attr.i_mode = S_IFDIR | 0755;
        attr.i_atime = attr.i_ctime = attr.i_mtime = CURRENT_TIME;
        attr.i_nlink = 2;
        attr.u.generic_ip = (void *) 0xff00;

        memset(buffer, 0, INODE_LEN);
        memcpy(buffer, &attr, sizeof(struct inode));
        if (osd_set_one_attr((void *)&sb->s_dev, g_gid, g_root_uid, 
                             0x30000000, 0x1, INODE_LEN, 
                             (void *) buffer, &osd_exec_via_scsi)!=0) {
            TRACE_ERROR("osd_set_one_attr() failed\n");
            return NULL;
        }
    } else {
        if (osd_read(&sb->s_dev, g_gid, g_super_uid, 0, 8, &g_next_uid, 
                     0, &osd_exec_via_scsi) != 0) {
            TRACE_ERROR("osd_read() failed for super object\n");           
            return NULL;
        }
    }

    /* create root inode */
    
    if ((inode=osdfs_get_inode(sb, S_IFDIR | 0755, 0, "/", g_root_uid))==NULL) {
        TRACE_ERROR("osdfs_get_inode() failed\n");
        return NULL;
    }

    TRACE(TRACE_OSDFS, 0, "SET NEXT UID TO 0x%llx\n", g_next_uid);  

    if ((sb->s_root=d_alloc_root(inode))==NULL) {
        TRACE_ERROR("d_alloc_root() failed\n");
        iput(inode);
        return NULL;
    }

    return sb;
}

static DECLARE_FSTYPE_DEV(osdfs_fs_type, "osdfs", osdfs_read_super);

/*
 * Module operations
 */

static int __init init_osdfs_fs(void) {
    TRACE(TRACE_OSDFS, 0, "init_osdfs_fs()\n");
    ISCSI_LOCK_INIT_ELSE(&g_lock, return -1);
    return register_filesystem(&osdfs_fs_type);
}

static void __exit exit_osdfs_fs(void) {
    ISCSI_LOCK_DESTROY_ELSE(&g_lock, printk("failed to destroy g_lock\n"));
    unregister_filesystem(&osdfs_fs_type);
}

module_init(init_osdfs_fs)
module_exit(exit_osdfs_fs)

#if 0
if (!list_empty(&dir_inode->i_sb->s_dirty)) {
    struct list_head *head = &(dir_inode->i_sb->s_dirty);
    struct list_head *tmp = head;
    struct inode *inode;
    do {
        inode = list_entry(tmp, struct inode, i_list);
        PRINT("0x%lx -> ", inode->i_ino);
        tmp = tmp->next;
    } while (tmp != head);
    PRINT("\n");
 } else {
    PRINT("dirty list is empty\n");
 }
#endif
