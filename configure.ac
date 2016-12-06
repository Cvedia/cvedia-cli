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

AC_ARG_WITH([hdf5], 
  AS_HELP_STRING([--without-hdf5], [build without HDF5 (default: no)]))

AC_SEARCH_LIBS([H5T_open], [hdf5],
	if test x"$with_hdf5" != x"no"; then
		[AC_DEFINE(HAVE_HDF5,1,[define if HDF5 is supported.])  
		with_hdf5=yes
	fi
],
[AC_MSG_WARN([HDF5 not found; disabling HDF5 support]) 
	with_hdf5=no
]
)

AM_CONDITIONAL(HAVE_HDF5, test x"$with_hdf5" = x"yes")
AM_CONDITIONAL(HAVE_TFRECORDS, test x"$with_tfrecords" = x"yes")
AM_CONDITIONAL(HAVE_PYTHON, test x"$with_python" = x"yes")

PKG_CHECK_MODULES([CURL], [libcurl])

AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)

AC_SUBST(PYINCS)
AC_SUBST(PYLIBS)

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
Support HDF5 output format:                 $with_hdf5
  (requires libhdf5-dev)
Support TFRecords output format:            $with_python
  (requires python 3.1 or later)

])
