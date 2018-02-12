#!/bin/bash

echo "Installing cpprestsdk dependencies"

sudo apt-get install g++ git make libssl-dev cmake

echo "Downloading cpprestsdk source"

git clone https://github.com/Microsoft/cpprestsdk.git casablanca

echo "Building cpprestsdk"

cd casablanca/Release
mkdir build.debug
cd build.debug
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j5
sudo make install

echo "Done installing cpprestsdk"


