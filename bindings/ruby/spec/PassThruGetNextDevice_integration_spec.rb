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

require 'spec_helper'

describe Ecutools::J2534 do
  include_context 'J2534' do
    context 'PassThruGetNextDevice' do

      it 'points to first device when DeviceName is NULL' do
        things.test(1) {

          deviceCount = j2534.PassThruScanForDevices
          expect(deviceCount).to be 1

          device = Ecutools::J2534::Models::Device.new
          response = j2534.PassThruGetNextDevice(device)

          expect(response.DeviceName).to eq(things[0][:name])
        }
      end

      it 'responds with ERR_EXCEEDED_LIMIT when all devices have been returned' do
        things.test(1) {

          deviceCount = j2534.PassThruScanForDevices
          expect(deviceCount).to be 1

          device = Ecutools::J2534::Models::Device.new
          response = j2534.PassThruGetNextDevice(device)
          expect(response.DeviceName).to eq(things[0][:name])

          expect(j2534.PassThruGetNextDevice(response)).to raise_error(Ecutools::J2534Error, /ERR_EXCEEDED_LIMIT/)
        }
      end

      it 'responds with ERR_BUFFER_EMPTY when no devices present' do
        things.test(0) {

          deviceCount = j2534.PassThruScanForDevices
          expect(deviceCount).to be 0

          device = Ecutools::J2534::Models::Device.new
          expect(j2534.PassThruGetNextDevice(device)).to raise_error(Ecutools::J2534Error, /ERR_BUFFER_EMPTY/)
        }
      end

      it 'responds with ERR_BUFFER_EMPTY when devices are present but PassThruScanForDevices hasnt been called' do
        things.test(1) {
          device = Ecutools::J2534::Models::Device.new
          expect(j2534.PassThruGetNextDevice(device)).to raise_error(Ecutools::J2534Error, /ERR_BUFFER_EMPTY/)
        }
      end

    end
  end
end
