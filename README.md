# Cvedia CLI

After setting up your project on [CVEDIA](https://cvedia.com/) you have the ability to Export your training set. The export provides you with a command line argument that can be used to run this tool. The CLI takes care of downloading and storing all the content you have predefined in your project.

Various storage engines are supported for writing your training set to disk. The list below will be expanded once more engines are made available:

**Caffe**
- CaffeImageData
- HDF5

**TensorFlow**
- TFRecords

**Generic**
- CSV

The TFRecords engine also serves as an example on how to write your own custom Python output engine. 

----

Following you can find installation procedures for:
- [Docker](#docker)
- [Ubuntu 16.04 Xenial](#u16)
- [Ubuntu 14.04 Trusty](#u14)
- [Windows 10](#w10)

<a name=docker></a>
# Installation for docker (Recommended) #

The easiest way to get cvedia-cli working is thuru [docker](https://www.docker.com/community-edition), you don't need to worry about compiling and installing packages.

### One line install:

```bash
docker build https://raw.githubusercontent.com/Cvedia/cvedia-cli/master/docker/cvedia-cli/Dockerfile -t cvedia-cli
```

Basic usage:

```bash
docker run --rm --net=host --volume /data:/data cvedia-cli  -j <jobID> -b <batch size> -o <output format>
```

Example:
```bash
docker run --rm --net=host --volume /data:/data cvedia-cli  -j CVT2CUg5M7RwcOXsz8UcQZipQbMPMQDz -b 500 -o CSV
```

For more information on the dockerized installation you can use the provided [Dockerfile](https://github.com/Cvedia/cvedia-cli/tree/master/docker) and its separate usage guide. 


This concludes the installation procedure. If all the above went well, you can use ```cvedia``` from the command line to interface to Cvedia.

<a name=u16></a>
# Installation for Ubuntu 16.04

## Prerequisites ##
The following installation guide is created and tested for a fresh Ubuntu 16.04 installation.

## Install thuru bash script ##

```bash
mkdir -p /usr/src/cvedia && cd /usr/src/cvedia
git clone https://github.com/Cvedia/cvedia-cli
cd /usr/src/cvedia/cvedia-cli/scripts
./install_ubuntu_16.sh
```

## Install step by step ##

**Step 1:** Prepare the system
```bash
apt-get update
```
**Step 2:** Install necessary system packages
```bash
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
```
**Step 3:** Install necessary lua packages
```bash
mkdir -p /usr/src/cvedia/luarocks && cd /usr/src/cvedia/luarocks
wget http://luarocks.github.io/luarocks/releases/luarocks-2.4.2.tar.gz
tar xzf luarocks-2.4.2.tar.gz
cd /usr/src/cvedia/luarocks/luarocks-2.4.2
./configure --prefix=/usr && \
  make build && \
  make install && \
  luarocks install serpent
```
**Step 4:** Install tensorflow or tensorflox-gpu
Pick ONE of the following depending if you'll utilize GPU or not.
```bash
pip3 install tensorflow
```
-or-
```bash
pip3 install tensorflow-gpu
```
**Step 5:** Installation of latest cvedia-cli from git
```bash
mkdir -p /usr/src/cvedia && cd /usr/src/cvedia
git clone https://github.com/Cvedia/cvedia-cli
cd /usr/src/cvedia/cvedia-cli
ACLOCAL_PATH="/usr/src/cvedia/cvedia-cli" autoreconf -if && \
  autoheader && \
  aclocal && \
  autoconf && \
  automake && \
  LDFLAGS=-L/usr/lib/x86_64-linux-gnu/hdf5/serial CFLAGS=-I/usr/include/hdf5/serial ./configure && \
  make -j 4 && \
  make install
```

<a name=u14></a>
# Installation for Ubuntu 14.04

## Prerequisites ##
The following installation guide is created and tested for a fresh Ubuntu 14.04 installation. The list below points out the differences from the Ubuntu 16 installation.

- Install gcc 5 
- Install libopenexr6 instead of libopenexr22 
- Install specific version of tensorflow 
- Minor patch in cvedia-cli source to    compile with python 3.4

## Install thuru bash script ##

```bash
mkdir -p /usr/src/cvedia && cd /usr/src/cvedia
git clone https://github.com/Cvedia/cvedia-cli
cd /usr/src/cvedia/cvedia-cli/scripts
./install_ubuntu_14.sh
```

## Install step by step ##

**Step 1:** Prepare the system
```bash
apt-get update
```
**Step 2:** Install necessary system packages
```bash
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
```
**Step 3:** Install necessary lua packages
```bash
mkdir -p /usr/src/cvedia/luarocks && cd /usr/src/cvedia/luarocks
wget http://luarocks.github.io/luarocks/releases/luarocks-2.4.2.tar.gz
tar xzf luarocks-2.4.2.tar.gz
cd /usr/src/cvedia/luarocks/luarocks-2.4.2
./configure --prefix=/usr
make build
make install
luarocks install serpent
```
**Step 4:** Install specific tensorflow
```bash
pip3 install https://storage.googleapis.com/tensorflow/linux/cpu/tensorflow-0.7.1-cp34-none-linux_x86_64.whl
```
**Step 5:** Install gcc5 from test toolchain repo
Note: This changes the standard system gcc.
```bash
apt-get -y install software-properties-common python-software-properties
add-apt-repository ppa:ubuntu-toolchain-r/test
apt-get update && apt-get -y install gcc-5 g++-5
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 1 --slave /usr/bin/g++ g++ /usr/bin/g++-5
```
**Step 6:** Install latest cvedia-cli from github
```bash
mkdir -p /usr/src/cvedia && cd /usr/src/cvedia
git clone https://github.com/Cvedia/cvedia-cli
cd /usr/src/cvedia/cvedia-cli
sed -i s/python3.5m-config/python3.4m-config/g configure.ac
ACLOCAL_PATH="/usr/src/cvedia/cvedia-cli" autoreconf -if
autoheader
aclocal
autoconf
automake
./configure
make -j 4
make install
```

<a name=w10></a>
# Installation for Windows 10 #

Windows 10 now supports Ubuntu 16 command line through Developer mode. 

To install Bash shell on your Windows 10 PC, do the following:

**Step 1:** Enable Developer mode

>- Open Settings.
- Click on Update & security.
- Click on For Developers.
- Under "Use developer features", select the Developer mode option to setup the environment to install Bash.
- On the message box, click Yes to turn on developer mode.

After the necessary components install, you'll need to restart your computer.

**Step 2:** Add the Linux (beta) subsystem

>- Open Control Panel.
- Click on Programs.
- Click on Turn Windows features on or off.
- Check the Windows Subsystem for Linux (beta) option.
  Click OK.

**Step 3:** Prepare Linux (beta) for use

>- Open Start, do a search for bash.exe, and press Enter.
- On the command prompt, type y and press Enter to download and install Bash from the Windows Store.
- Then you'll need to create a default UNIX user account. This account doesn't have to be the same as your Windows account.

At this point, Windows 10 has a fully functional Linux subsystem installed on it. It has access to the Windows filesystem and you can follow the [Ubuntu 16](u16) installation guide to setup cvedia-cli.

This summary of the installation of the Windows 10 bash subsystem was derived from its full version on [windowscentral.com](http://www.windowscentral.com/how-install-bash-shell-command-line-windows-10).

# Limitations #

- We currently do not support 32-bit systems, support for this will be added in the future.
