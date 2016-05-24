require 'ecutools'
require 'stackit/cli/stack_cli'

module Ecutools
  class Cli < Thor

    def initialize(*args)
      super(*args)
    end

    def self.require_clis
      Dir.glob("#{Ecutools.home}/ecutools/*") do |pkg|
        next if File.file?(pkg)
        pkg_name = pkg.split('/').last
        full_pkg_name = "Ecutools::#{pkg_name.capitalize}::Cli"
        cli = "#{pkg}/cli.rb"
        if File.exist?(cli)
          require cli
          clazz = full_pkg_name.constantize
          clazz.initialize_cli if clazz.respond_to?('initialize_cli')
        end
      end
    end

    desc 'version', 'Displays ecutools version'
    def version
      puts "ECU Tools CLI v#{Ecutools::VERSION}"
    end

  end
end
