#!/bin/bash
NEWPATH=.:$PATH:/usr/lib/jvm/jdk1.7.0_65/bin
export PATH=$NEWPATH
echo "export PATH='$NEWPATH'" >> .bashrc
sudo apt-get update -y
sudo apt-get install -y dos2unix unzip tar vim

# Build jCUTE
cd /vagrant/
dos2unix build-lp-solve.sh
sudo ./build-lp-solve.sh
dos2unix package
sudo ./package
dos2unix setup
sudo ./setup
