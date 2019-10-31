AC_PREREQ(2.59)
AC_INIT([PSUart],[0.5],[Objective Systems <tony@eztor.com>],[PSUart])
PACKAGE=psuart
VERSION=0.1

AC_CONFIG_AUX_DIR(config) 
AM_INIT_AUTOMAKE([1.9 tar-pax])
AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h]) 
dnl should include it in src files before using here

PROJECTDIR=`pwd`
AC_SUBST(PROJECTDIR)

if test "x$prefix" = "xNONE"; then
   INSTALLPREFIX=.
else
   INSTALLPREFIX="${prefix}"
fi

AC_SUBST(INSTALLPREFIX)

#Find psrlib path and read  version
PSRLIB_INC="$PROJECTDIR/inc"
psr_location="/usr/include /usr/local/include $PROJECTRDIR/inc $PROJECTRDIR/src/include"
for al in $psr_location
do
	AC_CHECK_FILE($al/psuservice.h)
	if [[ -f "$al/psuservice.h" ]]
	then
		PSR_INC=$al
	fi
	AC_CHECK_FILE($al/psu_ver.h)
	if [[ -f "$al/psu_ver.h" ]]
	then
		PSRLIB_VERSION=`grep "PSRLIB_VERSION \"" $al/psu_ver.h | cut -f3 -d ' ' | sed -e 's/"//g'`
		PSRLIB_MAJOR_VERSION=`echo ${PSRLIB_VERSION} | cut -f1 -d.`
		PSRLIB_MINOR_VERSION=`echo ${PSRLIB_VERSION} | cut -f2 -d.`
		PSRLIB_BUILD_VERSION=`echo ${PSRLIB_VERSION} | cut -f3 -d.`
		let PSRLIB_VER=${PSRLIB_MAJOR_VERSION}*10000+${PSRLIB_MINOR_VERSION}*100+${PSRLIB_BUILD_VERSION}
		echo "PSRLIB_VER is ${PSRLIB_VER}"
	fi
done
AC_SUBST(PSRLIB_INC)
AC_CHECK_FILE($PSRLIB_INC,, AC_MSG_ERROR([No psuservice.h found]))

DEBUG_THREADS="-DDUMP_SCHEDULER -DDEBUG_SCHEDULER -DDEBUG_THREADS -DDETECT_DEADLOCKS #-DDO_CRASH"
AC_SUBST(DEBUG_THREADS)

AC_PROG_CC
#AC_PROG_RANLIB				# 这里的意义是：当你使用了静态库的时候，需要这一句
#AC_PROG_LIBTOOL				# 如果你使用了共享库，那么使用AC_PROG_LIBTOOL
AC_LANG([C])

dnl not required as using libtool AC_PROG_RANLIB
AC_HEADER_STDC
AC_CHECK_HEADERS([netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_HEADER_DIRENT
AC_STRUCT_TM

dnl Checks for library functions.
#AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([localtime_r memmove memset select socket strcasecmp strchr strerror])

CFLAGS="$CFLAGS -I$PSRLIB_INC -I$PROJECTDIR/src/include"

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 ])
AC_OUTPUT
