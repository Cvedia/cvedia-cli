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
apt-get -y install autoconf pkg-config vim automake libtool curl make g++ \
  unzip git gawk python3-dev python3-pip libcurl3 hdf5-tools hdf5-helpers \
  libcurl4-openssl-dev libhdf5-dev cmake build-essential libprotobuf-dev \
  libleveldb-dev libsnappy-dev libhdf5-serial-dev protobuf-compiler \
  lua5.2 liblua5.2-dev libssl-dev libsqlite3-dev ibgtk2.0-dev pkg-config \
  libavcodec-dev libavformat-dev libswscale-dev python-dev python-numpy \
  libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev \
  libdc1394-22-dev libglu1-mesa libgtkglext1 libhighgui-dev \
  libilmbase-dev libilmbase12 libopenexr-dev libopenexr22 libpangox-1.0-0 \
  libv4l-0 libv4lconvert0 libxmu6 libxt6
```
**Step 4:** Install necessary python pachages
```bash
pip3 install pycurl numpy h5py requests
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
**Step 6:** Install latest protobuf from git
```bash
mkdir /usr/src/cvedia
cd /usr/src/cvedia
git clone https://github.com/google/protobuf
cd /usr/src/cvedia/protobuf
./autogen.sh
./configure
make 
make check
make install
ldconfig 
```
**Step 7:** Install latest OpenCV from git
```bash
cd /usr/src/cvedia
wget https://github.com/opencv/opencv/archive/3.2.0.zip
unzip 3.2.0.zip
cd /usr/src/cvedia/opencv-3.2.0
mkdir release
cd /usr/src/cvedia/opencv-3.2.0/release
cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr ..
make 
make install
```
**Step 8:** Installation of latest libarchive from git
```bash
mkdir /usr/src/cvedia/libarchive
cd /usr/src/cvedia/libarchive
wget http://libarchive.org/downloads/libarchive-3.2.2.tar.gz
tar xzf libarchive-3.2.2.tar.gz 
cd /usr/src/cvedia/libarchive/libarchive-3.2.2
./configure
make
make check
make install
```
**Step 9:** Installation of latest cvedia-cli from git
```bash
cd /usr/src/cvedia
git clone https://github.com/Cvedia/cvedia-cli
cd /usr/src/cvedia/cvedia-cli
ACLOCAL_PATH="/usr/src/cvedia/cvedia-cli" autoreconf -if
autoheader
aclocal
autoconf
automake
./configure
make
make install
```

This concludes the installation procedure. If all the above went well, you can use ```cvedia``` from the command line to interface to Cvedia.


