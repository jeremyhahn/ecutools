#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ecutune], [0.1], [root@localhost])
AM_INIT_AUTOMAKE([1.9 foreign])
AC_CONFIG_SRCDIR([src/ecutune.c])
AC_CONFIG_HEADERS([config.h])

if test -z $CFLAGS; then
    CFLAGS=''
fi

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

# Remember externally set CFLAGS
EXTERNAL_CFLAGS="$CFLAGS"

# Checks for libraries.
AC_CHECK_LIB([crypto], [main],, AC_MSG_ERROR($missing_library))
PKG_CHECK_MODULES([crypto], [libcrypto], [have_libcrypto=yes], [have_libcrypto=no])
AM_CONDITIONAL([CRYPTO],  [test "$have_libcrypto" = "yes"])

AC_CHECK_LIB([ssl], [main],, AC_MSG_ERROR($missing_library))
PKG_CHECK_MODULES([ssl], [libssl], [have_libssl=yes], [have_libssl=no])
AM_CONDITIONAL([SSL],  [test "$have_libssl" = "yes"])

AC_CHECK_LIB([pthread], [main],, AC_MSG_ERROR($missing_library))
PKG_CHECK_MODULES([pthread], [pthread], [have_libpthread=yes], [have_libpthread=no])
AM_CONDITIONAL([PTHREAD],  [test "$have_libpthread" = "yes"])

#AC_CHECK_LIB([wsclient], [main],, AC_MSG_ERROR($missing_library))
#PKG_CHECK_MODULES([wsclient], [wsclient], [have_libwsclient=yes], [have_libwsclient=no])
#AM_CONDITIONAL([WSCLIENT],  [test "$have_libwsclient" = "yes"])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset socket strcasecmp strchr strerror strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT


# Conditionals
AC_ARG_ENABLE(debug,
[--enable-debug    Turn on debugging],
[case "${enableval}" in
  yes) debug=true ;;
  no)  debug=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
esac],[debug=false])
if test "$debug" = true; then
    AC_DEFINE(ENABLE_DEBUG, [1], ["Compile with GDB support"])
    CFLAGS='-g'
fi

AC_ARG_ENABLE(ssl,
[--enable-ssl Enable ssl, default: enabled],
[case "${enableval}" in
  yes) ssl=true ;;
  no)  ssl=false ;;
  *)   AC_MSG_ERROR([bad value ${enableval} for --enable-ssl]) ;;
esac], [ssl=true])
if test "$ssl" = true; then
    AC_DEFINE(ENABLE_SSL, [1], ["Compile with SSL support"])
fi

AC_ARG_ENABLE(threads,
[--enable-threads    Enable multi-threading],
[case "${enableval}" in
  yes) threads=true ;;
  no)  threads=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-threads) ;;
esac],[threads=true])
if test "$threads" = true; then
    AC_DEFINE(ENABLE_THREADS, [1], ["Compile with client multi-threading support"])
fi

AC_OUTPUT