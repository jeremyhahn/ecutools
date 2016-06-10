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

require 'ffi'
require "ecutools/j2534/version"
require 'ecutools/j2534/constants'
require 'ecutools/j2534/libj2534'
require 'ecutools/j2534/models'

module Ecutools
  module J2534

   include Ecutools::J2534::Constants
   include Ecutools::J2534::Structs
   include Ecutools::J2534::Models
   include Ecutools::J2534::Error

   def PassThruScanForDevices
     pDeviceCount = FFI::MemoryPointer.new(:ulong, 1)
     response = Libj2534.PassThruScanForDevices(pDeviceCount)
     raise Ecutools::J2534Error, Error[response] unless response === STATUS_NOERROR
     pDeviceCount.read_long
   end

   def PassThruGetNextDevice(device)
     raise Ecutools::J2534Error, 'Device must be an instance of Ecutools::Models::Device' unless device.instance_of?(Ecutools::J2534::Models::Device)
     sdevice = Ecutools::J2534::Structs::SDEVICE.new
     sdevice[:DeviceName].to_ptr.put_string(0, device.DeviceName) if device.DeviceName
     response = Libj2534.PassThruGetNextDevice(sdevice)
     raise Ecutools::J2534Error, Error[response] unless response === STATUS_NOERROR
     map_sdevice_to_model(sdevice, device)
   end

   def PassThruOpen(name, deviceId)
     pName = FFI::MemoryPointer.from_string(name)
     pDeviceId = FFI::MemoryPointer.new(:ulong, 8).put_ulong(0, deviceId)
     response = Libj2534.PassThruOpen(pName, pDeviceId)
     raise Ecutools::J2534Error, Error[response] unless response === STATUS_NOERROR
     true
   end

   def PassThruClose(deviceId)
     response = Libj2534.PassThruClose(deviceId)
     raise Ecutools::J2534Error, Error[response] unless response === STATUS_NOERROR
     true
   end

   def PassThruConnect(deviceId, protocolId, flags, baudRate, resource, channelId)
    raise Ecutools::J2534Error, 'resource must be an instance of Ecutools::Models::Resource' unless resource.instance_of?(Ecutools::J2534::Models::Resource)
     resourceStruct = Ecutools::J2534::Structs::RESOURCE_STRUCT.new
     resourceStruct[:Connector] = resource.Connector || 0
     resourceStruct[:NumOfResources] = resource.NumOfResources || 0
     resourceStruct[:ResourceListPtr] = resource.ResourceListPtr || 0
     pChannelID = FFI::MemoryPointer.new(:ulong, 8).put_ulong(0, channelId)
     response = Libj2534.PassThruConnect(deviceId, protocolId, flags, baudRate, resourceStruct, pChannelID)
     raise Ecutools::J2534Error, Error[response] unless response === STATUS_NOERROR
     true
   end

   def PassThruDisconnect(channelId)
     response = Libj2534.PassThruDisconnect(channelId)
     raise Ecutools::J2534Error, Error[response] unless response === STATUS_NOERROR
     true
   end

   def PassThruLogicalConnect(physicalChannelId, protocolId, flags, channelDescriptor, channelId)
   end

   def PassThruLogicalDisconnect(channelId)
   end 

   def PassThruSelect(channelSet, selectType, timeout)
     channelset = Ecutools::J2534::Structs::SCHANNELSET.new
     channelset[:ChannelCount] = channelSet.ChannelCount
     channelset[:ChannelThreshold] = channelSet.ChannelThreshold
     channelset[:ChannelList] = FFI::MemoryPointer.new(
       :ulong, 8 * channelSet.ChannelList.length
     ).put_array_of_ulong(0, channelSet.ChannelList)
     response = Libj2534.PassThruSelect(channelset, selectType, timeout)
     raise Ecutools::J2534Error, Error[response] unless response === STATUS_NOERROR
     true
   end

  private

    def map_sdevice_to_model(sdevice, device)
      device.DeviceName = sdevice[:DeviceName].to_ptr.read_string
      device.DeviceAvailable = sdevice[:DeviceAvailable]
      device.DeviceDLLFWStatus = sdevice[:DeviceDLLFWStatus]
      device.DeviceConnectMedia = sdevice[:DeviceConnectMedia]
      device.DirectConnectSpeed = sdevice[:DirectConnectSpeed]
      device.DirectSignalQuality = sdevice[:DirectSignalQuality]
      device.DirectSignalStrength = sdevice[:DirectSignalStrength]
      device
    end

  end
end
