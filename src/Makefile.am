include $(top_srcdir)/common.mk

AM_CPPFLAGS = ${libcurl_CFLAGS}
AM_LDFLAGS = -lcurl

bin_PROGRAMS = cvedia

cvedia_SOURCES = cvedia.cpp curlreader.cpp
cvedia_CPPFLAGS = $(AM_CPPFLAGS)
cvedia_LDADD = ${AM_LDFLAGS}