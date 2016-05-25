#!/bin/bash

sudo apt-get install libjansson-dev libcurl4-openssl-dev can-utils

git clone https://github.com/jeremyhahn/ecutools.git
cd ecutools

./autogen.sh && ./configure && make mbedtls && make && sudo make install

