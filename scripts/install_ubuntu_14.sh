#!/bin/bash

echo Instaling system packages...

apt-get update
apt-get -y install autoconf build-essential cmake curl gawk git \
  hdf5-tools ibgtk2.0-dev libarchive-dev libavcodec-dev libavformat-dev \
  libcurl4-openssl-dev libdc1394-22-dev libglu1-mesa libgtkglext1 \
  libhdf5-serial-dev libhighgui-dev libilmbase-dev libjasper-dev \
  libjpeg-dev libleveldb-dev liblua5.2-dev libopencv-dev libopenexr6 \
  libopenexr-dev libpangox-1.0-0 libpng-dev libprotobuf-dev \
  libsnappy-dev libsqlite3-dev libssl-dev libswscale-dev libtbb-dev \
  libtiff-dev libtool libv4l-0 libv4lconvert0 libxmu6 libxt6 \
  lua5.2 lua-md5 pkg-config protobuf-compiler python3-h5py \
  python3-pip python3-pycurl python3-requests python-dev python-h5py \
  python-pycurl python-requests rsync unzip wget

echo Installing lua packages...

mkdir -p /usr/src/cvedia/luarocks && cd /usr/src/cvedia/luarocks
wget http://luarocks.github.io/luarocks/releases/luarocks-2.4.2.tar.gz
tar xzf luarocks-2.4.2.tar.gz
cd /usr/src/cvedia/luarocks/luarocks-2.4.2
./configure --prefix=/usr && \
  make build -j4 && \
  make install && \
  luarocks install serpent

read -p "Install tensorflow? [Y/N] " -n 1 -r
if [[ $REPLY =~ [Yy]$ ]]; then
	echo
	pip3 install https://storage.googleapis.com/tensorflow/linux/cpu/tensorflow-0.7.1-cp34-none-linux_x86_64.whl
fi

echo
echo "Installing cvedia-cli..."

apt-get -y install software-properties-common python-software-properties
add-apt-repository ppa:ubuntu-toolchain-r/test
apt-get update && apt-get -y install gcc-5 g++-5
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 1 --slave /usr/bin/g++ g++ /usr/bin/g++-5

cd /usr/src/cvedia/cvedia-cli
sed -i s/python3.5m-config/python3.4m-config/g configure.ac
make clean
ACLOCAL_PATH="/usr/src/cvedia/cvedia-cli" autoreconf -if && \
	autoheader && \
	aclocal && \
	autoconf && \
	automake && \
	./configure && \
	make -j4 && \
	make install

echo "Completed, use 'cvedia -h' to see all options"
