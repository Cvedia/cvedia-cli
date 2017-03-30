# Base Dockerfile to provide dockerized cvedia app
#
#  Build:  docker build -t cvedia-cli ./
#
#  Usage:  docker run --rm --net=host \
#   --volume [path]:[path] \
#   cvedia-cli [params]
#
#  Usage (gpu): docker run --rm --net=host \
#   --device /dev/nvidia0:/dev/nvidia0 \
#   --device /dev/nvidiactl:/dev/nvidiactl \
#   --device /dev/nvidia-uvm:/dev/nvidia-uvm \
#   --volume [path]:[path] \
#   cvedia-cli [params]
# 
FROM ubuntu:latest

ENV tnsrpkg  tensorflow
#ENV tnsrpkg tensorflow-gpu

RUN apt-get update && \
  apt-get -y install autoconf build-essential cmake curl gawk git \
    hdf5-tools ibgtk2.0-dev libarchive-dev libavcodec-dev libavformat-dev \
    libcurl4-openssl-dev libdc1394-22-dev libglu1-mesa libgtkglext1 \
    libhdf5-serial-dev libhighgui-dev libilmbase-dev libjasper-dev \
    libjpeg-dev libleveldb-dev liblua5.2-dev libopencv-dev libopenexr22 \
    libopenexr-dev libpangox-1.0-0 libpng-dev libprotobuf-dev \
    libsnappy-dev libsqlite3-dev libssl-dev libswscale-dev libtbb-dev \
    libtiff-dev libtool libv4l-0 libv4lconvert0 libxmu6 libxt6 \
    lua5.2 lua-md5 pkg-config protobuf-compiler python3-h5py \
    python3-pip python3-pycurl python3-requests python-dev python-h5py \
    python-pycurl python-requests rsync unzip wget

RUN mkdir -p /usr/src/cvedia/luarocks && cd /usr/src/cvedia/luarocks && \
  wget http://luarocks.github.io/luarocks/releases/luarocks-2.4.2.tar.gz && \
  tar xzf luarocks-2.4.2.tar.gz && \
  cd /usr/src/cvedia/luarocks/luarocks-2.4.2 && \
  ./configure --prefix=/usr && \
  make build && \
  make install && \
  luarocks install serpent

RUN pip3 install ${tnsrpkg}

COPY  cvedia.patch /usr/src/cvedia

RUN mkdir -p /usr/src/cvedia && cd /usr/src/cvedia && \
  git clone https://github.com/Cvedia/cvedia-cli && \
  cd /usr/src/cvedia/cvedia-cli && \
  ACLOCAL_PATH="/usr/src/cvedia/cvedia-cli" autoreconf -if && \
  autoheader && \
  aclocal && \
  autoconf && \
  automake && \
  LDFLAGS=-L/usr/lib/x86_64-linux-gnu/hdf5/serial CFLAGS=-I/usr/include/hdf5/serial ./configure && \
  make -j 4 && \
  make install

WORKDIR /usr/src/cvedia/cvedia-cli/src

VOLUME ["/data"]

ENTRYPOINT ["/usr/local/bin/cvedia","-d","/data"]
