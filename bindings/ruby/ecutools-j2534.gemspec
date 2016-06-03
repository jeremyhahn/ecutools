# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'ecutools/j2534/version'

Gem::Specification.new do |spec|
  spec.name          = "ecutools-j2534"
  spec.version       = Ecutools::J2534::VERSION
  spec.authors       = ["Jeremy Hahn"]
  spec.email         = ["mail@jeremyhahn.com"]

  spec.summary       = %q{DevOps automation toolkit generator for AWS.}
  spec.description   = %q{Generates an AWS DevOps toolkit.}
  spec.homepage      = "https://github.com/jeremyhahn/awskit"
  spec.license       = "GPLv3"

  # Prevent pushing this gem to RubyGems.org by setting 'allowed_push_host', or
  # delete this section to allow pushing this gem to any host.
  if spec.respond_to?(:metadata)
    spec.metadata['allowed_push_host'] = "https://rubygems.org"
  else
    raise "RubyGems 2.0 or newer is required to protect against public gem pushes."
  end

  spec.files         = `git ls-files -z`.split("\x0").reject { |f| f.match(%r{^(test|spec|features)/}) }
  spec.bindir        = "exe"
  spec.executables   = spec.files.grep(%r{^exe/}) { |f| File.basename(f) }
  spec.require_paths = ["lib"]

  spec.add_development_dependency "bundler", "~> 1.11"
  spec.add_development_dependency "rake", "~> 10.0"
  spec.add_development_dependency "rspec", "~> 3.0"
  spec.add_development_dependency "awsclient", "~> 0"

  spec.add_runtime_dependency "ffi", "~> 1.9.10"
end
