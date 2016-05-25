require_relative 'service'

module Ecutools::Passthru
  class Cli < Stackit::BaseCli

    def initialize(*args)
      super(*args)
    end

    def self.initialize_cli
      Thor.desc "passthru", "Manages PassThru Things via J2534"
      Thor.subcommand "passthru", self
    end

    desc 'PassThruScanForDevices', 'Returns the number of accessible Pass-Thru Devices'
    def PassThruScanForDevices
      say_status 'OK', Service.new(options).PassThruScanForDevices
    end

    desc 'PassThruOpen', 'Establish communications with the designated Pass-Thru Device verifying its connected and initialize it'
    method_option :name, :aliases => '-n', :desc => 'PassThru device name', :required => true
    method_option :device_id, :type => :numeric, :aliases => '-d', :desc => 'PassThru device Id', :required => true
    def PassThruOpen
      say_status 'OK', Service.new(options).PassThruOpen
    end

#print_table passthru.PassThruScanForDevices.map.with_index{|a, i| [i+1, *a]}
  end

end
