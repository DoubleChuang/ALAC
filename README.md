# ALAC

## Introduction
ALAC is a tool using the ALSA API to capture the Raspberry Pi's pcm audio and convert it to aac format using faac.

## Compile ALSA
* ### Method 1:
```shell=
sudo apt-get install libasound2-dev
```
* ### Method 2:
```shell=
wget ftp://ftp.alsa-project.org/pub/lib/alsa-lib-1.1.7.tar.bz2
tar xjf alsa-lib-1.1.7.tar.bz2
cd alsa-lib-1.1.7/
./configure --prefix=$(pwd)/build                                                                                                                          
make -j4 
make install
```

## Usage

see alacSample.c.

## To-Do
Add description.
