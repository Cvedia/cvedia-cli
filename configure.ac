AC_INIT([cvedia], [0.0.1])
AC_CONFIG_SRCDIR([src/cvedia.cpp])

INTI_REQUIRED_VERSION=1.0.7

PKG_CHECK_MODULES([CURL], [libcurl])
AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)

# check for C++ preprocessor and compiler and the library compiler
AC_PROG_CXXCPP
AC_PROG_CXX
AC_PROG_RANLIB

AM_INIT_AUTOMAKE([1.9])

AC_LANG([C++])
AC_HEADER_STDC

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])

AC_OUTPUT
