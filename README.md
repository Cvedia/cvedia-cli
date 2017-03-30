# Cvedia CLI

After setting up your project on https://cvedia.com/ you have the ability to Export your training set. The export provides you with a command line argument that can be used to run this tool. The CLI takes care of downloading and storing all the content you have predefined in your project.

Various storage engines are supported for writing your training set to disk. The list below will be expanded once more engines are made available:

**Caffe**
- CaffeImageData
- HDF5

**TensorFlow**
- TFRecords

**Generic**
- CSV

The TFRecords engine also serves as an example on how to write your own custom Python output engine. 

# Installation

## Prerequisites ##
The following installation guide is created and tested for a fresh Ubuntu 16.04 installation.

This is the installation procedure
**Step 1:** Prepare the system
```bash
apt-get update
```
**Step 2:** Install i386 libraries (optional)
```bash
apt-get -y install libc6-dev-i386 g++-multilib
```
**Step 3:** Install necessary system packages
```bash
apt-get -y install autoconf build-essential cmake curl gawk git \
  hdf5-tools ibgtk2.0-dev libarchive-dev libavcodec-dev libavformat-dev \
  libcurl4-openssl-dev libdc1394-22-dev libglu1-mesa libgtkglext1 \
  libhdf5-serial-dev libhighgui-dev libilmbase-dev libjasper-dev \
  libjpeg-dev libleveldb-dev liblua5.2-dev libopencv-dev libopenexr22 \
  libopenexr-dev libpangox-1.0-0 libpng-dev libprotobuf-dev \
  libsnappy-dev libsqlite3-dev libssl-dev libswscale-dev libtbb-dev \
  libtiff-dev libtool libv4l-0 libv4lconvert0 libxmu6 libxt6 lua5.2 \
  lua-md5 luarocks pkg-config protobuf-compiler python3-h5py \
  python3-pip python3-pycurl python3-requests python-dev python-h5py \
  python-pycurl python-requests unzip
```
**Step 4:** Install necessary lua packages
```bash
luarocks install serpent
```
**Step 5:** Install tensorflow or tensorflox-gpu
Pick ONE of the following depending if you'll utilize GPU or not.
```bash
pip3 install tensorflow
```
-or-
```bash
pip3 install tensorflow-gpu
```
**Step 6:** Installation of latest cvedia-cli from git
```bash
mkdir /usr/src/cvedia
cd /usr/src/cvedia
git clone https://github.com/Cvedia/cvedia-cli
cd /usr/src/cvedia/cvedia-cli
ACLOCAL_PATH="/usr/src/cvedia/cvedia-cli" autoreconf -if
autoheader
aclocal
autoconf
automake
LDFLAGS=-L/usr/lib/x86_64-linux-gnu/hdf5/serial CFLAGS=-I/usr/include/hdf5/serial ./configure
make
make install
```

This concludes the installation procedure. If all the above went well, you can use ```cvedia``` from the command line to interface to Cvedia.


