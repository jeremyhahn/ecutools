##
# ecutools: IoT Automotive Tuning, Diagnostics & Analytics
# Copyright (C) 2014 Jeremy Hahn
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

module Ecutools::J2534::Models

  class Device
    attr_accessor :DeviceName
    attr_accessor :DeviceAvailable
    attr_accessor :DeviceDLLFWStatus
    attr_accessor :DeviceConnectMedia
    attr_accessor :DirectConnectSpeed
    attr_accessor :DirectSignalQuality
    attr_accessor :DirectSignalStrength
  end

  class Resource
    attr_accessor :Connector
    attr_accessor :NumOfResources
    attr_accessor :ResourceListPtr
  end

  class ChannelSet
    attr_accessor :ChannelCount
    attr_accessor :ChannelThreshold
    attr_accessor :ChannelList
  end

  class Message
    attr_accessor :ProtocolID
    attr_accessor :MsgHandle
    attr_accessor :RxStatus
    attr_accessor :TxFlags
    attr_accessor :Timestamp
    attr_accessor :DataLength
    attr_accessor :ExtraDataIndex
    attr_accessor :DataBuffer
    attr_accessor :DataBufferSize
  end

end
