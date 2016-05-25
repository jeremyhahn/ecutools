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
    context 'PassThruOpen' do

      it 'PassThruOpen returns true when a connection is opened' do
      	things.test(1) {
          expect(j2534.PassThruOpen(things[0][:name], 1)).to eq(true)
        }
      end

      it 'PassThruOpen raises an exception when failing to open a connection' do
        expect(j2534.PassThruOpen("foo", 1)).to raise_error(J2534Error, /ERR_DEVICE_NOT_CONNECTED/)
      end

    end
  end
end
