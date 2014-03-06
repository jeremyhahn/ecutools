package uds;

public class UDS {

	private native int uds_set_diagnostic_session_control(int session);
	private native int uds_set_diagnostic_trouble_code_setting(int setting);
	private native int uds_set_communication_control(int communicationType, int controlType);
	private native int uds_request_seed();
	private native int uds_request_download(int dataFormatIdentifier, int addressAndLengthFormatIdentifier, int memoryAddress, int memorySize);
	private native int uds_transfer_data(int address, String data);
	private native int uds_request_transfer_exit();
	private native int uds_set_routine_control(int controlType);
	private native int uds_ecu_reset(int resetType);

	static {
		System.loadLibrary("ecutools");
	}

	public int setDiagnosticSessionControl(int session) {
		return uds_set_diagnostic_session_control(session);
	}

	public int setDiagnosticTroubleCodeSetting(int setting) {
		return uds_set_diagnostic_trouble_code_setting(setting);
	}

	public int setCommunicationControl(int communicationType, int controlType) {
		return uds_set_communication_control(communicationType, controlType);
	}

	public int requestSeed() {
		return uds_request_seed();
	}

	public int requestDownload(int dataFormatIdentifier, int addressAndLengthFormatIdentifier, int memoryAddress, int memorySize) {
		return uds_request_download(dataFormatIdentifier, addressAndLengthFormatIdentifier, memoryAddress, memorySize);
	}

	public int requestTransferData(int address, String data) {
		return uds_transfer_data(address, data);
	}

	public int requestTransferExit() {
		return uds_request_transfer_exit();
	}

	public int setRoutineControl(int controlType) {
		return uds_set_routine_control(controlType);
	}

	public int ecuReset(int resetType) {
		return uds_ecu_reset(resetType);
	}
}