package uds;

import org.testng.Assert;
import org.testng.annotations.Test;

public class UDSTests {

	private UDS uds;
	
	public UDSTests() {
		uds = new UDS();
	}
	
	@Test
	public void setDiagnosticSessionControlWorks() {

		int flashMode = 3;  // Reprogram / flash
		int response = uds.setDiagnosticSessionControl(flashMode);
		Assert.assertEquals(response, flashMode);
	}

	@Test
	public void setDiagnosticTroubleCodeSettingWorks() {

		int off = 2;  // 1=on, 2=off
		int response = uds.setDiagnosticTroubleCodeSetting(off);
		Assert.assertEquals(response, off);
	}

	@Test
	public void setCommunicationControlWorks() {

		int communicationType = 3;  // 1=normal, 2=network, 3=normal and network
		int controlType = 0;        // 0=enableRxAndTx, 1=enableRxAndDisableTx, 2=disableRxAndEnableTx, 3=disableRxAndTx
		int response = uds.setCommunicationControl(communicationType, controlType);
		Assert.assertEquals(response, communicationType);
	}

	@Test
	public void requestSeedWorks() {

		int response = uds.requestSeed();
		Assert.assertEquals(response, 1);  // hard coded in uds.c
	}

	@Test
	public void requestDownloadWorks() {

		int dataFormatIdentifier = 0; // no compressionMethod or encryptingMethod is used
		int addressAndLengthFormatIdentifier = 1;  // ?
		int memoryAddress = 0x1234;
		int memorySize = 10;  // 10 bytes

		int response = uds.requestDownload(dataFormatIdentifier, addressAndLengthFormatIdentifier, memoryAddress, memorySize);
		Assert.assertEquals(response, 1);  // hard coded in uds.c
	}

	@Test
	public void requestTransferDataWorks() {

		int address = 0x01;
		String data = "Test";

		int response = uds.requestTransferData(address, data);
		Assert.assertEquals(response, 1);  // hard coded in uds.c
	}

	@Test
	public void requestTransferExitWorks() {
		int response = uds.requestTransferExit();
		Assert.assertEquals(response, 1);  // hard coded in uds.c
	}

	@Test
	public void setRoutineControlWorks() {

		int controlType = 1; // 1=startRoutine, 2=stopRoutine, 3=requestRoutineControl
		int response = uds.setRoutineControl(controlType);
		Assert.assertEquals(response, controlType);
	}

	@Test
	public void ecuResetWorks() {

		int resetType = 1; // 1=hardReset, 2=keyOffOnReset, 3=softReset, 4=enableRapidPowerShutDown, 5=disableRapidPowerShutdown
		int response = uds.ecuReset(resetType);
		Assert.assertEquals(response, resetType);
	}
}