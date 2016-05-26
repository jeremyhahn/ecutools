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
require 'ecutools/j2534/libj2534'
require 'ecutools/j2534/structs'
require 'ecutools/j2534/models'

module Ecutools
  module J2534

   include Ecutools::J2534::Structs
   include Ecutools::J2534::Models
   include Ecutools::J2534::Error

   def PassThruScanForDevices
     pDeviceCount = FFI::MemoryPointer.new(:ulong, 1)
     response = Libj2534.PassThruScanForDevices(pDeviceCount)
     raise Ecutools::J2534Error, Error[response] if response != 0
     pDeviceCount.read_long
   end

   def PassThruGetNextDevice(device)
     raise Ecutools::J2534Error, 'Device must be an instance of Ecutools::Models::Device' unless device.instance_of?(Ecutools::J2534::Models::Device)
     sdevice = Ecutools::J2534::Structs::SDEVICE.new
     sdevice[:DeviceName].to_ptr.put_string(0, device.DeviceName) if device.DeviceName
     response = Libj2534.PassThruGetNextDevice(sdevice)
     raise Ecutools::J2534Error, Error[response] if response != 0
     map_sdevice_to_model(sdevice, device)
   end

   def PassThruOpen(name, deviceId)
     pName = FFI::MemoryPointer.from_string(name)
     pDeviceId = FFI::MemoryPointer.new(:ulong, 8).put_ulong(0, deviceId)
     response = Libj2534.PassThruOpen(pName, pDeviceId)
     raise Ecutools::J2534Error, Error[response] unless response == 0
     true
   end

   def PassThruClose(deviceId)
     response = Libj2534.PassThruClose(deviceId)
     raise Ecutools::J2534Error, Error[response] unless response == 0
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
