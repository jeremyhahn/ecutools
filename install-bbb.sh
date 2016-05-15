# install build dependencies
sudo apt-get install libjansson-dev libcurl4-openssl-dev can-utils

# compile mbedTLS
cd src/aws_iot_src/external_libs/mbedTLS
make
cd -

# build ecutools
git clone https://github.com/jeremyhahn/ecutools.git
cd ecutools
./autogen.sh && ./configure && make && sudo make install

echo "All done. Create certs directory and update aws_iot_config.h."
