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

module Ecutools::J2534::Structs

  class SDEVICE < FFI::Struct
    layout :DeviceName, [:char, 80],
           :DeviceAvailable, :ulong,
           :DeviceDLLFWStatus, :ulong,
           :DeviceConnectMedia, :ulong,
           :DirectConnectSpeed, :ulong,
           :DirectSignalQuality, :ulong,
           :DirectSignalStrength, :ulong
  end

  class RESOURCE_STRUCT < FFI::Struct
    layout :Connector, :ulong,
           :NumOfResources, :ulong,
           :ResourceListPtr, :pointer
  end

  class SCHANNELSET < FFI::Struct
    layout :ChannelCount, :ulong,
           :ChannelThreshold, :ulong,
           :ChannelList, :pointer
  end

  class PASSTHRU_MSG < FFI::Struct
    layout :ProtocolID, :ulong,
           :MsgHandle, :ulong,
           :RxStatus, :ulong,
           :TxFlags, :ulong,
           :Timestamp, :ulong,
           :DataLength, :ulong,
           :ExtraDataIndex, :ulong,
           :DataBuffer, :pointer,
           :DataBufferSize, :ulong
  end

end
