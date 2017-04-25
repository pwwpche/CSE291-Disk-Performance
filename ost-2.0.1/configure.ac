AC_INIT([ost], [2.0.1])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_PROG_RANLIB
AC_ARG_ENABLE(debug, [  --enable-debug debugging mode [default=yes]], enable_debug="$enableval", enable_debug="yes")
AM_CONDITIONAL(ENABLE_DEBUG, test "$enable_debug" = "yes")
AC_ARG_ENABLE(mpi, [  --enable-mpi compile fitness test with MPI [default=no]], enable_mpi="$enableval", enable_mpi="no")
AM_CONDITIONAL(ENABLE_MPI, test "$enable_mpi" = "yes")
AC_ARG_ENABLE(watchdog, [  --enable-watchdog Initiator watchdog [default=no]], enable_watchdog="$enableval", enable_watchdog="no")
AM_CONDITIONAL(ENABLE_WATCHDOG, test "$enable_watchdog" = "yes")
AC_ARG_ENABLE(experimental, [  --enable-experimental Experimental code [default=no]], enable_experimental="$enableval", enable_experimental="no")
AM_CONDITIONAL(ENABLE_EXPERIMENTAL, test "$enable_experimental" = "yes")

if test $sysconfdir != '${prefix}/etc'; then
	AC_DEFINE_UNQUOTED(IPS_CONFIG_FILE, "$sysconfdir/ips.conf", "target configuration file")
else
	if test $prefix = 'NONE'; then
		sysconfdir="/etc"
		AC_DEFINE_UNQUOTED(IPS_CONFIG_FILE, "/etc/ips.conf", "target configuration file")
	else
		AC_DEFINE_UNQUOTED(IPS_CONFIG_FILE, "$prefix/etc/ips.conf", "target configuration file")
	fi
fi

if test "$enable_debug" = "yes"; then AC_DEFINE_UNQUOTED(CONFIG_ISCSI_DEBUG, 1, "debugging mode") fi;
if test "$enable_mpi" = "yes"; then AC_DEFINE_UNQUOTED(ENABLE_MPI, 1, "MPI mode for fitness") fi;
if test "$enable_experimental" = "yes"; then AC_DEFINE_UNQUOTED(CONFIG_EXPERIMENTAL, 1, "experimental code") fi;
if test "$enable_watchdog" = "yes"; then AC_DEFINE_UNQUOTED(CONFIG_INITIATOR_WATCHDOG, 1, "initiator watchdog") fi;

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])

AC_OUTPUT
