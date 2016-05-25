require 'ecutools/j2534'

module Ecutools::Passthru
  class Service

    include Ecutools::J2534

    attr_accessor :options

    def initialize(options)
      self.options = options
    end

    def PassThruScanForDevices
      super
    end

    def PassThruGetNextDevice
      super(options[:name])
    end

    def PassThruOpen
      super(options[:name], options[:device_id])
    end

  end
end
