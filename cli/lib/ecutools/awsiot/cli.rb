require_relative 'service'

module Ecutools::Awsiot
  class Cli < Stackit::BaseCli

    def initialize(*args)
      super(*args)
    end

    def self.initialize_cli
      Thor.desc "awsiot", "Manages AWS IoT J2534 PassThru Things"
      Thor.subcommand "awsiot", self
    end

    desc 'create-thing', 'Creates a new AWS PassThru Thing, policy, and certificate'
    method_option :name, :aliases => '-n', :desc => 'The name of the new passthru thing', :required => true
    method_option :acct, :aliases => '-a', :desc => 'Account identifier', :required => true
    method_option :save_certs, :aliases => '-s', :desc => 'True to save generated certificates to disk', :default => true
    method_option :config_h, :desc => 'True to configure ecutools aws_iot_config.h header'
    method_option :owner, :aliases => '-o', :desc => 'Optional certificate owner'
    method_option :group, :aliases => '-g', :desc => 'Optional certificate group'
    def create_thing
      awsiot = Service.new(options)
      thing_arn = awsiot.create_thing

      config_h = ask("Would you like to update ecutools src/aws_iot_config.h?")
      if config_h == "y"
        awsiot.config_h
      end

      say_status 'OK', thing_arn
    end

    desc 'delete-thing', 'Deletes an AWS PassThru Thing, policy, and certificate'
    method_option :name, :aliases => '-n', :desc => 'The name of the new passthru thing', :required => true
    def delete_thing
      awsiot = Service.new(options)
      awsiot.delete_thing

      delcerts = ask("Would you like to delete certificates from ecutools/certs?")
      if delcerts == "y"
        awsiot.delete_certificates
      end

      say_status 'OK', "Successfully deleted #{options[:name]}"
    end

    desc 'delete-things', 'Deletes all AWS PassThru Things, policies, and certificates'
    def delete_things
      awsiot = Service.new(options)
      i = 0
      awsiot.list_things.each do |thing|
        next if thing == "myj2534"
        awsiot.thing_name = thing
        awsiot.delete_thing
        i = i + 1
      end
      say_status 'OK', "Successfully deleted #{i} PassThru Things"
    end

    desc 'list-things', 'Displays a list of J2534 PassThru Things'
    method_option :acct, :aliases => '-a', :desc => '', :required => true
    def list_things
      print_table Service.new(options).list_things.map.with_index{|a, i| [i+1, *a]}
    end

  end

end
