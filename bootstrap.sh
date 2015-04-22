#!/bin/bash
sudo apt-get update -y
sudo apt-get install -y dos2unix unzip tar vim
echo "export 'PATH=$PATH:/usr/lib/jvm/jdk1.7.0_65/bin'" >> .bashrc

# Build jCUTE
cd /vagrant/
dos2unix build-lp-solve.sh
sudo ./build-lp-solve.sh
dos2unix package
sudo ./package
dos2unix setup
sudo ./setup
