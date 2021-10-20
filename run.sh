#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./3rdparty/opencv/TX2/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./3rdparty
./nvidia_tx2
