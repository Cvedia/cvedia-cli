AC_INIT([cvedia], [0.0.1])
AC_CONFIG_SRCDIR([src/cvedia.cpp])

INTI_REQUIRED_VERSION=1.0.7

AM_INIT_AUTOMAKE([-Wno-portability foreign subdir-objects])

AC_CONFIG_HEADERS(config.hpp)

AC_CHECK_PROG([PROTOC], [protoc], [protoc])
AS_IF([test "x${PROTOC}" == "x"],
    [AC_MSG_ERROR([ProtoBuf compiler "protoc" not found.])])

AC_ARG_WITH([tfrecords], 
  AS_HELP_STRING([--without-tfrecords], [build without TFRecords (default: no)]))

AC_ARG_WITH([python], 
  AS_HELP_STRING([--without-python], [build without Python support (default: no)]))

AC_ARG_WITH([lua], 
  AS_HELP_STRING([--without-lua], [build without Lua support (default: no)]))

PKG_CHECK_MODULES(PROTOBUF, protobuf >= 3.0.0,
	if test x"$with_tfrecords" != x"no"; then
		[AC_DEFINE(HAVE_TFRECORDS,1,[define if TFRecords is supported.])  
		with_tfrecords=yes
		PACKAGES_USED="$PACKAGES_USED protobuf"   
	fi
],
[AC_MSG_WARN([protobuf 3.0 not found; disabling TFRecords support]) 
	with_tfrecords=no
]
)

PKG_CHECK_MODULES(PYTHON, python3 >= 3.1, 
	if test x"$with_python" != x"no"; then
		[AC_DEFINE([HAVE_PYTHON], 1, [Define if you have 'Python' development SDK])
		with_python=yes
		PYLIBS=`python3.5m-config --libs`
		PYINCS=`python3.5m-config --includes`
		PACKAGES_USED="$PACKAGES_USED python"   
	fi
],
[AC_MSG_WARN([python3 >= 3.1 not found; disabling Python support]) 
	with_python=no
]
)

# Check for numpy
AC_MSG_CHECKING([for Numpy include directory])
NUMPY_INCLUDE_DIR=`echo "import numpy; print(numpy.get_include())" | python3 - 2>/dev/null`
CPPFLAGS=-I$PYINCS $CPPFLAGS
AC_SUBST(NUMPY_INCLUDE_DIR)
AC_CHECK_HEADER([${NUMPY_INCLUDE_DIR}/numpy/arrayobject.h], [NUMPY_HEADER=yes],
			[AC_MSG_WARN([Numpy extension header not found])], [#include "Python.h"])

AC_SUBST(NUMPY_HEADER)

AC_ARG_WITH([hdf5_cpp], 
  AS_HELP_STRING([--without-hdf5], [build without HDF5 (default: no)]))

AC_SEARCH_LIBS([H5Fopen], [hdf5],
	if test x"$with_hdf5" != x"no"; then
		[LIBS="-lhdf5_cpp";]
		[AC_DEFINE(HAVE_HDF5,1,[Define if HDF5 is supported.])  
		with_hdf5=yes
	fi
],
[AC_MSG_WARN([HDF5 not found; disabling HDF5 support]) 
	with_hdf5=no
]
)

PKG_CHECK_MODULES(OPENCV,
		  opencv,
		  HAVE_OPENCV=yes, HAVE_OPENCV=no)

AC_SUBST(OPENCV_CFLAGS)
AC_SUBST(OPENCV_LIBS)

if test "x$HAVE_OPENCV" = "xno"; then
  AC_MSG_ERROR(OpenCV libraries could not be found)
fi

#### Find OpenSSL
AC_MSG_CHECKING([for --with-openssl])
AC_ARG_WITH(
	[openssl],
	[AS_HELP_STRING([--with-openssl],[/absolute/path/to/openssl-base])],
	[ LDFLAGS="$LDFLAGS -L$with_openssl/lib";
	  CFLAGS="$CFLAGS -I$with_openssl/include/";
	  LIBS="-lssl";
	  AC_MSG_RESULT([$with_openssl])
	],
	[AC_MSG_RESULT([no])]
)

AC_CHECK_LIB([crypto],[CRYPTO_new_ex_data], [], [AC_MSG_ERROR([OpenSSL libraries required])])

AC_CHECK_HEADERS([openssl/crypto.h openssl/x509.h openssl/pem.h openssl/ssl.h openssl/err.h],[],[AC_MSG_ERROR([OpenSSL headers required])])

PKG_CHECK_MODULES([SQLITE3], [sqlite3], [have_libsqlite3=yes], [have_libsqlite3=no])
AM_CONDITIONAL([LIB_SQLITE3],  [test "$have_libsqlite3" = "yes"])

## Check for lua
PKG_CHECK_MODULES(Lua, lua52, 
    if test x"$with_lua" != x"no"; then
        [AC_DEFINE([HAVE_LUA], 1, [Define if you have 'Lua' development SDK])
        with_lua=yes
        LUALIBS="-llua5.2 -lpthread -ldl  -lutil -lm "
        LUAINCS="/usr/include/lua5.2"
        PACKAGES_USED="$PACKAGES_USED lua52"   
        CPPFLAGS=-"I$LUAINCS $CPPFLAGS"
    fi
],
[AC_MSG_WARN([lua52 not found; disabling Lua support]) 
    with_lua=no
]
)


AM_CONDITIONAL(HAVE_HDF5, test x"$with_hdf5" = x"yes")
AM_CONDITIONAL(HAVE_TFRECORDS, test x"$with_tfrecords" = x"yes")
AM_CONDITIONAL(HAVE_PYTHON, test x"$with_python" = x"yes")
AM_CONDITIONAL(HAVE_LUA, test x"$with_lua" = x"yes")
AM_CONDITIONAL(HAVE_OPENCV, test x"$HAVE_OPENCV" = x"yes")

AM_CONDITIONAL(HAVE_SQLITE, test x"$have_libsqlite3" = x"yes")

PKG_CHECK_MODULES([CURL], [libcurl])

AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)

AC_SUBST(PYINCS)
AC_SUBST(PYLIBS)

AC_SUBST(LUAINCS)
AC_SUBST(LUALIBS)

# check for C++ preprocessor and compiler and the library compiler
AC_PROG_CXXCPP
AC_PROG_CXX
AC_PROG_RANLIB

AC_LANG([C++])
AC_HEADER_STDC

AC_OUTPUT([
		Makefile
		src/Makefile
])

AC_MSG_RESULT([dnl

* optional dependencies  
Support Sqlite3:                            $have_libsqlite3
  (requires libsqlite3-dev)
Support HDF5 output format:                 $with_hdf5
  (requires libhdf5-dev)
Support TFRecords output format:            $with_python
  (requires python 3.1 or later)
Support LUA output format:                 $with_lua
  (requires liblua5.2-dev)

])
