require 'spec_helper'

describe Ecutools::Api do

  let(:ecutools) { Ecutools::Api.new }
  let(:deviceName) { "J2534-1:ecutools"}
  let(:deviceId) { 420 }

  it 'PassThruScanForDevices returns 1 device' do
    expect(ecutools.PassThruScanForDevices).to eq(1)
  end

  it 'PassThruOpen returns success when it finds VirtualDataLogger' do
    expect(ecutools.PassThruOpen("VirtualDataLogger", deviceId)).to eq(0)
  end

  it 'PassThruOpen throws EcutoolsError when it doesnt find the requested device' do
    expect { ecutools.PassThruOpen(deviceName, deviceId) }.to raise_error EcutoolsError
  end

end
