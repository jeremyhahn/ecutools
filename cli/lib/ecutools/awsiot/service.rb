module Ecutools::Awsiot
  class Service

    attr_accessor :thing_name
    attr_accessor :acct
    attr_accessor :policy
    attr_accessor :path
    attr_accessor :certificate_id
    attr_accessor :save_certs
    attr_accessor :owner
    attr_accessor :group

    def initialize(options)
      self.thing_name = options[:name]
      self.acct = options[:acct]
      self.policy = options[:policy]
      self.path = options[:path] || "#{Ecutools.home}/../../"
      self.certificate_id = options[:certificate_id]
      self.save_certs = options[:save_certs]
      self.owner = options[:owner] || 'root'
      self.group = options[:group] || 'ecutools'
    end

    def create_thing
      keys_and_cert = Ecutools.aws.iot.create_keys_and_certificate({
	    set_as_active: true
	  })
      thing = Ecutools.aws.iot.create_thing({
        thing_name: thing_name,
        attribute_payload: {
          attributes: {
            "type" => "j2534",
            "acct" => acct,
            "certificate_id" => keys_and_cert[:certificate_id]
          }
        }
      })
	  Ecutools.aws.iot.attach_thing_principal({
       thing_name: thing_name,
       principal: keys_and_cert[:certificate_arn]
      })
      policy = Ecutools.aws.iot.create_policy({
        policy_name: "#{thing_name}-policy",
        policy_document: policy_body
      })
      Ecutools.aws.iot.attach_principal_policy({
        policy_name: "#{thing_name}-policy",
        principal: keys_and_cert[:certificate_arn]
      })
      save_certificates(keys_and_cert) if save_certs
      thing[:thing_arn]
    end

    def delete_thing
      certificate_id = Ecutools.aws.iot.describe_thing(
      	thing_name: thing_name
      )[:attributes]["certificate_id"]
      Ecutools.aws.iot.list_thing_principals({
        thing_name: thing_name
      })[:principals].each do |principal|
        Ecutools.aws.iot.detach_thing_principal({
          thing_name: thing_name,
          principal: principal
        })
        begin
          Ecutools.aws.iot.detach_principal_policy({
            policy_name: "#{thing_name}-policy",
            principal: principal
          })
        rescue Aws::IoT::Errors::ResourceNotFoundException; end
      end
      Ecutools.aws.iot.update_certificate({
        certificate_id: certificate_id,
        new_status: "INACTIVE"
      })
      Ecutools.aws.iot.delete_certificate({
        certificate_id: certificate_id
      })
      Ecutools.aws.iot.delete_policy({
        policy_name: "#{thing_name}-policy"
      })
      Ecutools.aws.iot.delete_thing({
        thing_name: thing_name
      })
    end

    def list_things
      Ecutools.aws.iot.list_things({
        max_results: 100,
        attribute_name: "type",
        attribute_value: "j2534"
      })[:things].map { |thing| 
      	thing[:thing_name] 
      }
    end

    def config_h
      contents = File.read(config_h_file)
      newconfig = contents.sub(/#define\s+AWS_IOT_MQTT_CLIENT_ID.*$/, "#define AWS_IOT_MQTT_CLIENT_ID         \"#{thing_name}\"")
      newconfig = newconfig.sub(/#define\s+AWS_IOT_MY_THING_NAME.*$/, "#define AWS_IOT_MY_THING_NAME          \"#{thing_name}\"")
      newconfig = newconfig.sub(/#define\s+AWS_IOT_ROOT_CA_FILENAME.*$/, "#define AWS_IOT_ROOT_CA_FILENAME       \"ca.crt\"")
      newconfig = newconfig.sub(/#define\s+AWS_IOT_CERTIFICATE_FILENAME.*$/, "#define AWS_IOT_CERTIFICATE_FILENAME   \"#{thing_name}.crt.pem\"")
      newconfig = newconfig.sub(/#define\s+AWS_IOT_PRIVATE_KEY_FILENAME.*$/, "#define AWS_IOT_PRIVATE_KEY_FILENAME   \"#{thing_name}.key.pem\"")
      File.write(config_h_file, newconfig)
    end

    def empty_config_h
      contents = File.read(config_h_file)
      newconfig = contents.sub(/#define\s+AWS_IOT_MQTT_CLIENT_ID.*$/, "#define AWS_IOT_MQTT_CLIENT_ID         \"\"")
      newconfig = newconfig.sub(/#define\s+AWS_IOT_MY_THING_NAME.*$/, "#define AWS_IOT_MY_THING_NAME          \"\"")
      newconfig = newconfig.sub(/#define\s+AWS_IOT_ROOT_CA_FILENAME.*$/, "#define AWS_IOT_ROOT_CA_FILENAME       \"\"")
      newconfig = newconfig.sub(/#define\s+AWS_IOT_CERTIFICATE_FILENAME.*$/, "#define AWS_IOT_CERTIFICATE_FILENAME   \"\"")
      newconfig = newconfig.sub(/#define\s+AWS_IOT_PRIVATE_KEY_FILENAME.*$/, "#define AWS_IOT_PRIVATE_KEY_FILENAME   \"\"")
      File.write(config_h_file, newconfig)
    end

    def delete_certificates
      FileUtils.rm "#{certs_dir}/#{thing_name}.crt.pem"
      FileUtils.rm "#{certs_dir}/#{thing_name}.key.pem"
      empty_config_h
    end

  private

    def policy_body
      (policy && File.exist?(policy)) ? File.read(policy) : File.read("#{Ecutools.home}/ecutools/awsiot/default-policy.json")
    end

    def config_h_file
      "#{Ecutools.home}/../../src/aws_iot_config.h"
    end

    def certs_dir
      @certs_dir ||= "/etc/ecutools/certs"
    end

    def save_certificates(keys_and_cert)
      Dir.mkdir certs_dir unless File.exist?(certs_dir)
      File.write("#{certs_dir}/#{thing_name}.crt.pem", keys_and_cert[:certificate_pem])
      File.write("#{certs_dir}/#{thing_name}.key.pem", keys_and_cert[:key_pair][:private_key])
      FileUtils.cp("#{Ecutools.home}/ecutools/awsiot/ca.crt", "#{certs_dir}/ca.crt") unless File.exist?("#{certs_dir}/ca.crt") 
      begin
        FileUtils.chmod 0600, "#{certs_dir}/#{thing_name}.key.pem"
      rescue Errno::EPERM => e
        puts "Failed to chmod key! Permission error."
      end
    end

  end

end
