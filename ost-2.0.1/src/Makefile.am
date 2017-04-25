EXTRA_DIST = ips.conf.tmpl driver.c driver.h \
	iscsi-diff-counters iscsi-diff-initiator-counters iscsi-diff-target-counters \
	iscsi-initiator-counters iscsi-monitor \
	iscsi-sample-counters iscsi-system-counters iscsi-gnupipe \
	iscsi-kill-udisk iscsi-kill-monitor iscsi-kill-tree \
	osdfs.c so.c

AM_CFLAGS = -Wall -Wno-unused -g -O2 -D_GNU_SOURCE

noinst_LIBRARIES = libiscsi.a libinitiator.a libtarget.a

bin_PROGRAMS = udisk utest fitness uosd iscsi_reset_counters

bin_SCRIPTS = iscsi-monitor iscsi-initiator-counters iscsi-diff-initiator-counters \
	iscsi-diff-target-counters iscsi-sample-counters iscsi-system-counters \
	iscsi-diff-counters iscsi-gnupipe iscsi-kill-udisk iscsi-kill-monitor iscsi-kill-tree

noinst_HEADERS = initiator.h iscsi.h md5.h osd.h osd_ops.h parameters.h target.h \
        tests.h ost.h util.h debug.h research.h device.h so.h disk.h

sysconf_DATA = ips.conf

ips.conf: ips.conf.tmpl
	@cp $< $@; \
	echo; \
	echo "######################################################"; \
	echo "#                                                    #"; \
	echo "# Modify src/ips.conf before running \"make install.\" #"; \
	echo "#                                                    #"; \
	echo "######################################################"; \
	echo

#
# User libraries
#

libiscsi_a_SOURCES = ost.c util.c iscsi.c parameters.c md5.c debug.c
libinitiator_a_SOURCES = initiator.c
libtarget_a_SOURCES = target.c

#
# User programs
#

fitness_SOURCES = fitness.c research.c
fitness_DEPENDENCIES = libinitiator.a libiscsi.a
if ENABLE_MPI
fitness_LDADD = -lpthread -lm -L . -linitiator -liscsi /home/mmesnier/usr/lib/libmpich.a
else
fitness_LDADD = -lpthread -lm -L . -linitiator -liscsi
endif

utest_SOURCES = utest-alpha.c tests.c osd_ops.c research.c
utest_DEPENDENCIES = libinitiator.a libiscsi.a
utest_LDADD = -lpthread -lm -L . -linitiator -liscsi

udisk_SOURCES = ost.c utarget.c disk-alpha.c tests.c osd_ops.c research.c
udisk_DEPENDENCIES = libtarget.a libinitiator.a libiscsi.a
udisk_LDADD = -lpthread -lm -L . -ltarget -liscsi -linitiator

uosd_SOURCES = utarget.c osd.c
uosd_DEPENDENCIES = libtarget.a libiscsi.a libinitiator.a
uosd_LDADD = -lpthread -lm -L . -ltarget -liscsi -linitiator

#usocktest_SOURCES = usocktest.c
#usocktest_DEPENDENCIES = libiscsi.a
#usocktest_LDADD = -lpthread -L . -liscsi

iscsi_reset_counters_SOURCES = iscsi_reset_counters.c

#install-exec-hook:
#	chmod a+s $(bindir)/iscsi_reset_counters

clean-local:

distclean-local:
	rm -f ips.conf
