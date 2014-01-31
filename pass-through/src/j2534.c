#include "j2534.h"

/**
 * The PassThruConnect function is used to establish a logical communication channel between the User
 * Application and the vehicle network(via the PassThru device) using the specified network layer protocol
 * and selected protocol options.
 *
 * ProtocolID
 * 		The protocol identifier selects the network layer protocol that will be used for the communications
 * 		channel. The network layer protocol identifier must be one of the values listed below.
 *
 * Flags
 * 		Protocol specific options that are defined by bit fields. This parameter is usually set to zero.
 * 		The flag value must adhere to the bit field values listed below.
 *
 * pChannelID
 * 		Pointer to an unsigned long(4 bytes) that receives the handle to the open communications channel. The
 * 		returned handle is used as an argument to other PassThru functions which require a communications
 * 		channel reference.
 *
 * 	If the function succeeds the return value is STATUS_NOERROR. If the function fails, the return value is a
 * 	nonzero error code and is defined in below.
 *
 * 	ERR_DEVICE_NOT_CONNECTED
 * 	ERR_INVALID_PROTOCOL_ID
 * 	ERR_NULL_PARAMETER
 * 	ERR_INVALID_FLAGS
 * 	ERR_FAILED
 * 	ERR_CHANNEL_IN_USE
 */
long PassThruConnect(unsigned long ProtocolID, unsigned long Flags, unsigned long *pChannelID) {
	return 1;
}

/**
 * The PassThruDisconnect function is used to terminate an existing logical communication channel
 * between the User Application and the vehicle network(via the PassThru device). Once disconnected the
 * channel identifier or handle is invalid. For the associated network protocol this function will terminate the
 * transmitting of periodic messages and the filtering of receive messages. The PassThru device periodic and
 * filter message tables will be cleared.
 *
 * ChannelID
 * 		The logical communication channel identifier assigned by the J2534 API/DLL when the communication
 * 		channel was opened via the PassThruConnect function.
 *
 * If the function succeeds the return value is STATUS_NOERROR. If the function fails, the return value is a
 * nonzero error code and is defined below.
 *
 * ERR_DEVICE_NOT_CONNECTED
 * ERR_FAILED
 * ERR_INVALID_CHANNEL_ID
 */
long PassThruDisconnect(unsigned long ChannelID) {
	return 1;
}

/**
 * The PassThruReadMsgs function is used to receive network protocol messages, receive indications and
 * transmit indications from an existing logical communication channel. The network protocol messages will
 * flow from the PassThru device to the User Application.
 *
 * ChannelID
 * 		The logical communication channel identifier assigned by the J2534 API/DLL when the communication
 * 		channel was opened via the PassThruConnect function.
 *
 * pMsg
 * 		Pointer to the message structure where the J2534 API/DLL will write the receive message(s). For reading
 * 		more than one message, this must be a pointer to an array of PASSTHRU_MSG structures.
 *
 * pNumMsgs
 * 		Pointer to the variable that contains the number of PASSTHRU_MSG structures that are allocated for
 * 		receive frames. The API regards this value as the maximum number of receive frames that can be
 * 		returned to the UserApplication. On function completion this variable will contain the actual number of
 * 		receive frames contained in the PASSTHRU_MSG structure. The number of receive messages returned
 * 		may be less than the number requested by the UserApplication.
 *
 * Timeout
 * 		Timeout interval(in milliseconds) to wait for read completion. A value of zero instructs the API/DLL to
 * 		read buffered receive messages and return immediately. A nonzero timeout value instructs the API/DLL
 * 		to return after the timeout interval has expired. The API/DLL will not wait the entire timeout
 * 		interval if an error occurs or the specified number of messages have been read.
 *
 * If the function succeeds the return value is STATUS_NOERROR. If the function fails, the return value is a
 * nonzero error code and is defined below.
 *
 * ERR_DEVICE_NOT_CONNECTED
 * ERR_FAILED
 * ERR_INVALID_CHANNEL_ID
 * ERR_NULL_PARAMETER
 * ERR_TIMEOUT
 * ERR_BUFFER_EMPTY
 * ERR_BUFFER_OVERFLOW
 */
long PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout) {
	return 1;
}

/**
 * The PassThruWriteMsgs function is used to transmit network protocol messages over an existing logical
 * communication channel. The network protocol messages will flow from the User Application to the
 * PassThru device.
 *
 * ChannelID
 * 		The logical communication channel identifier assigned by the J2534 API/DLL when the communication
 * 		channel was opened via the PassThruConnect function.
 *
 * pMsg
 * 		Pointer to the message structure containing the UserApplication transmit message(s).
 * 		For sending more than one message, this must be a pointer to an array of PASSTHRU_MSG structures.
 *
 * PNumMsgs
 * 		Pointer to the variable that contains the number of PASSTHRU_MSG structures that are allocated for
 * 		transmit frames. On function completion this variable will contain the actual number of messages sent to
 * 		the vehicle network. The transmitted number of messages may be less than the number requested by the
 * 		UserApplication.
 *
 * Timeout
 * 		Timeout interval(in milliseconds) to wait for transmit completion. A value of zero instructs the API/DLL
 * 		to queue as many transmit messages as possible and return immediately. A nonzero timeout value
 * 		instructs the API/DLL to wait for the timeout interval to expire before returning. The API/DLL will not
 * 		wait the entire timeout interval if an error occurs or the specified number of messages have been sent.
 *
 * If the function succeeds the return value is STATUS_NOERROR. If the function fails, the return value is a
 * nonzero error code and is defined below.
 *
 * ERR_DEVICE_NOT_CONNECTED
 * ERR_FAILED
 * ERR_INVALID_CHANNEL_ID
 * ERR_NULL_PARAMETER
 * ERR_TIMEOUT
 * ERR_MSG_PROTOCOL_ID
 * ERR_BUFFER_FULL
 * ERR_INVALID_MSG
 */
long PassThruWriteMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout) {
	return 1;
}

/**
 * The PassThruStartPeriodicMsg function is used to repetitively transmit network protocol messages at the
 * specified time interval over an existing logical communication channel. The periodic messages will flow
 * from the User Application to the PassThru device. There is a limit of ten periodic messages per network
 * layer protocol.
 *
 * ChannelID
 * 		The logical communication channel identifier assigned by the J2534 API/DLL when the communication
 * 		channel was opened via the PassThruConnect function.
 *
 * pMsg
 * 		Pointer to the message structure containing the User Application’s periodic message.
 *
 * pMsgID
 * 		Pointer to the variable that receives the handle to the periodic message. The returned handle is used as an
 * 		argument to PassThruStopPeriodicMsg to identify a specific periodic message.
 *
 * TimeInterval
 * 		Time interval(in milliseconds) at which the periodic message is repetitively transmitted. The acceptable
 * 		range is 5 t0 65,535 milliseconds.
 *
 * If the function succeeds the return value is STATUS_NOERROR. If the function fails, the return value is a
 * nonzero error code and is defined below.
 *
 * ERR_DEVICE_NOT_CONNECTED
 * ERR_FAILED
 * ERR_INVALID_CHANNEL_ID
 * ERR_NULL_PARAMETER
 * ERR_ INVALID_TIME_INTERVAL
 * ERR_MSG_PROTOCOL_ID
 * ERR_ EXCEEDED_LIMIT
 * ERR_INVALID_MSG
 */
long PassThruStartPeriodicMsg(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval) {
	return 1;
}

/**
 * The PassThruStopPeriodicMsg function is used to terminate the specified periodic message. Once
 * terminated the message identifier or handle value is invalid.
 *
 * ChannelID
 * 		The logical communication channel identifier assigned by the J2534 API/DLL when the communication
 * 		channel was opened via the PassThruConnect function.
 *
 * MsgID
 * 		The message identifier or handle assigned by the J2534 API/DLL when PassThruStartPeriodicMsg
 * 		function was called.
 *
 * If the function succeeds the return value is STATUS_NOERROR. If the function fails, the return value is a
 * nonzero error code and is defined below.
 *
 * ERR_DEVICE_NOT_CONNECTED
 * ERR_FAILED
 * ERR_INVALID_CHANNEL_ID
 * ERR_INVALID_MSG_ID
 */
long PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID) {
	return 1;
}

/**
 * The PassThruStartMsgFilter function is used to setup a network protocol filter that will selectively
 * restrict or limit network protocol messages received by the PassThru device. The filter messages will flow
 * from the User Application to the PassThru device. There is a limit of ten filter messages per network layer
 * protocol.
 *
 *
 */
long PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, PASSTHRU_MSG *pMaskMsg, PASSTHRU_MSG *pPatternMsg,
				PASSTHRU_MSG *pFlowControlMsg,	unsigned long *pMsgID) {
	return 1;
}

/**
 * The PassThruStopMsgFilter function is used to terminate the specified network protocol filter. Once
 * terminated the filter identifier or handle value is invalid.
 */
long PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID) {
	return 1;
}

/**
 * The PassThruSetProgrammingVoltage function is used to set the programmable voltage level on the
 * specified J1962 connector pin. Only one pin can have a specified voltage applied at a time. The
 * UserApplication must turn the voltage off(VOLTAGE_OFF option) on the selected pin before applying
 * voltage to a different pin. It is permissible to program pin 15 for SHORT_TO_GROUND and program
 * another pin to a voltage level.
 *
 * The User Application must protect against applying incorrect voltage levels to any of the programmable
 * pins.
 */
long PassThruSetProgrammingVoltage(unsigned long PinNumber, unsigned long Voltage) {
	return 1;
}

/**
 * The PassThruReadVersion function is used to retrieve the PassThru device firmware version, the
 * PassThru device DLL version and the version of the J2534 specification that was referenced. The version
 * information is in the form of NULL terminated strings. The User Application must dimension the version
 * character arrays to be at least 80 characters in size.
 *
 * pFirmwareVersion
 * 		Pointer to Firmware Version array, which will receive the firmware version string in XX.YY format.
 * 		The PassThru device supplies this string.
 *
 * pDllVersion
 * 		Pointer to DLL Version array, which will receive the DLL version string in XX.YY format. The
 * 		PassThru device installed DLL supplies this string.
 *
 * pApiVersion
 * 		Pointer to API Version array, which will receive the API version string in XX.YY format. The PassThru
 * 		device installed DLL supplies this string. It corresponds to the ballot date of the J2534 specification that
 * 		was referenced by the PassThru DLL implementers.
 *
 * If the function succeeds the return value is STATUS_NOERROR. If the function fails, the return value is a
 * nonzero error code and is defined below.
 *
 * ERR_DEVICE_NOT_CONNECTED
 * ERR_FAILED
 * ERR_NULL_PARAMETER
 */
long PassThruReadVersion(char *pFirmwareVersion, char * pDllVersion, char *pApiVersion) {
	return 1;
}


/**
 * The PassThruGetLastError function is used to retrieve the text description for the last PassThru function
 * that generated an error. This function should be called immediately after an error code is returned as any
 * intervening successful function call will wipe out the error code set by the most recently failed PassThru
 * function call. The error information is in the form of a NULL terminated string. The User Application must
 * dimension the error character array to be at least 80 characters in size.
 *
 * pErrorDescription
 * 		Pointer to Error Description array, which will receive the error description string.
 *
 * If the function succeeds the return value is STATUS_NOERROR. If the function fails, the return value is a
 * nonzero error code and is defined below.
 *
 * ERR_NULL_PARAMETER
 */
long PassThruGetLastError(char *pErrorDescription) {
	return 1;
}

/**
 * The PassThruIoctl function is a general purpose I/O control function for modifying the vehicle network
 * interface characteristics, measuring battery and voltage levels and clearing internal message tables and
 * Receive/Transmit queues of the PassThru device. The input and output structures are dependent on the
 * selected Ioctl function. The Ioctl supported function list is described below.
 *
 * ChannelID
 * 		The logical communication channel identifier assigned by the J2534 API/DLL when the communication
 * 		channel was opened via the PassThruConnect function.
 *
 * IoctlID
 * 		Ioctl function identifier, refer to Ioctl detailed function description below.
 *
 * pInput
 * 		Input structure pointer, refer to Ioctl detailed function description below.
 *
 * pOutput
 * 		Output structure pointer, refer to Ioctl detailed function description below.
 *
 * If the function succeeds the return value is STATUS_NOERROR. If the function fails, the return value is a
 * nonzero error code and is defined below.
 *
 * ERR_DEVICE_NOT_CONNECTED
 * ERR_FAILED
 * ERR_NULL_PARAMETER
 * ERR_INVALID_CHANNEL_ID
 * ERR_INVALID_IOCTL_ID
 * ERR_NOT_SUPPORTED
 */
long PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, void *pInput, void *pOutput) {
	return 1;
}
