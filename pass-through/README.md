###### Open Source ECU Tuning and Diagnostics

A J2534 compliant passthru device is responsible for interfacing with the ECU via OBD-II to carry out tuning and diagnostic operations requested by an operator. The J2534 standard is intended to abstract away the differences between manufacturer communication protocols and provide a consistent interface that enables compatibility between various tool and software manufacturers.

This section of the project provides resources and code needed to build a pass-through device using popular open source Microcontroller development boards and CAN Integrated Circuits (IC), as well as client software that connects the pass-through device with the ecutools.io server.

Supported / Tested Architectures:

1. [x86](http://en.wikipedia.org/wiki/X86)
2. [x86_64](http://en.wikipedia.org/wiki/X86-64)
3. [ARM](http://en.wikipedia.org/wiki/ARM_architecture)

### Build

	./autogen.sh
	./configure
	make
	make install

### Resources

#### Microcontrollers

Any Microcontroller capable of running Linux with enough head room left over to implement the CAN SAE/ISO standards and communicate with the CAN interface hardware will work.

1. [Raspberry Pi](http://en.wikipedia.org/wiki/Raspberry_Pi)
2. [BeagleBone](http://beagleboard.org)

#### CAN Interface

There are several open source CAN schematics floating around. Google Images is a great resource. Many are derived from an MCP2515 CAN controller / MCP2551 high-speed CAN transciever design. As an added convenience, the Linux source tree includes a driver for the MCP2515. Here are a few links to get you started:

##### DIY

1. [http://lnxpps.de/rpie](http://lnxpps.de/rpie)
2. [http://lnxpps.de/can2udpe/openwrt](http://lnxpps.de/can2udpe/openwrt/)
3. [http://hackaday.com/2011/03/08/can-sniffing-for-steering-wheel-button-presses/](http://hackaday.com/2011/03/08/can-sniffing-for-steering-wheel-button-presses)
4. [http://en.pudn.com/downloads167/sourcecode/embed/detail766529_en.html](http://en.pudn.com/downloads167/sourcecode/embed/detail766529_en.html)

##### Commercial

1. [PICAN](http://skpang.co.uk/catalog/pican-canbus-board-for-raspberry-pi-p-1196.html)
2. [CANBus Cape](http://elinux.org/Beagleboard:BeagleBone_CANBus)

#### Protocols

1. [SAE J-2534](http://standards.sae.org/j2534/1_200412)
2. [ISO 9141 - OBD](http://en.wikipedia.org/wiki/On-board_diagnostics)
3. [ISO 14230 - Keyword Protocol 2000](http://en.wikipedia.org/wiki/Keyword_Protocol_2000)
4. [ISO 11898 - CAN](http://en.wikipedia.org/wiki/ISO_11898)
5. [ISO 15765](http://en.wikipedia.org/wiki/ISO_15765-2)
6. [J1850PWM - Ford]
7. [J1850VPW - Chrysler/GM]
