include $(top_srcdir)/common.mk

if HAVE_TFRECORDS
%.pb.cc %.pb.h: %.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(builddir) $^
endif

AM_CPPFLAGS = ${libcurl_CFLAGS} -I$(PYINCS) -I$(NUMPY_INCLUDE_DIR)
AM_LDFLAGS = -lcurl -lpthread

if HAVE_HDF5
AM_LDFLAGS += -lhdf5_cpp -lhdf5
endif

if HAVE_TFRECORDS
AM_LDFLAGS += -lprotobuf
endif

bin_PROGRAMS = cvedia

cvedia_SOURCES = md5.cpp cvedia.cpp api.cpp curlreader.cpp csvwriter.cpp tfrecordswriter.cpp hdf5writer.cpp pythonwriter.cpp caffeimagedata.cpp

cvedia_CPPFLAGS = $(AM_CPPFLAGS)
cvedia_LDADD = ${AM_LDFLAGS} $(PYLIBS)

if HAVE_TFRECORDS
dist_noinst_DATA = example.proto feature.proto
nodist_cvedia_SOURCES = @builddir@/example.pb.cc @builddir@/example.pb.h @builddir@/feature.pb.cc @builddir@/feature.pb.h

MOSTLYCLEANFILES = example.pb.cc example.pb.h feature.pb.cc feature.pb.h
BUILT_SOURCES = example.pb.h feature.pb.h
endif
