AC_INIT([hcs], [0.14.8], [qball@gmpclient.org])

AC_CONFIG_SRCDIR([src/hcs.cc])
AC_CONFIG_HEADER([config.h])

##
# Setup automake to be silent and in foreign mode.
##
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AM_SILENT_RULES([yes])

##
# Check for compiler
##
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11 
AM_PROG_CC_C_O


##
# Readline
##

AC_CHECK_LIB([readline], [readline])
AC_CHECK_LIB([rt], [clock_nanosleep],[], AC_MSG_ERROR("Require realtime (-lrt) support"))
AC_CHECK_HEADERS(readline/history.h readline/readline.h)

AC_CHECK_LIB([udev], [udev_new],[
AC_SUBST(libudev_LIBS, "-ludev")
AC_CHECK_HEADERS(libudev.h)
], [
AC_MSG_NOTICE([UDev is not found, autodetecting power supplies is disabled])
])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
