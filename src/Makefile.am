include $(top_srcdir)/common.mk

if HAVE_TFRECORDS
%.pb.cc %.pb.h: %.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(builddir) $^
endif

AM_CPPFLAGS = ${libcurl_CFLAGS} -I$(PYINCS) -I$(NUMPY_INCLUDE_DIR) -I/usr/local/include/opencv -I/usr/local/include
AM_LDFLAGS = -lcurl -lpthread -larchive -L/usr/local/lib -lopencv_shape -lopencv_stitching -lopencv_objdetect -lopencv_superres -lopencv_videostab -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_video -lopencv_photo -lopencv_ml -lopencv_imgproc -lopencv_flann -lopencv_core

if HAVE_HDF5
AM_LDFLAGS += -lhdf5_cpp -lhdf5
endif

if HAVE_OPENCV
AM_LDFLAGS += -lopencv_core -lopencv_imgproc -lopencv_features2d -lopencv_highgui -ltiff
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

cvedia_SOURCES = md5.cpp cvedia.cpp api.cpp curlreader.cpp csvwriter.cpp pythonwriter.cpp caffeimagedata.cpp functions.cpp pythonmodules.cpp metadb.cpp imagemean.cpp luamodules.cpp luawriter.cpp

cvedia_CPPFLAGS = $(AM_CPPFLAGS)
cvedia_LDADD = $(AM_LDFLAGS) $(PYLIBS)
