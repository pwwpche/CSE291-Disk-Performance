
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
#include <sys/vfs.h>
#include <utime.h>
#include <scsi/scsi.h>
#include <inttypes.h>
#include "iscsi.h"
#include "util.h"
#include "device.h"
#include "osd.h"

#define CONFIG_OSD_NUM_LUNS_DFLT   1
#define CONFIG_OSD_MAX_LUNS        8
#define CONFIG_LUN_QUEUE_DEPTH     256
#define CONFIG_LUN_SIZE 2047*1048576
#define CONFIG_OSD_WORKERS_PER_LUN 8

#define OSD_INITIALIZED   1
#define OSD_SHUTTING_DOWN 2

/*
 * Types
 */

typedef struct lun_worker_args_t {    
    uint64_t lun;
} LUN_WORKER_ARGS_T;


/*
 * Globals
 */

static unsigned osd_num_luns = CONFIG_OSD_LUNS_DFLT;
static char base_dir[64] = CONFIG_OSD_BASEDIR_DFLT;
static int osd_format = 0;
ISCSI_QUEUE_T g_cmd_queue[CONFIG_OSD_MAX_LUNS];
static ISCSI_WORKER_T g_lun_worker[CONFIG_OSD_MAX_LUNS];
#if 0
static unsigned char *ramdisk[CONFIG_OSD_MAX_LUNS];
#endif
static LUN_WORKER_ARGS_T 
g_lun_worker_args[CONFIG_OSD_MAX_LUNS][CONFIG_OSD_WORKERS_PER_LUN];
static OST_DEVICE_COUNTERS_T device_stats[CONFIG_OSD_MAX_LUNS];

uint32_t g_disk_block_len[1] = {512}; /* only used in target.c stats */

OST_DEVICE_COUNTERS_T* device_counters_get(uint64_t lun) {
    return &device_stats[lun];
}

void device_pargs(void) {
    PRINT("  -f                  Format OSD device\n");
    PRINT("  -l <number of LUs>  Number of logical units (dflt %i)\n", 
          CONFIG_OSD_LUNS_DFLT);
    PRINT("  -d <dir>            Base directory for OSD (dflt %s)\n", \
          CONFIG_OSD_BASEDIR_DFLT);
}

int device_args(int argc, char *argv[]) {
    extern int optind, optopt, opterr;
    extern char *optarg;
    char c;

    opterr = 0;
    while ((c = getopt(argc, argv, "fc:l:d:")) != -1) {
        switch(c) {
        case 'f':
            osd_format = 1;
            break;
        case 'l':
            osd_num_luns = atoi(optarg);
            if (osd_num_luns > CONFIG_OSD_MAX_LUNS) {
                TRACE_ERROR("max luns is %i\n", CONFIG_OSD_MAX_LUNS);
                return -1;
            }
            break;
        case 'd':
            if (strlen(optarg)>64) {
                TRACE_ERROR("directory name too long\n");
                return -1;
            } 
            strcpy(base_dir, optarg);
            break;
        case '?':
            TRACE_ERROR("unknown option or bad syntax: -%c\n", optopt);
            return -1;
        }
    }
    return 0;
}

int lun_worker_proc(ISCSI_WORKER_T *me) {
    TARGET_CMD_T *cmd;
    ISCSI_SCSI_CMD_T *args;
    LUN_WORKER_ARGS_T *worker_args = me->ptr;

    worker_args->lun+=0;
    START_ELSE(me, 1, EXIT(me, -1));
    while (1) {
        ISCSI_WAIT(&me->wq, WORKER_STOPPING(me) || 
                   iscsi_queue_depth(&g_cmd_queue[me->id]));
        if (WORKER_STOPPING(me)) {
            TRACE(TRACE_WORKER, 0, "got shutdown signal\n");
            EXIT(me, 0);
        } else if (iscsi_queue_depth(&g_cmd_queue[me->id])) {
            if ((cmd=iscsi_queue_remove(&g_cmd_queue[me->id]))==NULL) {
                TRACE_ERROR("lun_worker[%i]: iscsi_queue_remove() failed\n", 
                            me->id);
                EXIT(me, -1);
            }
            args = &(cmd->scsi_cmd);
            TRACE(TRACE_SCSI, 2, "dequeued 0x%x (lun %"PRIu64", lun %u)\n", 
                  args->tag, args->lun, ISCSI_LUN(cmd->header));
            if (device_command(cmd)!=0) {
                TRACE_ERROR("device_command() failed\n");
                EXIT(me, -1);
            }
        } else {
            TRACE_ERROR("woken up for no reason\n");
            EXIT(me, -1);
        }
    }
    EXIT(me, 0);
}

int device_init() {
    char FileName[1024];
    char string[1024];
    struct stat stat_buff;
    int i, fid;
    char buffer[512];

    if (osd_format) {

        /* Remove entire OSD directory */
        sprintf(string, "rm -rf %s", base_dir); 
        if (system(string)!=0) {
            TRACE_ERROR("\"%s\" failed\n", string);
            return -1;
        }

        /* Create directory for OSD */
        if (mkdir(base_dir, 0755)!=0) {
            if (errno!=EEXIST) {
                TRACE_ERROR("error creating directory \"%s\" for OSD: "
                            "errno %i\n", base_dir, errno);
                return -1;
            }
        } 

        /* Create directory  for each LU */
        for (i=0; i<osd_num_luns; i++) {
            sprintf(FileName, "%s/lun_%i", base_dir, i);
            if (mkdir(FileName, 0755)!=0) {
                if (errno!=EEXIST) {
                    TRACE_ERROR("error creating \"%s\" for LU %i: errno %i\n", 
                                FileName, i, errno);
                    return -1;
                }
            }
        }
    }

    for (i=0; i<osd_num_luns; i++) {    

#if 0
        /* ramdisk for when operating in block mode */
        PRINT("DISK: LU %i: ", i);
        if ((ramdisk[i]=iscsi_malloc(CONFIG_LUN_SIZE))==NULL) {
            TRACE_ERROR("iscsi_malloc() failed\n");
            return -1;
        }
        PRINT("\n%.0f MB ramdisk for block mode\n", CONFIG_LUN_SIZE/1048576.0);
#else
        sprintf(string, "mkdir %s/lun_%i/0x0", base_dir, i); 
        system(string);
        sprintf(string, "%s/lun_%i/0x0/0x0", base_dir, i); 
        if ((fid=open(string, O_CREAT|O_RDWR, 0666))==-1) {
            TRACE_ERROR("error opening \"%s\"\n", string);
            return -1;
        }
        if (lseek(fid, CONFIG_LUN_SIZE-512, SEEK_SET) == -1) {
            TRACE_ERROR("error seeking \"%s\"\n", string);
            return -1;
        }
        if (write(fid, buffer, 512) == -1) {
            TRACE_ERROR("error writing \"%s\" (errno %i)", string, errno);
            return -1;
        }
        close(fid);
        PRINT("\n%.0f MB object for block mode (e.g., through /dev/so0 "
              "on the host)\n\n", CONFIG_LUN_SIZE/1048576.0);
        PRINT("Note: Run \"mknod /dev/so0 b 232 0\" on the host to create "
              "/dev/so0.\n");
        PRINT("      You can then a) open it like a block device (e.g., "
              "fdisk /dev/so0)\n");
        PRINT("      or b) mount osdfs (e.g., \"mount -t osdfs /dev/so0 "
              "/mnt\").\n");
#endif


        /* Create worker thread for each LU */

        if (iscsi_queue_init(&g_cmd_queue[i], CONFIG_LUN_QUEUE_DEPTH)!=0) {
            TRACE_ERROR("iscsi_queue_init() failed\n");
            return -1;
        }
        g_lun_worker_args[i][0].lun = i;
        START_WORKER_ELSE("lun worker", i, g_lun_worker[i], lun_worker_proc, 
                          &g_lun_worker_args[i][0], return -1);
    }

    if (stat(base_dir, &stat_buff)!=0) {
        PRINT("\n***********************\n");
        PRINT("* OSD not formatted.  *\n");
        PRINT("* Run with -f option. *\n");
        PRINT("***********************\n\n");
        return -1;
    }

    return 0;
}

static int command_done(TARGET_CMD_T *cmd) {
    int i;

    TRACE(TRACE_SCSI, 1, 
          "session %i: completed SCSI op 0x%x (status 0x%x, tag 0x%x)\n",
          cmd->sess->id, cmd->scsi_cmd.cdb[0], cmd->scsi_cmd.status, 
          ISCSI_ITT(cmd->header));
    
    if (cmd->callback(cmd->callback_arg)!=0) {
        TRACE_ERROR("target callback failed for 0x%x\n", 
                    ISCSI_ITT(cmd->header));
        return -1;
    }

    for (i=0; i<cmd->scatter_len; i++) {
        switch (cmd->scatter_type[i]) {
        case (MEM_STACK):
            break;
        case (MEM_HEAP):
            TRACE(TRACE_SCSI, 2, "freeing scatter %i (len %Zu)\n", i, 
                  cmd->gather_list[i].iov_len);
            iscsi_free_atomic(cmd->scatter_list[i].iov_base);
            break;
        case (MEM_MAPPED):
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
        case (MEM_HEAP):
            TRACE(TRACE_SCSI, 2, "freeing gather %i (len %Zu)\n", i, 
                  cmd->gather_list[i].iov_len);
            iscsi_free_atomic(cmd->gather_list[i].iov_base);
            break;
        case (MEM_MAPPED):
            break;
        default:
            TRACE_ERROR("unknown memory type %i\n", cmd->gather_type[i]);
            exit(1);        
        }
    }

    if (cmd->scsi_cmd.cdb[0] == 0x26) {
        exit(0);
    }

    return 0;
}

/*
 * This is the function that executes the extended CDB. It is called
 * after all incoming data has been received from the target system.
 */

int exec_upper(TARGET_CMD_T *cmd) {
    osd_args_t osd_args;
    ISCSI_SCSI_CMD_T *args = &cmd->scsi_cmd;
    uint8_t *buffer_ptr = cmd->scatter_list[0].iov_base;
    uint8_t *set_list = NULL;
    uint8_t *get_list = NULL;
    uint8_t *write_data = NULL;
    uint8_t *get_data = NULL;
    uint8_t *read_data = NULL;
    int fid = -1;
    char FileName[1024];
    char string[1024];
    uint32_t GroupID = 0;
    uint64_t UserID = 0;
    int rc;
    int i;
    unsigned page = 0;
    unsigned index = 0;
    int attr_len = 0;

    TRACE(TRACE_SCSI, 1, "executing upper half of 0x%x\n", args->tag);
    if (args->ext_cdb == NULL) {
        TRACE_ERROR("ext_cdb is NULL for op 0x%x\n", args->tag);
        return -1;
    }
    OSD_DECAP_CDB(args->cdb, args->ext_cdb, &osd_args);
    //OSD_PRINT_CDB(args->cdb, args->ext_cdb);

    /* set pointers to incoming data */
    if (osd_args.set_attributes_list_length) {
        TRACE(TRACE_SCSI, 1, "preparing for %u byte SET list\n", 
              osd_args.set_attributes_list_length);
        set_list = buffer_ptr;
        buffer_ptr += osd_args.set_attributes_list_length;
    }
    if (osd_args.get_attributes_list_length) {
        TRACE(TRACE_SCSI, 1, "preparing for %u byte GET list\n", 
              osd_args.get_attributes_list_length);
        get_list = buffer_ptr;
        buffer_ptr += osd_args.get_attributes_list_length;
    }
    if (osd_args.service_action == OSD_WRITE) {
        write_data = buffer_ptr;
        TRACE(TRACE_SCSI, 1, 
              "preparing for %"PRIu64" byte WRITE data (buffer_ptr %p)\n", 
              osd_args.length, buffer_ptr);
        buffer_ptr += osd_args.length;
    }

    /* Execute extended CDB (FIX ME: user errors should return status) */
    switch(osd_args.service_action) {

    case OSD_CREATE_GROUP:
        TRACE_CLEAN(TRACE_SCSI, 0, 
                    "0x%x: OSD_CREATE_GROUP(lun %"PRIu64") --> ",
                    args->tag, args->lun);
        if (!osd_args.GroupID) {
            GroupID = rand()%1048576*1024+1;
        } else {            
            GroupID = osd_args.GroupID;
        }
    create_group_again: sprintf(FileName, "%s/lun_%"PRIu64"/0x%x", base_dir, 
                                args->lun, GroupID);
        rc = mkdir(FileName, 0755);
        if ((rc==-1)&&(errno==EEXIST)&&!osd_args.GroupID) {
            GroupID = rand()%1048576*1024+1;
            goto create_group_again;
        } else if ((rc == -1 ) && (errno==EEXIST)) {
            TRACE_CLEAN(TRACE_SCSI, 0, 
                        "0x%x already exists (use \"-o no_format\")\n", 
                        GroupID);
            rc = 0;
        } else if (rc == -1) {
            TRACE_ERROR("mkdir() failed\n");
            goto protocol_error;
        } else {
            TRACE_CLEAN(TRACE_SCSI, 0, "0x%x\n", GroupID);
        }
        break;
        
    case OSD_REMOVE_GROUP:      
        TRACE_CLEAN(TRACE_SCSI, 0, 
                    "0x%x: OSD_REMOVE_GROUP(lun %"PRIu64", 0x%x)\n", 
                    args->tag, args->lun, osd_args.GroupID);
        sprintf(FileName, "%s/lun_%"PRIu64"/0x%x", 
                base_dir, args->lun, osd_args.GroupID);
        if (rmdir(FileName)==-1) {
            TRACE_ERROR("rmdir(\"%s\") failed: errno %i\n", FileName, errno);
            goto device_error;
        } 
        break;
        
    case OSD_CREATE:
        TRACE_CLEAN(TRACE_SCSI, 0, 
                    "0x%x: OSD_CREATE(lun %"PRIu64", GroupID 0x%x) --> ", 
                    args->tag, args->lun, osd_args.GroupID);

    create_again:
        if (!osd_args.UserID) {
            UserID = rand()%1048576*1024+1;
        } else {            
            UserID = osd_args.UserID;
        }
        sprintf(FileName, "%s/lun_%"PRIu64"/0x%x/0x%"PRIx64"", 
                base_dir, args->lun, osd_args.GroupID, UserID);
        fid = open(FileName, O_CREAT|O_RDWR|O_TRUNC, 0644);
        if ((fid==-1)&&(errno==EEXIST)&&!osd_args.UserID) {
            goto create_again;
        } else if (fid == -1) {
            TRACE_ERROR("OSD_CREATE(UserID 0x%"PRIx64") failed (errno %i)\n", 
                        UserID, errno);     
            goto protocol_error;
        }
        close(fid);
        osd_args.UserID = UserID;
        TRACE_CLEAN(TRACE_SCSI, 0, 
                    "0x%"PRIx64"%s\n", UserID, 
                    osd_args.UserID?" (user-assigned)":" (device-assigned)");
        break;
        
    case OSD_REMOVE:
        TRACE_CLEAN(TRACE_SCSI, 0, 
                    "0x%x: OSD_REMOVE(lun %"PRIu64", 0x%"PRIx64")\n", 
                    args->tag, args->lun, osd_args.UserID);
#if 1
        /* remove object data */
        sprintf(string, "rm -f %s/lun_%"PRIu64"/0x%x/0x%"PRIx64"*",
                base_dir, args->lun, osd_args.GroupID, osd_args.UserID); 
        if ((rc=system(string))!=0) {
            TRACE_ERROR("\"%s\" failed (rc %i errno %i)\n", string, rc, errno);
            goto device_error;
        } else {
            args->status = 0;
        }
#else
        args->status = 0;
#endif
        break;

    case OSD_WRITE:

        TRACE_CLEAN(TRACE_SCSI, 0, 
                    "0x%x: OSD_WRITE(lun %"PRIu64", GroupID 0x%x, UserID 0x%"
                    PRIx64", length %"PRIu64", offset %"PRIu64")\n",
                    args->tag, args->lun, osd_args.GroupID, osd_args.UserID, 
                    osd_args.length, osd_args.offset);

#if 0
        if (!osd_args.GroupID&&!osd_args.UserID) {
            if (osd_args.length+osd_args.offset > CONFIG_LUN_SIZE) {
                TRACE_ERROR("ramdisk capacity exceeded\n");
                goto protocol_error;
            }
            memcpy(ramdisk[args->lun]+osd_args.offset, write_data, 
                   osd_args.length);
        } else {
#endif
            sprintf(FileName, "%s/lun_%"PRIu64"/0x%x/0x%"PRIx64"",
                    base_dir, args->lun, osd_args.GroupID, osd_args.UserID);
            if ((fid=open(FileName, O_CREAT|O_WRONLY, 0644))==-1) {
                TRACE_ERROR("error opening \"%s\": errno %i\n", 
                            FileName, errno);
                goto protocol_error;
            } 
            if (lseek(fid, osd_args.offset, SEEK_SET) == -1) {
                TRACE_ERROR("error seeking \"%s\": errno %i\n", 
                            FileName, errno);
                close(fid);
                goto protocol_error;
            } 
            if ((rc=write(fid, write_data, osd_args.length))!=osd_args.length) {
                TRACE_ERROR("write() failed (errno %i, rc %i)\n", errno, rc);
                close(fid);
                goto protocol_error;
            }
            close(fid);
#if 0
        }
#endif

        /* update device counters */
        ISCSI_SPIN_LOCK_ELSE(&device_stats[args->lun].lock, exit(1));
        ISCSI_SPIN_UNLOCK_ELSE(&device_stats[args->lun].lock, exit(1));

        break;

    case OSD_READ:

        TRACE_CLEAN(TRACE_SCSI, 0, "0x%x: OSD_READ(lun %"PRIu64", GroupID "
                    "0x%x, UserID 0x%"PRIx64", length %"PRIu64", offset %"
                    PRIu64")\n", args->tag, args->lun, osd_args.GroupID, 
                    osd_args.UserID, osd_args.length, osd_args.offset);

#if 0
        if (!osd_args.GroupID&&!osd_args.UserID) {
            if (osd_args.length+osd_args.offset > CONFIG_LUN_SIZE) {
                TRACE_ERROR("ramdisk capacity exceeded\n");
                goto protocol_error;
            }
            read_data = ramdisk[args->lun]+osd_args.offset;
            cmd->gather_type[0] = MEM_STACK; 
        } else {
#endif
            if ((read_data=iscsi_malloc_atomic(osd_args.length))==NULL) {
                TRACE_ERROR("iscsi_malloc_atomic() failed\n");
                close(fid);
                goto device_error;
            } 
            cmd->gather_type[0] = MEM_HEAP;
            sprintf(FileName, "%s/lun_%"PRIu64"/0x%x/0x%"PRIx64"", 
                    base_dir, args->lun, osd_args.GroupID, osd_args.UserID);
            if ((fid=open(FileName, O_CREAT|O_RDONLY, 0644))==-1) {
                TRACE_ERROR("error opening \"%s\": errno %i\n", 
                            FileName, errno);
                goto protocol_error;
            } 
            if (lseek(fid, osd_args.offset, SEEK_SET) == -1) {
                TRACE_ERROR("error seeking \"%s\": errno %i\n", 
                            FileName, errno);
                close(fid);
                goto protocol_error;
            } 
            if ((rc=read(fid, read_data, osd_args.length))!=osd_args.length) {
                memset(read_data+rc, 0, osd_args.length-rc);
            }
            args->status = 0;
            close(fid);
#if 0
        }
#endif
        args->fromdev = 1;
        cmd->gather_list[0].iov_base = read_data;
        cmd->gather_list[0].iov_len = osd_args.length;
        cmd->gather_len = 1;
        cmd->gather_list[1].iov_base = NULL; 
        cmd->gather_list[1].iov_len = 0;

        /* update device counters */
        ISCSI_SPIN_LOCK_ELSE(&device_stats[args->lun].lock, exit(1));
        ISCSI_SPIN_UNLOCK_ELSE(&device_stats[args->lun].lock, exit(1));

        break;

    case OSD_GET_ATTR:

        TRACE_CLEAN(TRACE_SCSI, 0, "0x%x: OSD_GET_ATTR(lun %"PRIu64", "
                    "GroupID 0x%x, UserID 0x%"PRIx64")\n", args->tag, 
                    args->lun, osd_args.GroupID, osd_args.UserID);
        break;

    case OSD_SET_ATTR:

        TRACE_CLEAN(TRACE_SCSI, 0, "0x%x: OSD_SET_ATTR(lun %"PRIu64", "
                    "GroupID 0x%x, UserID 0x%"PRIx64")\n", args->tag, 
                    args->lun, osd_args.GroupID, osd_args.UserID);
        break;

    default:
        TRACE_ERROR("0%x: unknown service action 0%x\n", args->tag, 
                    osd_args.service_action);
        goto protocol_error;
        break;
    }

    /*
     * SET attributes
     */
    
    if (osd_args.set_attributes_list_length) {
        uint32_t page, attr;
        uint16_t len;
        int i;
        
        for (i=0; i<osd_args.set_attributes_list_length;) {
            page = NTOHL(*((uint32_t *)(&(set_list[i])))); i+=4;
            attr = NTOHL(*((uint32_t *)(&(set_list[i])))); i+=4;
            len = NTOHS(*((uint16_t *)(&(set_list[i])))); i+=2;

            TRACE_CLEAN(TRACE_SCSI, 1, "0x%x: SET(0x%x,%u,%u>\n", 
                        args->tag, page, attr, len); 

            sprintf(FileName, "%s/lun_%"PRIu64"/0x%x/0x%"PRIx64".0x%x.%u",
                    base_dir, args->lun, osd_args.GroupID, 
                    osd_args.UserID, page, attr);
            if ((fid=open(FileName, O_WRONLY|O_CREAT, 0644))==-1) {
                TRACE_ERROR("error opening \"%s\": errno %i\n", 
                            FileName, errno);
                goto protocol_error;
            }
            if (write(fid, set_list+i, len)!=len) {
                TRACE_ERROR("write() failed\n");
                close(fid);
                goto protocol_error;
            }
            close(fid);
            i += len;
        }
    }

    /* GET attributes */

    if (osd_args.get_attributes_list_length||osd_args.get_attributes_page) {
        TRACE(TRACE_SCSI, 1, "preparing %u bytes of attributes\n", 
              osd_args.get_attributes_allocation_length);
        if ((get_data=
             iscsi_malloc_atomic(osd_args.get_attributes_allocation_length))
            ==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
            goto device_error;
        }
    }

    if (osd_args.get_attributes_list_length) {
        TRACE(TRACE_SCSI, 1, 
              "preparing get attributes from %u byte GET list\n", 
              osd_args.get_attributes_list_length);
        for (i=0; i<osd_args.get_attributes_list_length;) {
            page = NTOHL(*((uint32_t *)(&(get_list[i])))); i+=4;
            index = NTOHL(*((uint32_t *)(&(get_list[i])))); i+=4;

            TRACE_CLEAN(TRACE_SCSI, 1, "0x%x: GET(page 0x%x, index %u)\n", 
                        args->tag, page, index);

            switch(page) {
            case (0x40000001):
                switch(index) {
                case (0x1):
                    *((uint32_t *)&get_data[attr_len]) = HTONL(page);    
                    attr_len +=4;  
                    *((uint32_t *)&get_data[attr_len]) = HTONL(index);   
                    attr_len +=4;
                    *((uint16_t *)&get_data[attr_len]) = HTONS(4);       
                    attr_len +=2;
                    *((uint32_t *)&get_data[attr_len]) = HTONL(GroupID); 
                    attr_len += 4;
                    break;
                default:
                    TRACE_ERROR("unknown attr index %u\n", index);
                    goto protocol_error;
                }
                break;
            case (0x00000001):
                switch(index) {
                case (0x1):
                    *((uint32_t *)&get_data[attr_len]) = HTONL(page);     
                    attr_len +=4;  
                    *((uint32_t *)&get_data[attr_len]) = HTONL(index);    
                    attr_len +=4;
                    *((uint16_t *)&get_data[attr_len]) = HTONS(4);        
                    attr_len +=2;
                    *((uint32_t *)&get_data[attr_len]) = HTONL(GroupID);  
                    attr_len += 4;
                    break;
                case (0x2):
                    *((uint32_t *)&get_data[attr_len]) = HTONL(page);    
                    attr_len +=4;  
                    *((uint32_t *)&get_data[attr_len]) = HTONL(index);   
                    attr_len +=4;
                    *((uint16_t *)&get_data[attr_len]) = HTONS(8);       
                    attr_len +=2;
                    *((uint64_t *)&get_data[attr_len]) = HTONLL(UserID); 
                    attr_len += 8;
                    break;
                default:
                    TRACE_ERROR("unknown attr index %u\n", index);
                    goto protocol_error;
                }
                break;

            case (0x30000000): /* vendor specific */
                switch(index) {
                case (0x1):
                    *((uint32_t *)&get_data[attr_len]) = HTONL(page);    
                    attr_len +=4;  
                    *((uint32_t *)&get_data[attr_len]) = HTONL(index);   
                    attr_len +=4;
                    *((uint16_t *)&get_data[attr_len]) = HTONS(INODE_LEN);     
                    attr_len +=2;
                    sprintf(FileName, 
                            "%s/lun_%"PRIu64"/0x%x/0x%"PRIx64".0x%x.%u",
                            base_dir, args->lun, osd_args.GroupID, 
                            osd_args.UserID, page, index);
                    if ((fid=open(FileName, O_RDONLY, 0644))==-1) {
                        TRACE_ERROR("error opening \"%s\": errno %i\n", 
                                    FileName, errno);
                        goto protocol_error;
                    }
                    if (read(fid, get_data+attr_len, INODE_LEN)!=INODE_LEN) {
                        TRACE_ERROR("read() failed\n");
                        close(fid);
                        goto protocol_error;
                    }
                    close(fid);
                    attr_len += INODE_LEN;
                    break;
                default:
                    TRACE_ERROR("unknown vendor attr index %u\n", index);
                    goto protocol_error;
                }
                break;

            default:
                TRACE_ERROR("unknown page 0x%x\n", page);
                goto protocol_error;
            }
        } 
    } 

    if (osd_args.get_attributes_page) {

        TRACE(TRACE_SCSI, 1, "preparing get attributes from page 0x%x\n", 
              osd_args.get_attributes_page);

        /* 
         * Right now, if we get a request for an entire page, we 
         * return only one attribute.
         */
  
        page = osd_args.get_attributes_page;

        TRACE_CLEAN(TRACE_SCSI, 1, "0x%x: GET(page 0x%x)\n", args->tag, page);

        switch (osd_args.get_attributes_page) {
        case (0x40000001):
            index = 1;
            *((uint32_t *)&get_data[attr_len]) = HTONL(page);    attr_len +=4;  
            *((uint32_t *)&get_data[attr_len]) = HTONL(index);   attr_len +=4;
            *((uint16_t *)&get_data[attr_len]) = HTONS(4);       attr_len +=2;
            *((uint32_t *)&get_data[attr_len]) = HTONL(GroupID); attr_len +=4;
            break;

        case (0x00000001):
            index = 2;
            *((uint32_t *)&get_data[attr_len]) = HTONL(page);    attr_len +=4;  
            *((uint32_t *)&get_data[attr_len]) = HTONL(index);   attr_len +=4;
            *((uint16_t *)&get_data[attr_len]) = HTONS(8);       attr_len +=2;
            *((uint64_t *)&get_data[attr_len]) = HTONLL(UserID); attr_len +=8;
            break;

        case (0x30000000):
            index = 1;
            *((uint32_t *)&get_data[attr_len]) = HTONL(page);    
            attr_len +=4;  
            *((uint32_t *)&get_data[attr_len]) = HTONL(index);   
            attr_len +=4;
            *((uint16_t *)&get_data[attr_len]) = HTONS(INODE_LEN);     
            attr_len +=2;
            sprintf(FileName, "%s/lun_%"PRIu64"/0x%x/0x%"PRIx64".0x%x.%u",
                    base_dir, args->lun, osd_args.GroupID, osd_args.UserID, 
                    page, index);
            if ((fid=open(FileName, O_RDONLY, 0644))==-1) {
                TRACE_ERROR("error opening \"%s\": errno %i\n", FileName, 
                            errno);
                goto protocol_error;
            }
            if (read(fid, get_data+attr_len, INODE_LEN)!=INODE_LEN) {
                TRACE_ERROR("read() %s failed\n", FileName);
                close(fid);
                goto protocol_error;
            }
            close(fid);
            attr_len += INODE_LEN;
            break;
        default:
            TRACE_ERROR("page not yet supported\n");
            goto protocol_error;
        }
    }

    if (attr_len) {
        if (attr_len != osd_args.get_attributes_allocation_length) {
            TRACE_ERROR("allocation lengths differ: got %u, expected %u\n", 
                        osd_args.get_attributes_allocation_length, attr_len);
            goto protocol_error;
        }
        args->fromdev = 1;
        cmd->gather_list[cmd->gather_len].iov_base = get_data;
        cmd->gather_list[cmd->gather_len].iov_len = 
            osd_args.get_attributes_allocation_length;
        cmd->gather_type[cmd->gather_len] = MEM_HEAP;
        cmd->gather_len++;
        cmd->gather_list[cmd->gather_len].iov_base = NULL; 
        cmd->gather_list[cmd->gather_len].iov_len = 0;
    }
    args->status = 0;
    goto done;

 device_error:
    if (read_data) iscsi_free_atomic(read_data);
    if (get_data) iscsi_free_atomic(get_data);
    exit(1);

 protocol_error:
    args->status = 2; args->length = 0;
    exit(1);
    goto done;

 done:
    if (command_done(cmd)!=0) {
        TRACE_ERROR("command_done() failed for 0x%x\n", args->tag);
        return -1;
    }
    return 0;
}

/*
 * For each extended SCSI command, we first set up the buffers for any incoming
 * data and then request that the target system transfer this data from the
 * initiator. When the data transfer has completed, the target will invoke
 * the callback we provide here (exec_upper) so that we can execute the 
 * actual CDB.
 */

int exec_lower(TARGET_CMD_T *cmd) {
    ISCSI_SCSI_CMD_T *args = &cmd->scsi_cmd;
    void *ptr;

    /* allocate buffer for all incoming data and copy in any immediate data */
    
    if (args->todev) {
        TRACE(TRACE_SCSI, 1, "preparing for %u bytes incoming data\n", 
              args->trans_len);
        if ((ptr=iscsi_malloc_atomic(args->trans_len))==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
            goto error;
        }
        cmd->scatter_list[0].iov_base = ptr;
        cmd->scatter_list[0].iov_len = args->trans_len;
        cmd->scatter_type[0] = MEM_HEAP;
        cmd->scatter_len = 1;
        if (args->length) {
            TRACE(TRACE_SCSI, 1, "%u of %u bytes is immediate\n", 
                  args->length, args->trans_len);
            memcpy(ptr, cmd->immediate, args->length);
            args->bytes_todev = args->length;
        } else {
            args->bytes_todev = 0;
        } 
    }

    /* Request transfer of data from the target system. As the data becomes
       available, the target code will call device_place_data() to place the
       data into the buffer we've allocated above. When the data transfer
       has completed, device_place_data() will be called with a length of
       0 - a signal to us that the transfer has finished. We'll then call
       exec_upper() to execute the extended CDB. If there is no data to
       transfer from the initiator, we can just call exec_lower() from here. */
 
    if (args->todev && (args->trans_len-args->bytes_todev)) {
        TRACE(TRACE_SCSI, 1, 
              "requesting transfer of %u bytes OSD data (tag 0x%x)\n", 
              args->trans_len-args->bytes_todev, cmd->scsi_cmd.tag);
        if (target_transfer_data(cmd)!=0) {
            TRACE_ERROR("target_transfer_data() failed\n");
            goto error;
        }
    } else {
        TRACE(TRACE_SCSI, 1, "no data to transfer (tag 0x%x)\n", args->tag);
        if (exec_upper(cmd)!=0) {
            TRACE_ERROR("exec_upper() failed for 0x%x\n", args->tag);
            exit(0);
            goto error;
        }
    }

    return 0;
 error:
    return -1;
}

/*
 * This is the callback the target will use whenever we have incoming
 * data for a particular command. It is our job to place the data into
 * a buffer that we allocated in exec_upper()
 */

int device_place_data(TARGET_CMD_T *cmd, unsigned off, unsigned len) {
    TARGET_SESSION_T *sess = cmd->sess;
    struct iovec *iov, *iov_ptr = NULL;
    int iov_len;
    int rc = -1;
    
    if (!len) {
        TRACE(TRACE_SCSI, 1, "data transfer completed (tag 0x%x)\n", 
              cmd->scsi_cmd.tag);
        return exec_upper(cmd);
    }

    TRACE(TRACE_SCSI, 1, "placing %u bytes of data at offset %u (tag 0x%x)\n", 
          len, off, cmd->scsi_cmd.tag);
    
    /* create iovec for just this portion of the data */
    
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
    
    /* read data off the socket */

    if ((rc=iscsi_sock_msg(sess->sock, 0, len, iov, iov_len))!=len) {
        TRACE_ERROR("iscsi_sock_msg() failed (rc %u)\n", rc);
        goto error;;
    }
    rc = len;
 error:
    if (iov_ptr) iscsi_free_atomic(iov_ptr);
    return rc;
}

int device_command(TARGET_CMD_T *cmd) {
    ISCSI_SCSI_CMD_T *args = &cmd->scsi_cmd;
    unsigned char data[4096];
    unsigned char lun = args->lun;
    unsigned char *cdb = args->cdb;
    int done = 1;
    int override_gather = 0;

    /* check for correct lun */
    
    if ((lun>=osd_num_luns)&&(cdb[0] == INQUIRY)) {
        memset(data, 0, cdb[4]);
        data[0] = 0x60; // peripheral qualifier
        data[0] |= 0x7F;  // no device
        if (cdb[4] != args->trans_len) {
            TRACE_WARN("WARN: cdb allocation length (%u) != iSCSI transfer "
                       "length (%u)\n", cdb[4], args->trans_len);
        }
        args->fromdev = 1;
        args->status = 0;
        args->length = args->trans_len;
        TRACE(TRACE_SCSI, 1, "SCSI op 0x%x (lun %u): Invalid lun\n", 
              cdb[0], lun);
        goto done;
    } else if (lun>=osd_num_luns) {
        args->status = 2;
        args->fromdev = 0;
        args->length = 0;
        TRACE(TRACE_SCSI, 0, "SCSI op 0x%x (lun %u): Invalid lun\n", 
              cdb[0], lun);
        goto done;
    }

    args->status = 0;

    switch(args->cdb[0]) {

    case TEST_UNIT_READY:
        TRACE_CLEAN(TRACE_SCSI, 0, "0x%x: TEST_UNIT_READY(lun %"PRIu64")\n", 
                    args->tag, args->lun);
        if (args->trans_len != 0) {
            TRACE_ERROR("bad trans len for op 0x%x TEST_UNIT_READY (%u)\n", 
                        args->tag, args->trans_len);
            exit(0);
        }
        EQUAL_ELSE("trans len", args->trans_len, 0, {args->status = 2; 
                goto done;});
        args->length = 0;
        break;

    case INQUIRY:
        TRACE_CLEAN(TRACE_SCSI, 0, "0x%x: INQUIRY(lun %"PRIu64")\n", 
                    args->tag, args->lun);
        memset(data, 0, args->cdb[4]);            // Clear allocated buffer
        data[0] = 0x0e;                           // Peripheral Device Type
        //data[1] |= 0x80;                        // Removable Bit
        data[2] |= 0x02;                          // ANSI-approved version
        //data[3] |= 0x80;                        // AENC
        //data[3] |= 0x40;                        // TrmIOP
        //data[3] |= 0x20;                        // NormACA
        data[4] = args->cdb[4]-4;                 // Additional length
        //data[7] |= 0x80;                        // Relative addressing
        data[7] |= 0x40;                          // WBus32
        data[7] |= 0x20;                          // WBus16
        //data[7] |= 0x10;                        // Sync
        //data[7] |= 0x08;                        // Linked Commands
        //data[7] |= 0x04;                        // TransDis
        if (CONFIG_LUN_QUEUE_DEPTH>1) {
            data[7] |= 0x02;                        // Tagged Command Queueing
        }
        //data[7] |= 0x01;                        // SftRe
        strcpy((char *)(data+8),"Intel" );                // Vendor
        strcpy((char *)(data+16), "iSCSI OSD" );          // Product ID
        sprintf((char *)(data+32), "%i", ISCSI_VERSION);  // Product Revision
        if (cdb[4] != args->trans_len) {
            TRACE_WARN("WARN: cdb allocation length (%u) != iSCSI transfer "
                       "length (%u)\n", cdb[4], args->trans_len);
        }
        args->fromdev = 1;
        args->status = 0;
        args->length = args->trans_len;
        break;

        /* not an OSD command, but comes in handy for block mode */
    case READ_CAPACITY:
        TRACE_CLEAN(TRACE_SCSI, 0, " READ_CAPACITY(lun %"PRIu64")\n", 
                    args->lun);
        *((unsigned *) data) = HTONL(CONFIG_LUN_SIZE/512);  /* Max LBA */
        *((unsigned *) (data+4)) = HTONL(512);  /* Block len */
        args->fromdev = 1;
        args->status = 0;
        args->length = 8;
        break;

    case 0x7F:
        TRACE_CLEAN(TRACE_SCSI, 1, 
                    "0x%x: EXTENDED(lun %"PRIu64", service action 0x%x)\n", 
                    args->tag, args->lun, 
                    NTOHS(*((uint16_t *)((args->cdb)+8))));
        if (exec_lower(cmd)!=0) {
            TRACE_ERROR("exec_extended() failed\n");        
            args->status = 0x02;
            break;
        }
        done = 0;
        break;
       
    case 0x3:
        TRACE_CLEAN(TRACE_SCSI, 0, "0x%x: REQUEST_SENSE(lun %"PRIu64")\n", 
                    args->tag, args->lun);
        args->status = args->length = 0;
        break;

    case 0x24: {
        TRACE_CLEAN(TRACE_SCSI, 0, "COUNTERS (tag 0x%x, %u bytes)\n", 
                    args->tag, args->trans_len);        
        if (read_counters(lun, (char *) data)!=0) {
            args->status = 0x02;                
        } else {        
            args->status = 0x0;
            args->length = 4096;
            args->fromdev = 1;
        }
        done = 1; 
        break;
    }

    case 0x26:
        PRINT("0x%x: DEVICE_EXIT\n", args->tag);
        done = 1;
        args->status = 0;
        args->length = 0;
        break;

    default:
        TRACE_ERROR("UNKNOWN OPCODE 0x%x\n", args->cdb[0]);
        args->status = 2;
        args->length = 0;
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

int device_shutdown() {
    int i;

#if 0
    for (i=0; i<osd_num_luns; i++) {
        iscsi_free(ramdisk[i]);
    }
#endif

    return 0;
}

int device_enqueue(TARGET_CMD_T *cmd) {
    ISCSI_SCSI_CMD_T *args = &(cmd->scsi_cmd);

    if (args->lun>=osd_num_luns) return device_command(cmd);
    ISCSI_WAKE_ELSE((iscsi_queue_insert(&g_cmd_queue[args->lun], cmd)==0), 
                    &g_lun_worker[args->lun].wq, return -1);
    return 0;
}
