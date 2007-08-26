# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

# Process this file with autoconf to produce a configure script.
AUTOCONF_CONFIG_DIR="src/autoconf"
AC_INIT(src/bnetd/handle_bnet.cpp)
AC_CONFIG_AUX_DIR(src/autoconf)

AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE(pvpgn, 1.99.x-SVN)
AC_CONFIG_HEADER(config.h)

builtin(include,src/autoconf/ccstdc.m4)
builtin(include,src/autoconf/mkdir.m4)

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB

# Checks compiler.
AC_PROG_CPP

AC_LANG(C++)

# Check for sed
AC_CHECK_PROG(SED, sed, sed)
if test "$SED" != "sed"; then
	AC_MSG_ERROR([sed not found, please install sed])
fi

# Check for cp
AC_CHECK_PROG(CP, cp, cp)
if test "$CP" != "cp"; then
	AC_MSG_ERROR([cp not found, please install cp])
fi

# Checks for systems.
AC_AIX
AC_MINIX

# Custom checks.
zwarnflags=""
AC_ARG_WITH(warn, [  --with-warn             enable compiler warnings], [zwarnflags="-pedantic -Wall -W -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Wunreachable-code -Winline -Wpacked -Wno-sign-compare"])

zlangflags=""
AC_ARG_WITH(ansi, [  --with-ansi             use strict ANSI C++98 mode], [zlangflags="-std=c++98 -pedantic"])

extra_include=""
AC_ARG_WITH(includes, [  --with-includes=DIR     search include DIR for header files], [
case "x$withval" in
x/*|x.*)
  AC_MSG_RESULT([adding ${withval} to include search path])
  if test ! -d ${withval}; then
    AC_MSG_RESULT([Warning: directory ${withval} does not exist])
  fi
  extra_include="${extra_include} -I${withval}"
  ;;
*)
  AC_MSG_RESULT([not adding any includes])
  ;;
esac])

extra_lib_dir=""
AC_ARG_WITH(libraries, [  --with-libraries=DIR    search library DIR for libraries], [
case "x${withval}" in
x/*|x.*)
  AC_MSG_RESULT([adding ${withval} to library search path])
  if test ! -d ${withval}; then
    AC_MSG_RESULT([Warning: directory ${withval} does not exist])
  fi
  extra_lib_dir="${extra_lib_dir} -L${withval}"
  ;;
*)
  AC_MSG_RESULT([not adding any libs])
  ;;
esac])

extra_lib=""
AC_ARG_WITH(mysql, [  --with-mysql            include MySQL user accounts support])

AC_ARG_WITH(pgsql, [  --with-pgsql            include PostgreSQL user accounts support])

AC_ARG_WITH(mssql, [  --with-mssql            include MSSQL user accounts support (requires FreeTDS includes/libs)])

AC_ARG_WITH(sqlite3, [  --with-sqlite3          include SQLite3 user accounts support ])

AC_ARG_WITH(odbc, [  --with-odbc             include ODBC user accounts support ])

extra_define=""
dnl Enable poll()
AC_ARG_ENABLE(poll,
[  --enable-poll           Enable poll() instead of select().  Normally poll
                          is preferred over select, but configure knows poll
                          is broken on some platforms.  If you think you are
                          smarter than the configure script, you may enable
                          poll with this option.
  --disable-poll          Disable the use of poll().],
[
  case "$enableval" in
  yes)
    echo "Forcing poll() to be enabled"
    ac_cv_func_poll='yes'
    ;;
  no)
    echo "Forcing poll() to be disabled"
    ac_cv_func_poll='no'
    ;;
  esac
])

dnl Shamesly stolen from squid 2.3 configure.in
dnl This has to be before AC_CHECK_FUNCS
# Disable poll() on certain platforms. Override by setting ac_cv_func_poll
# when running configure.
if test -z "$ac_cv_func_poll"; then
        case "$host" in
		[alpha-dec-osf3.*])
			# John Kay (jkay@nlanr.net) 19970818
			echo "disabling poll for $host..."
			ac_cv_func_poll='no'
			;;
		[*-hp-hpux*.*])
			# Duane Wessels
			echo "disabling poll for $host..."
			ac_cv_func_poll='no'
			;;
		[*-linux-*])
			# Henrik Nordstrom (hno@hem.passagen.se) 19980817
			# poll is problematic on Linux.  We disable it
			# by default until Linux gets it right.
			rev=`uname -r | awk -F. '{printf "%03d%03d",$1,$2}'`
			if test $rev -lt 002002; then
			    echo "disabling poll for $host < 2.2..."
			    ac_cv_func_poll='no'
			fi
			;;
		[powerpc-ibm-aix4.1.*])
			# Mike Laster (mlaster@metavillage.com) 19981021
			echo "disabling poll for $host..."
			ac_cv_func_poll='no'
			;;
		[*-pc-sco3.2*])
			# Robert Side <rside@aiinc.bc.ca>
			# Mon, 18 Jan 1999 17:48:00 GMT
			echo "disabling poll for $host..."
			ac_cv_func_poll='no'
			;;
	esac
fi

# disable parts of the build process
AC_ARG_ENABLE(bnetd,
[  --enable-bnetd          Enable building of bnetd server (default)
  --disable-bnetd         Disable building of bnetd server])
AC_ARG_ENABLE(d2cs,
[  --enable-d2cs           Enable building of d2cs server (default)
  --disable-d2cs          Disable building of d2cs server])
AC_ARG_ENABLE(d2dbs,
[  --enable-d2dbs          Enable building of d2dbs server (default)
  --disable-d2dbs         Disable building of d2dbs server])

LDFLAGS="${LDFLAGS} ${extra_lib_dir}"
LIBS="${LIBS} ${extra_lib}"
CFLAGS="${CFLAGS} ${zlangflags} ${zwarnflags}"
DEFINES="${DEFINES} ${extra_define}"
CPPFLAGS="${CPPFLAGS} ${extra_include} -fno-strict-aliasing"

# This is our invention so we have to substitute it ourselves
# autoconf uses DEFS
AC_SUBST(DEFINES)
AM_CONDITIONAL([COND_BNETD], [test -z "$enable_bnetd" -o x"$enable_bnetd" = xyes])
AM_CONDITIONAL([COND_D2CS], [test -z "$enable_d2cs" -o x"$enable_d2cs" = xyes])
AM_CONDITIONAL([COND_D2DBS], [test -z "$enable_d2dbs" -o x"$enable_d2dbs" = xyes])

# Checks for libraries.
AC_CHECK_LIB(m, pow, [LIBS="$LIBS -lm"])

AC_CHECK_LIB(z, compress, [LIBS="$LIBS -lz"], [AC_MSG_ERROR([not found, please install the zlib libraries])])

mysql_include_path=""
dnl Detecting mysql paths
if test "$with_mysql" = "yes" ; then
	AC_MSG_CHECKING([for mysql include path])
	mysql_try_includes="/usr/include /usr/include/mysql /usr/local/include /usr/local/include/mysql /usr/local/mysql/include /usr/local/mysql/include/mysql"
	for i in `echo ${mysql_try_includes}`; do
	    if test -f "${i}/mysql.h" ; then
		CPPFLAGS="${CPPFLAGS} -I${i}"
		mysql_include_path=${i}
		break
	    fi
	done
	if test -n "$mysql_include_path" ; then
	    AC_MSG_RESULT([found in $mysql_include_path])
	else
	    AC_MSG_ERROR([not found, please install the MySQL headers])
	fi
fi

mysql_lib_path=""
if test "$with_mysql" = "yes" ; then
    AC_CHECK_LIB(mysqlclient, mysql_init , [DEFINES="${DEFINES} -DWITH_SQL_MYSQL" ; LIBS="${LIBS} -lmysqlclient"], [
	AC_MSG_CHECKING([for mysql lib path])
	mysql_try_libs="/usr/lib /usr/lib/mysql /usr/local/lib /usr/local/lib/mysql /usr/local/mysql/lib /usr/local/mysql/lib/mysql"
	found=""
	for i in `echo ${mysql_try_libs}`; do
	    if test -f "${i}/libmysqlclient.so" -o -f "${i}/libmysqlclient.a" ; then
		save_ldflags="${LDFLAGS}"
		LDFLAGS="${LDFLAGS} -L${i}"
		AC_CHECK_LIB(mysqlclient, mysql_close , [DEFINES="${DEFINES} -DWITH_SQL_MYSQL" ; LIBS="${LIBS} -lmysqlclient" ; mysql_lib_path="${i}"], [LDFLAGS="${save_ldflags}"])
		break
	    fi
	done
	if test -n "$mysql_lib_path" ; then
	    AC_MSG_RESULT([found in $mysql_lib_path])
	else
	    AC_MSG_ERROR([not found, please install the MySQL libraries])
	fi
    ])
fi

pgsql_include_path=""
dnl Detecting pgsql paths
if test "$with_pgsql" = "yes" ; then
	AC_MSG_CHECKING([for pgsql include path])
	pgsql_try_includes="/usr/include /usr/include/pgsql /usr/local/include /usr/local/include/pgsql /usr/local/pgsql/include"
	for i in `echo ${pgsql_try_includes}`; do
	    if test -f "${i}/libpq-fe.h" ; then
		CPPFLAGS="${CPPFLAGS} -I${i}"
		pgsql_include_path=${i}
		break
	    fi
	done
	if test -n "$pgsql_include_path" ; then
	    AC_MSG_RESULT([found in $pgsql_include_path])
	else
	    AC_MSG_ERROR([not found, please install the PostgreSQL includes])
	fi
fi

pgsql_lib_path=""
if test "$with_pgsql" = "yes" ; then
     AC_CHECK_LIB(pq, PQsetdbLogin , [DEFINES="${DEFINES} -DWITH_SQL_PGSQL" ; LIBS="${LIBS} -lpq"], [
	AC_MSG_CHECKING([for pgsql lib path])
	pgsql_try_libs="/usr/lib /usr/lib/pgsql /usr/local/lib /usr/local/lib/pgsql /usr/local/pgsql/lib"
	for i in `echo ${pgsql_try_libs}`; do
	    if test -f "${i}/libpq.so" -o -f "${i}/libpq.a" ; then
		save_ldflags="${LDFLAGS}"
		LDFLAGS="${LDFLAGS} -L${i}"
		AC_CHECK_LIB(pq, PQfinish , [DEFINES="${DEFINES} -DWITH_SQL_PGSQL" ; LIBS="${LIBS} -lpq" ; pgsql_lib_path="${i}"], [LDFLAGS="${save_ldflags}"])
		break
	    fi
	done
	if test -n "$pgsql_lib_path" ; then
	    AC_MSG_RESULT([found in $pgsql_lib_path])
	else
	    AC_MSG_ERROR([not found, please install the PostgreSQL libraries])
	fi
     ])
fi

mssql_include_path=""
dnl Detecting mssql paths
if test "$with_mssql" = "yes" ; then
	AC_MSG_CHECKING([for mssql/FreeTDS include path])
	mssql_try_includes="/usr/include /usr/include/freetds /usr/local/include /usr/local/include/freetds /usr/local/freetds/include"
	for i in `echo ${mssql_try_includes}`; do
	    if test -f "${i}/sqlfront.h" ; then
		CPPFLAGS="${CPPFLAGS} -I${i}"
		mssql_include_path=${i}
		break
	    fi
	done
	if test -n "$mssql_include_path" ; then
	    AC_MSG_RESULT([found in $mssql_include_path])
	else
	    AC_MSG_ERROR([not found, please install the FreeTDS includes])
	fi
fi

mssql_lib_path=""
if test "$with_mssql" = "yes" ; then
     AC_CHECK_LIB(sybdb, dbinit, [DEFINES="${DEFINES} -DWITH_SQL_MSSQL" ; LIBS="${LIBS} -lsybdb"], [
	AC_MSG_CHECKING([for mssql/FreeTDS lib path])
	mssql_try_libs="/usr/lib /usr/lib/freetds /usr/local/lib /usr/local/lib/freetds /usr/local/freetds/lib"
	for i in `echo ${mssql_try_libs}`; do
	    if test -f "${i}/libsybdb.so" -o -f "${i}/libsybdb.a" ; then
		save_ldflags="${LDFLAGS}"
		LDFLAGS="${LDFLAGS} -L${i}"
		AC_CHECK_LIB(sybdb, dbclose, [DEFINES="${DEFINES} -DWITH_SQL_MSSQL" ; LIBS="${LIBS} -lsybdb" ; mssql_lib_path="${i}"], [LDFLAGS="${save_ldflags}"])
		break
	    fi
	done
	if test -n "$mssql_lib_path" ; then
	    AC_MSG_RESULT([found in $mssql_lib_path])
	else
	    AC_MSG_ERROR([not found, please install the FreeTDS libraries])
	fi
     ])
fi

sqlite3_include_path=""
dnl Detecting sqlite3 paths
if test "$with_sqlite3" = "yes" ; then
	AC_MSG_CHECKING([for SQLite3 include path])
	sqlite3_try_includes="/usr/include /usr/include/sqlite3 /usr/local/include /usr/local/include/sqlite3 /usr/local/sqlite3/include"
	for i in `echo ${sqlite3_try_includes}`; do
	    if test -f "${i}/sqlite3.h" ; then
		CPPFLAGS="${CPPFLAGS} -I${i}"
		sqlite3_include_path=${i}
		break
	    fi
	done
	if test -n "$sqlite3_include_path" ; then
	    AC_MSG_RESULT([found in $sqlite3_include_path])
	else
	    AC_MSG_ERROR([not found, please install the SQLite3 includes])
	fi
fi

sqlite3_lib_path=""
if test "$with_sqlite3" = "yes" ; then
     AC_CHECK_LIB(sqlite3, sqlite3_open, [DEFINES="${DEFINES} -DWITH_SQL_SQLITE3" ; LIBS="${LIBS} -lsqlite3"], [
	AC_MSG_CHECKING([for SQLite3 lib path])
	sqlite3_try_libs="/usr/lib /usr/lib/sqlite3 /usr/local/lib /usr/local/lib/sqlite3 /usr/local/sqlite3/lib"
	for i in `echo ${sqlite3_try_libs}`; do
	    if test -f "${i}/libsqlite3.so" -o -f "${i}/libsqlite3.a" ; then
		save_ldflags="${LDFLAGS}"
		LDFLAGS="${LDFLAGS} -L${i}"
		AC_CHECK_LIB(sqlite3, sqlite3_close, [DEFINES="${DEFINES} -DWITH_SQL_SQLITE3" ; LIBS="${LIBS} -lsqlite3" ; sqlite3_lib_path="${i}"], [LDFLAGS="${save_ldflags}"])
		break
	    fi
	done
	if test -n "$sqlite3_lib_path" ; then
	    AC_MSG_RESULT([found in $sqlite3_lib_path])
	else
	    AC_MSG_ERROR([not found, please install the SQLite3 libraries])
	fi
     ])
fi

odbc_include_path=""
dnl Detecting odbc paths
if test "$with_odbc" = "yes" ; then
	AC_MSG_CHECKING([for odbc include path])
	odbc_try_includes="/usr/include /usr/include/odbc /usr/local/include /usr/local/include/odbc /usr/local/odbc/include"
	for i in `echo ${odbc_try_includes}`; do
	    if test -f "${i}/sqlext.h" ; then
		CPPFLAGS="${CPPFLAGS} -I${i}"
		odbc_include_path=${i}
		break
	    fi
	done
	if test -n "$odbc_include_path" ; then
	    AC_MSG_RESULT([found in $odbc_include_path])
	else
	    AC_MSG_ERROR([not found, please install the odbc includes])
	fi
fi

odbc_lib_path=""
if test "$with_odbc" = "yes" ; then
     AC_CHECK_LIB(odbc, SQLConnect, [DEFINES="${DEFINES} -DWITH_SQL_ODBC" ; LIBS="${LIBS} -lodbc"], [
	AC_MSG_CHECKING([for odbc lib path])
	odbc_try_libs="/usr/lib /usr/lib/odbc /usr/local/lib /usr/local/lib/odbc /usr/local/odbc/lib"
	for i in `echo ${odbc_try_libs}`; do
	    if test -f "${i}/libodbc.so" -o -f "${i}/libodbc.a" ; then
		save_ldflags="${LDFLAGS}"
		LDFLAGS="${LDFLAGS} -L${i}"
		AC_CHECK_LIB(odbc, SQLDisconnect, [DEFINES="${DEFINES} -DWITH_SQL_ODBC" ; LIBS="${LIBS} -lodbc" ; odbc_lib_path="${i}"], [LDFLAGS="${save_ldflags}"])
		break
	    fi
	done
	if test -n "$odbc_lib_path" ; then
	    AC_MSG_RESULT([found in $odbc_lib_path])
	else
	    AC_MSG_ERROR([not found, please install the odbc libraries])
	fi
     ])
fi

AC_CHECK_LIB(nsl, gethostbyname, [LIBS="$LIBS -lnsl"])
AC_CHECK_LIB(socket, socket, [LIBS="$LIBS -lsocket"])
AC_MSG_CHECKING([for select in -lws2_32])
save_libs=$LIBS
LIBS="$LIBS -lws2_32"
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([[#include <winsock2.h>]],[[select(0,0,0,0,0);]])], 
	AC_MSG_RESULT( [yes] ) , 
	[ LIBS=$save_libs; AC_MSG_RESULT( [no] ) ] )
AC_CHECK_LIB(resolv, inet_aton, [LIBS="$LIBS -lresolv"])
dnl BeOS BONE...
AC_CHECK_LIB(bind, __inet_aton, [LIBS="$LIBS -lbind"])
case "$host" in
	[*-beos*])
		if test "$ac_cv_lib_bind___inet_aton" = "yes"; then
			dnl force things, AC_CHECK_FUNC will never get the mangled versions anyway.
			echo "Forcing inet_aton() and inet_ntoa() to be enabled"
			ac_cv_func_inet_aton='yes'
			ac_cv_func_inet_ntoa='yes'
		fi
		;;
esac
AC_CHECK_LIB(pcap, pcap_open_offline)
AM_CONDITIONAL([COND_BNPCAP], [test "$ac_cv_lib_pcap_pcap_open_offline" = "yes"])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STAT
dnl checking for STD C++98 + STD C90 headers, these are mandatory
AC_CHECK_HEADERS([\
	cassert \
	cctype \
	cerrno \
	climits \
	cmath \
	csignal \
	signal.h \
	cstdarg \
	cstddef \
	cstdio \
	cstdlib \
	cstring \
	ctime \
],, AC_MSG_ERROR(Standard C90/C++98 header missing, you need a fully standard compliant compiler/enviroment.))

AC_CHECK_HEADERS([\
	fcntl.h \
	sys/time.h \
	sys/select.h \
	unistd.h \
	sys/utsname.h \
	sys/timeb.h \
	sys/socket.h \
	sys/param.h \
	netinet/in.h \
	arpa/inet.h \
	netdb.h \
	termios.h \
	sys/types.h \
	sys/wait.h \
	sys/ioctl.h \
	stdint.h \
	sys/file.h \
	poll.h \
	sys/poll.h \
	stropts.h \
	sys/stropts.h \
	sys/stat.h \
	pwd.h \
	grp.h \
	dir.h \
	direct.h \
	sys/mman.h \
	sys/event.h \
	sys/epoll.h \
	sys/resource.h \
	sqlite3.h \
	pcap.h \
])

# Checks for typedefs and structures
AC_TYPE_PID_T
AC_TYPE_OFF_T
AC_CHECK_SIZEOF(unsigned char, 1)
AC_CHECK_SIZEOF(unsigned short, 2)
AC_CHECK_SIZEOF(unsigned int, 4)
AC_CHECK_SIZEOF(unsigned long, 4)
AC_CHECK_SIZEOF(unsigned long long, 0)
AC_CHECK_SIZEOF(signed char, 1)
AC_CHECK_SIZEOF(signed short, 2)
AC_CHECK_SIZEOF(signed int, 4)
AC_CHECK_SIZEOF(signed long, 4)
AC_CHECK_SIZEOF(signed long long, 0)

# Checks for library functions.
AC_FUNC_MMAP
AC_FUNC_VPRINTF
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MKDIR_ARGS
AC_CHECK_FUNCS([\
	gethostname \
	gettimeofday \
	select \
	socket \
	strdup \
	strtoul \
	inet_aton \
	inet_ntoa \
	uname \
	recv \
	send \
	recvfrom \
	sendto \
	uname \
	fork \
	getpid \
	sigaction \
	sigprocmask \
	sigaddset \
	setpgid \
	ftime \
	strcasecmp \
	strncasecmp \
	stricmp \
	strnicmp \
	chdir \
	difftime \
	strchr \
	strrchr \
	index \
	rindex \
	wait \
	waitpid \
	pipe \
	getenv \
	ioctl \
	setsid \
	poll \
	gethostbyname \
	getservbyname \
	getlogin \
	getpwnam \
	getgrnam \
	getuid \
	getgid \
	setuid \
	setgid \
	mkdir \
	_mkdir \
	strsep \
	getopt \
	kqueue \
	setitimer \
	epoll_create \
	getrlimit \
	vsnprintf \
	_vsnprintf \
	snprintf \
	_snprintf \
])
AC_CHECK_FUNC([setpgrp], [AC_FUNC_SETPGRP])

AC_OUTPUT([
	Makefile \
	src/Makefile \
	src/compat/Makefile \
	src/common/Makefile \
	src/tinycdb/Makefile \
	src/bnetd/Makefile \
	src/d2cs/Makefile \
	src/d2dbs/Makefile \
	src/bntrackd/Makefile \
	src/client/Makefile \
	src/bnpass/Makefile \
	src/bniutils/Makefile \
	src/bnpcap/Makefile \
	man/Makefile \
	conf/Makefile \
	docs/Makefile \
	Win32-Projects/Makefile \
	files/Makefile \
	src/autoconf/Makefile \
	src/win32/Makefile \
])
