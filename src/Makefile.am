include $(top_srcdir)/common.mk

AM_CPPFLAGS = ${libcurl_CFLAGS}
AM_LDFLAGS = -lcurl -lpthread -lhdf5_cpp -lhdf5

bin_PROGRAMS = cvedia

cvedia_SOURCES = md5.cpp cvedia.cpp api.cpp curlreader.cpp csvwriter.cpp hdf5writer.cpp caffeimagedata.cpp
cvedia_CPPFLAGS = $(AM_CPPFLAGS)
cvedia_LDADD = ${AM_LDFLAGS}
