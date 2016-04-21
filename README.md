# ecutools
## Open Source ECU Tuning and Diagnostics

The goal of ecutools is to provide an open source (POSIX friendly!) alternative to commercial J2534 pass-through devices and their accompanying PC software. Controller Area Network (CAN bus) is the only supported protocol right now.

This project includes information on how to create a pass-through device and includes software that can perform diagnostics and reprogramming of an ECU.

The project is broken down into 3 sub-projects. They are:

1. pass-through: J2534 compliant pass-through device and accompanying ecutools.io client software
2. server: WebSocket server responsible for hosting real-time tuning / diagnostic sessions on ecutools.io
3. webui: Real-time web based user interface for administration, configuration, and management of the ECU, pass-through device, and ecutools.io settings

Successful tests have been conducted on the following architectures:

1. [x86](http://en.wikipedia.org/wiki/X86)
2. [x86_64](http://en.wikipedia.org/wiki/X86-64)
3. [ARM](http://en.wikipedia.org/wiki/ARM_architecture)

### Build

        ./autogen.sh
        ./configure
        make
        make install

