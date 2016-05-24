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
    context 'PassThruScanForDevices' do

      it 'returns 0 devices when no things exist in AWS IoT' do
        expect(j2534.PassThruScanForDevices).to eq(0)
      end

      it 'returns 1 device when 1 things exists in AWS IoT' do
        things.test(1) {
          expect(j2534.PassThruScanForDevices).to eq(1)
        }
      end

      it 'returns 2 devices when 2 things exists in AWS IoT' do
      	things.test(2) {
          expect(j2534.PassThruScanForDevices).to eq(2)
        }
      end

    end

  end

end
