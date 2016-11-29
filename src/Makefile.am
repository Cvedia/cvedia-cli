include $(top_srcdir)/common.mk

if HAVE_TFRECORDS
%.pb.cc %.pb.h: %.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(builddir) $^
endif

AM_CPPFLAGS = ${libcurl_CFLAGS} -I$(builddir)
AM_LDFLAGS = -lcurl -lpthread -lhdf5_cpp -lhdf5

bin_PROGRAMS = cvedia

cvedia_SOURCES = md5.cpp cvedia.cpp api.cpp curlreader.cpp csvwriter.cpp tfrecordswriter.cpp hdf5writer.cpp caffeimagedata.cpp

cvedia_CPPFLAGS = $(AM_CPPFLAGS)
cvedia_LDADD = ${AM_LDFLAGS}

if HAVE_TFRECORDS
dist_noinst_DATA = example.proto
nodist_cvedia_SOURCES = $(builddir)/example.pb.cc $(builddir)/example.pb.h

MOSTLYCLEANFILES = example.pb.cc example.pb.h
BUILT_SOURCES = example.pb.h
endif
