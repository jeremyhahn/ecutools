# ECU Tools
## IoT Automotive Tuning, Diagnostics & Analytics

> What if you could turn off that annoying Check Engine Light?

> What if your car could automatically install firmware updates to fix manufacturer defects or allow your mechanic to remotely diagnose problems?

> What if you could get a text message when your spouse is 2000 miles overdue for an oil change, or your teenager is going over 100mph in the family station wagon?

> What if your alarm clock could start your car on those ice cold winter mornings?

> What if your performance tuner could  tune your ECU while you're sitting in the pits and he's sitting on his couch?

> What if you could replace the outdated, overpriced, closed source data logger on your race car with something that's much more powerful and costs less?

##### Race into the future of Connected Cars and the Internet of Things (IoT) with ECU Tools, the solution to archaic, overpriced, proprietary automotive technology!

## Capabilities

* POSIX alternative to commercial J2534 standard.
* ECU Reprogramming (J2534-1 Pass-Thru)
* OEM Diagnostics
* OBDII Diagnostics
* Data Logging
* Bus Monitoring
* Security Research

## Features

* Amazon Web Services Integration (API Gateway & IoT Platform) for endless automation possibilities.
* Real-time cloud based tuning, diagnostics & analytics.
* Unrestricted access to vehicle network. Own YOUR automobile and YOUR data.
* Log vehicle data to the cloud for real-time and offline data processing, trending and statistical analysis.
* Perform historical analysis using deep learning (AI / ML) technologies, integrate with other IoT things, build a custom web service to unlock your doors, offer remote mechanic services - the possibilities are endless.

## Build / Install

ECU Tools is developed with the following platforms in mind:

1. [x86](http://en.wikipedia.org/wiki/X86)
2. [x86_64](http://en.wikipedia.org/wiki/X86-64)
3. [ARM](http://en.wikipedia.org/wiki/ARM_architecture)

#### Dependencies

Included in the ecutools source and are statically linked during build: 

1. [mbedTLS](https://tls.mbed.org/)
2. [aws-iot-device-sdk-embedded-C](https://github.com/aws/aws-iot-device-sdk-embedded-C)

Dynamic libraries (not included):

1. [Jansson](http://www.digip.org/jansson/)
2. [libcurl](https://curl.haxx.se/libcurl/)

To install,

	./autogen.sh
	./configure
	make mbedtls
	make
	sudo make install

## Development

Change the hard coded device / account id in Makefile.am `install-thing` and `clean-thing` targets.

	./autogen.sh
	./configure
	make devenv

Got skills? Clone the repo and join the Slack channel to help create the future of open source connected cars.

