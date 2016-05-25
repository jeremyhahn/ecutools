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

$LOAD_PATH.unshift File.expand_path('../../lib', __FILE__)
$LOAD_PATH.unshift File.expand_path('../../../cli/lib', __FILE__)

require 'ecutools/j2534'
require 'ecutools'
require 'ecutools/awsiot/service'
require 'awsclient'
require 'pp'

RSpec.shared_context 'J2534' do

   let(:j2534) { Class.new { include Ecutools::J2534 }.new }
   let(:things) { Things.new }
end

class Things

  attr_accessor :things

  def initialize
    self.things = []
  end

  def create!(num = 1)
    num.times do |i|
      thing_name = "#{thing_id}-#{i}"
      certificate_id = Ecutools::Awsiot::Service.new(
        name: thing_name,
        acct: random_id
      ).create_thing
      things << { 
        :name => thing_name, 
        :certificate_id => certificate_id
      }
    end
  end

  def delete!
    things.each do |thing|
      Ecutools::Awsiot::Service.new(
        name: thing[:name],
        certificate_id: thing[:certificate_id]
      ).delete_thing
    end
  end

  def [](key)
    things[key]
  end

  def test(num_things = 1, &block)
    create!(num_things)
    begin
      block.call
    rescue => e
      Stackit.logger.error e.message
      puts e.backtrace unless e.instance_of?(Ecutools::J2534::J2534Error)
    end
    delete!
  end

private

  def random_id
    @random_id ||= (0...8).map { (65 + rand(26)).chr }.join
  end

  def thing_id
    "rspec-#{random_id}"
  end
end
