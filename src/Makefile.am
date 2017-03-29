include $(top_srcdir)/common.mk

if HAVE_TFRECORDS
%.pb.cc %.pb.h: %.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(builddir) $^
endif


AM_CPPFLAGS = ${libcurl_CFLAGS} -I$(PYINCS) -I$(NUMPY_INCLUDE_DIR)
AM_LDFLAGS = -lcurl -lpthread -larchive -L/usr/local/lib -L/usr/lib

if HAVE_HDF5
AM_CPPFLAGS += -I/usr/include/hdf5/serial
AM_LDFLAGS += -lhdf5_cpp -lhdf5 -L/usr/lib/x86_64-linux-gnu/hdf5/serial
endif

if HAVE_OPENCV
AM_CPPFLAGS += @OPENCV_CFLAGS@
AM_LDFLAGS += @OPENCV_LIBS@ -ltiff
endif

if HAVE_SQLITE
AM_LDFLAGS += -lsqlite3
endif

if HAVE_TFRECORDS
AM_LDFLAGS += -lprotobuf
endif

if HAVE_LUA
AM_LDFLAGS += -llua5.2
endif


bin_PROGRAMS = cvedia

cvedia_SOURCES = md5.cpp cvedia.cpp api.cpp curlreader.cpp csvwriter.cpp hdf5writer.cpp pythonwriter.cpp caffeimagedata.cpp functions.cpp pythonmodules.cpp metadb.cpp imagemean.cpp luamodules.cpp luawriter.cpp

cvedia_CPPFLAGS = $(AM_CPPFLAGS)
cvedia_LDADD = $(AM_LDFLAGS) $(PYLIBS)
