require 'stackit'
require 'ecutools/version'

module Ecutools
  class << self

    attr_accessor :home
    attr_accessor :logger
    attr_accessor :aws

    def aws
      @aws ||= Stackit.aws
    end

    def home
      Pathname.new(File.expand_path('ecutools.gemspec', __dir__)).dirname
    end

    def logger
      @logger ||= Logger.new(STDOUT)
    end

  end
end
