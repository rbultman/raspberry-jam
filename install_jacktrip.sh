#!/bin/bash
sudo apt-get --assume-yes install qt5-default
sudo apt-get --assume-yes install libjack-dev
wget --no-check-certificate cm-gitlab.stanford.edu/cc/jacktrip/repository/archive.zip
unzip archive.zip
cd jacktrip-master-c78f35339c0ed558220eba6f836f9c8199e7c3a8/src/
./build
sudo make install

