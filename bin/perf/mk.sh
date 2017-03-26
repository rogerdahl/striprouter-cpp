#!/bin/bash

set -x

sudo apt-get install libiberty-dev binutils-dev
mkdir install
cd install
apt-get source linux-tools-`uname -r`
sudo apt-get build-dep linux-tools-`uname -r`
cd linux-`uname -r | sed 's/-.*//'`/tools/perf
make
