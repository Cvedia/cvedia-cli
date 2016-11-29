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

PKG_CHECK_MODULES([CURL], [libcurl])
AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)

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
Support TFRecords output format:            $with_tfrecords
  (requires protobuf 3.0 or later)

])
