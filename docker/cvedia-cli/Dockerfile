FROM webdevops/base:ubuntu-16.04

RUN apt-get update
RUN export DEBIAN_FRONTEND=noninteractive
RUN apt-get -y install autoconf pkg-config vim automake libtool curl make g++ unzip git gawk python3-dev python3-pip libcurl3 hdf5-tools hdf5-helpers libcurl4-openssl-dev libhdf5-dev cmake build-essential 
RUN apt-get -y install libc6-dev-i386 g++-multilib #support for 32 bit install
RUN apt-get -y install libprotobuf-dev libleveldb-dev libsnappy-dev libhdf5-serial-dev protobuf-compiler lua5.2 liblua5.2-dev libssl-dev libsqlite3-dev ibgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-22-dev libopencv-dev
RUN pip3 install pycurl numpy h5py requests

RUN mkdir /home/root
WORKDIR /home/root
RUN git clone https://github.com/google/protobuf
WORKDIR /home/root/protobuf
RUN ./autogen.sh
RUN ./configure
RUN make
RUN make check
RUN make install
RUN ldconfig 

WORKDIR /home/root
RUN wget https://github.com/opencv/opencv/archive/3.2.0.zip
RUN unzip 3.2.0.zip
WORKDIR /home/root/opencv-3.2.0
RUN mkdir release
WORKDIR /home/root/opencv-3.2.0/release
RUN cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr ..
RUN make
RUN make install


RUN mkdir /home/root/libarchive
WORKDIR /home/root/libarchive
RUN wget http://libarchive.org/downloads/libarchive-3.2.2.tar.gz
RUN tar xzf libarchive-3.2.2.tar.gz 
WORKDIR /home/root/libarchive/libarchive-3.2.2
RUN ./configure
RUN make
RUN make check
RUN make install

WORKDIR /var/www/cvedia-cli
RUN ACLOCAL_PATH="/var/www/cvedia-cli" autoreconf -if
RUN autoheader
RUN aclocal
RUN autoconf
RUN automake
RUN ./configure
RUN make
