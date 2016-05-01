require 'ffi'
require 'pp'

class EcutoolsError < StandardError; end

module Ecutools; end

module Ecutools::J2534
  extend FFI::Library
  ffi_lib 'c'
  #ffi_lib '/usr/local/lib/libj2534.so'
  ffi_lib '../../.libs/libj2534.so'
  attach_function :PassThruScanForDevices, [ :pointer ], :long
  attach_function :PassThruOpen, [ :pointer, :pointer ], :long
end

module Ecutools

  class Api

    def PassThruScanForDevices
      pDeviceCount = FFI::MemoryPointer.new(:ulong, 1)
      raise 'PassThruScanForDevices error' if Ecutools::J2534.PassThruScanForDevices(pDeviceCount) != 0
      pDeviceCount.read_ulong
    end

    def PassThruOpen(name, deviceId)
      pName = FFI::MemoryPointer.from_string(name)
      pDeviceId = FFI::MemoryPointer.new(:ulong, 1)
      rc = Ecutools::J2534.PassThruOpen(pName, pDeviceId)
      raise EcutoolsError, "PassThruOpen: error=#{rc}" unless rc == 0
      rc
    end

  end

end
