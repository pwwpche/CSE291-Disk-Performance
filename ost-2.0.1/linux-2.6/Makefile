
#
# Modify SRC to point to your kernel source. If you build your kernel
# in a different directory, then set OBJ to the location of your
# kernel build.
# 
SRC=/usr/src/linux
OBJ=$(SRC)

#
# Do not modify anything below
#

# Builds the iSCSI driver intel_iscsi.ko
obj-m += intel_iscsi.o

# Should not need to modify anything below
CFLAGS_driver.o = -I/usr/include -I$(ISCSI_TOP)
CFLAGS_initiator.o = -I/usr/include -I$(ISCSI_TOP)
CFLAGS_iscsi.o = -I/usr/include -I$(ISCSI_TOP)
CFLAGS_util.o = -I/usr/include -I$(ISCSI_TOP)
CFLAGS_parameters.o = -I/usr/include -I$(ISCSI_TOP)
CFLAGS_debug.o = -I/usr/include -I$(ISCSI_TOP)
intel_iscsi-objs := driver.o initiator.o iscsi.o ost.o util.o parameters.o \
                    debug.o md5.o

all:
	rm -f *.c *.h; ln -s ../src/*.c .; ln -s ../src/*.h .;
	make -j 16 -C $(OBJ) M=`pwd` ISCSI_TOP=`pwd`/..
	make ips

ips:
	@echo '#!/bin/sh' >$@;  \
	echo 'start() {' >> $@; \
	echo 'if /sbin/lsmod | grep intel_iscsi > /dev/null; then' >>$@; \
	echo 'echo' >>$@; \
	echo 'echo "****************************************************"'  \
	>>$@; \
	echo 'echo "*           iSCSI driver already started            *"' \
	>>$@; \
	echo 'echo "****************************************************"'  \
	>>$@; \
	echo 'echo' >>$@; \
	echo 'exit 1;' >>$@; \
	echo 'fi;' >>$@; \
	echo 'VERSION=`uname -r`' >> $@; \
	echo 'if ! /sbin/modprobe intel_iscsi; then' >> $@; \
	echo 'echo "failed to load intel_iscsi"; exit 1; fi' >> $@; \
	echo 'echo' >> $@; \
	echo 'echo "****************************************************"' \
	>> $@; \
	echo 'echo "*              iSCSI driver started                *"' \
	>> $@; \
	echo 'echo "* 1. dmesg for kernel messages                     *"' \
	>> $@; \
	echo 'echo "* 2. cat /proc/scsi/iscsi/* for driver information *"' \
	>> $@; \
	echo 'echo "****************************************************"' \
	>> $@; \
	echo 'echo "****************************************************"' \
	>> $@; \
	echo 'echo 0 > /proc/sys/kernel/hung_task_timeout_secs' 	   \
	>> $@; \
	echo 'echo "*        hung task timeout warning disabled        *"' \
	>> $@; \
	echo 'echo "****************************************************"' \
	>> $@; \
	echo 'echo' >> $@; \
	echo '}' >> $@; \
	echo 'stop() {' >> $@; \
	echo '/sbin/rmmod intel_iscsi' >> $@; \
	echo '}' >> $@; \
	echo 'case "$$1" in' >> $@; \
	echo 'start) start $$2 ;;' >> $@; \
	echo 'stop) stop ;;' >> $@; \
	echo '*)' >> $@; \
	echo 'esac' >> $@; \
	chmod +x $@

clean:
	rm -f *.c *.h *.o *.ko Module.symvers ips .*.cmd .*.d
	rm -rf .tmp_versions modules.order Module.markers

version:
	if test -f $(OBJ)/include/config/kernel.release; then cat $(OBJ)/include/config/kernel.release; else if cat $(OBJ)/include/linux/version.h | grep UTS_RELEASE > /dev/null; then cat $(OBJ)/include/linux/version.h | grep UTS_RELEASE | awk '{print $$3}' | sed 's/"//g'; else if cat $(OBJ)/include/linux/version.h | grep LINUX_VERSION_CODE > /dev/null; then cat $(OBJ)/include/linux/version.h | grep LINUX_VERSION_CODE | awk '{print $$3}' | perl -e 'chop($$v=<stdin>);for($$i=2;$$i>=0;$$i--) {printf("%i", ($$v&(0xff<<(8*$$i)))>>8*$$i);if($$i){printf(".")} else{printf("\n")}}'; fi; fi; fi

install:
	@version=`${MAKE} -s version`; \
	echo; \
        echo "############################"; \
        echo "kernel version is $$version"; \
	if [ -f intel_iscsi.ko ]; then \
		echo cp intel_iscsi.ko /lib/modules/$$version/kernel/drivers/scsi; \
		if ! cp intel_iscsi.ko /lib/modules/$$version/kernel/drivers/scsi; \
		then exit 1; fi; \
	fi; \
	if [ -f ips ]; then \
		echo cp ips /etc/init.d; \
		if ! cp ips /etc/init.d; then exit 1; fi; \
	fi; \
	echo /sbin/depmod $$version; \
	if ! /sbin/depmod $$version; then exit 1; fi; \
        echo "############################"; \
	echo; \
	echo "usage: /etc/init.d/ips <start|stop>"; \
	echo;

uninstall:
	@version=`${MAKE} -s version`; \
	echo; \
        echo "############################"; \
        echo "kernel version is $$version"; \
	if [ -f /lib/modules/$$version/kernel/drivers/scsi/intel_iscsi.ko ]; then \
		echo rm /lib/modules/$$version/kernel/drivers/scsi/intel_iscsi.ko; \
		if ! rm -f /lib/modules/$$version/kernel/drivers/scsi/intel_iscsi.ko; \
		then exit 1; fi; \
	fi; \
	if [ -f /etc/init.d/ips ]; then \
		echo rm /etc/init.d/ips; \
		if ! rm -f /etc/init.d/ips; then exit 1; fi; \
	fi; \
	echo /sbin/depmod $$version; if ! /sbin/depmod $$version; \
	then exit 1; fi; \
        echo "############################";
