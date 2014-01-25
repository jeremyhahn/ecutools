###### Open Source ECU Tuning and Diagnostics

The goal of ecutools is to provide an open source (POSIX friendly!) alternative to commercial J2534 pass-through devices and their accompanying PC software. At
the present time, only Controller Area Network (CAN bus) protocol is supported.

This project includes information needed to create a pass-through device and includes the supporting software needed to perform
diagnostics and programming of an ECU.

ecutools is currently in an early development state and is not usable. Join the project and start contributing!

Successful tests have been conducted on the following architectures:

1. [x86](http://en.wikipedia.org/wiki/X86)
2. [x86_64](http://en.wikipedia.org/wiki/X86-64)
3. [ARM](http://en.wikipedia.org/wiki/ARM_architecture)

### Build

	./autogen.sh
	./configure
	make
	make install

