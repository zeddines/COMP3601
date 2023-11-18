#!/bin/bash
. /opt/petalinux/2021.1/environment-setup-cortexa72-cortexa53-xilinx-linux
export PROJECT_ROOT=$(pwd)
if [ ! -f lame-3.100 ]; then
	if [ ! -f lame-3.100.tar.gz ]; then
		wget 'https://sourceforge.net/projects/lame/files/lame/3.100/lame-3.100.tar.gz'
	fi
	tar -xf lame-3.100.tar.gz
fi
cd lame-3.100
./configure --host=aarch64-xilinx-linux --prefix=$SDKTARGETSYSROOT --enable-static --disable-shared
make
make install
