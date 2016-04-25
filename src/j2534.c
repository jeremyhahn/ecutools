/**
 * ecutools: Automotive ECU tuning, diagnostics & analytics
 * Copyright (C) 2014  Jeremy Hahn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "j2534.h"

static bool j2534_is_connected = false;
static SDEVICE device_list[10] = {0};

/**
 * 7.3.1 PassThruScanForDevices
 *
 * This function shall generate a static list of Pass-Thru Devices that are accessible. If the function
 * call is successful, the return value shall be STATUS_NOERROR and the value pointed to by <pDeviceCount>
 * shall contain the total number of devices found. Otherwise, the return value shall reflect the error
 * detected and the value pointed to by <pDeviceCount> shall not be altered. The application does NOT need
 * to call PassThruOpen prior to calling this function. PassThruScanForDevices can be called at any time.
 * As a guideline, this function should return within 5 seconds.
 *
 * Additionally, the list created by this function call shall contain devices supported by the manufacturer
 * of the Pass-Thru Interface DLL.  * Applications should not expect this list to include device from other
 * manufactures that may also be connected to the PC. This list can be accessed, one at a time, via a call
 * to PassThruGetNextDevice.
 *
 * If no devices are found, the return value shall be STATUS_NOERROR and the value pointed to by <pDeviceCount> shall
 * be 0. If <pDeviceCount> is NULL, then the return value shall be ERR_NULL_PARAMETER. If the Pass-Thru Interface
 * does not support this function call, then the return value shall be ERR_NOT_SUPPORTED. The return value
 * ERR_FAILED is a mechanism to return other failures, but this is intended for use during development. A fully compliant
 * SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED or ERR_NOT_SUPPORTED.
 *
 * pDeviceCount
 *     An input set by the application, which points to an unsigned long allocated by the application. Upon
 *     return, the unsigned long shall contain the actual number of devices found.
 *
 * Return Values:
 *   ERR_CONCURRENT_API_CALL  DLL does not support this API function. A J2534 API function has been called before the
 *                            previous J2534 function call has completed.
 *   ERR_NOT_SUPPORTED        A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_NOT_SUPPORTED
 *   ERR_NULL_PARAMETER       NULL pointer supplied where a valid pointer is required
 *   ERR_FAILED               Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1
 *                            Pass-Thru Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR           Function call was successful
 */
long PassThruScanForDevices(unsigned long *pDeviceCount) {

  if(pDeviceCount == NULL) {
    return ERR_NULL_PARAMETER;
  }

  const char *response = apigateway_get("/j2534/passthruscanfordevices");
  syslog(LOG_DEBUG, "j2534_PassThruScanForDevices: REST response: %s", response);

  json_t *root;
  json_error_t error;

  root = json_loads(response, 0, &error);
  free(response);

  if(!root) {
    syslog(LOG_ERR, "j2534_PassThruScanForDevices JSON error on line %d: %s", error.line, error.text);
    return ERR_FAILED;
  }

  if(!json_is_object(root)) {
    syslog(LOG_ERR, "j2534_PassThruScanForDevices: Expected REST response to be a JSON object");
    json_decref(root);
    return ERR_FAILED;
  }

  json_t *things = json_object_get(root, "things");
  if(!json_is_array(things)) {
    syslog(LOG_ERR, "j2534_PassThruScanForDevices: things JSON is not an array");
	json_decref(root);
	return ERR_FAILED;
  }

  unsigned long device_count = json_array_size(things);

  int i;
  for(i=0; i<device_count; i++) {

	json_t *thing, *thingName;
	thing = json_array_get(things, i);
    if(!json_is_object(thing)) {
      syslog(LOG_ERR, "j2534_PassThruScanForDevices: thing element is not an object");
      json_decref(root);
      return ERR_FAILED;
    }

    thingName = json_object_get(thing, "thingName");
    if(!json_is_string(thingName)) {
      syslog(LOG_ERR, "j2534_PassThruScanForDevices: thingName is not a string");
      json_decref(root);
      return ERR_FAILED;
	}

    const char *sThingName = json_string_value(thingName);

    if(strlen(sThingName) >= 80) {
      syslog(LOG_ERR, "j2534_PassThruScanForDevices: thingName must be less than 80 characters");
      return ERR_FAILED;
    }

    syslog(LOG_INFO, "j2534_PassThruScanForDevices: Adding J2534 thing: %s", sThingName);

    SDEVICE sdevice;
    strcpy(sdevice.DeviceName, sThingName);
    sdevice.DeviceDLLFWStatus = DEVICE_DLL_FW_COMPATIBLE;
    sdevice.DeviceConnectMedia = DEVICE_CONN_WIRELESS;
    sdevice.DeviceConnectSpeed = 100000;
    sdevice.DeviceSignalQuality = 100;
    sdevice.DeviceSignalStrength = 100;

    device_list[i] = sdevice;
  }

  *pDeviceCount = device_count;
  return STATUS_NOERROR;
}

/**
 * 7.3.2 PassThruGetNextDevice
 *
 * This function shall, one at a time, return the list of devices found by the most recent call to PassThruScanForDevices.
 * The list of devices may be in any order and the order may be different each time PassThruScanForDevices is called. If
 * the function call PassThruGetNextDevice is successful, the return value shall be STATUS_NOERROR and the structure
 * pointed to by psDevice shall contain the information about the next device. Otherwise, the return value shall reflect the
 * error detected and the structure pointed to by psDevice shall not be altered.
 *
 * The application does NOT need to call PassThruOpen prior to calling this function; it need only call
 * PassThruScanForDevices. Additionally, the application is not required to call PassThruGetNextDevice until the entire
 * list has been returned, it may stop at any time. However, subsequent calls to PassThruGetNextDevice will continue to
 * return the remainder of the list until the entire list has been returned, the DLL has been unloaded, or there has been
 * another call to PassThruScanForDevices.
 *
 *  If the call to PassThruScanForDevices indicated that more than one device is present, then each name in the list shall
 *  be unique. When generating device names, manufacturers should take into consideration that a given device may be
 *  available to more than one PC. These names should also be persistent, as calling
 *  PassThruScanForDevices/PassThruGetNextDevice is NOT a prerequisite for calling PassThruOpen. Manufacturers
 *  should consider documenting how their device names are formulated, so that time-critical applications can know the name
 *  in advance without calling PassThruScanForDevices/PassThruGetNextDevice.
 *
 *  If <psDevice> is NULL, then the return value shall be ERR_NULL_PARAMETER. If there is no more device information to
 *  return, then the return value shall be ERR_EXCEEDED_LIMIT. If the last call to PassThruScanForDevices did not locate
 *  any devices or PassThruScanForDevices was never called, then the return value shall be ERR_BUFFER_EMPTY. If the
 *  Pass-Thru Interface does not support this function call, then the return value shall be ERR_NOT_SUPPORTED. The
 *  return value ERR_FAILED is a mechanism to return other failures, but this is intended for use during development. A fully
 *  compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED or ERR_NOT_SUPPORTED.
 *
 *  psDevice
 *       An input, set by the application, which points to the structure SDEVICE allocated by the application.
 *       Upon return, the structure shall have been updated the as required.
 */



/**
 * 7.3.4 PassThruClose
 *
 * This function shall close the communications to the designated Pass-Thru Device. This function must be successfully
 * called prior to the termination of the application. If the function call is successful, the return value shall be
 * STATUS_NOERROR, the associated Device ID shall be invalidated, and the device shall be in a default state.
 *
 * When entering the default state, the first action shall be to return all pins (connecting physical communication channels to
 * the vehicle on the specified device) to the default state. In the default state: all logical communication channels shall be
 * disconnected (as described in PassThruLogicalDisconnect) and all physical communication channels shall be
 * disconnected (as described in PassThruDisconnect).
 *
 * Any function calls to the Pass-Thru Device after a call to PassThruClose (with the exception of
 * PassThruScanForDevices, PassThruGetNextDevice, and PassThruGetLastError) shall result in the return value
 * ERR_DEVICE_NOT_OPEN. If the corresponding <DeviceID> is not currently open, the return value shall be
 * ERR_DEVICE_NOT_OPEN. If PassThruOpen was successfully called but the <DeviceID> is currently not valid, the
 * return value shall be ERR_INVALID_DEVICE_ID. If the Pass-Thru Device is not currently connected or was previously
 * disconnected without being closed, the return value shall be ERR_DEVICE_NOT_CONNECTED. The return value
 * ERR_FAILED is a mechanism to return other failures, but this is intended for use during development. A fully compliant
 * SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED.
 *
 *  Return Values:
 *
 * 	  ERR_CONCURRENT_API_CALL        A J2534 API function has been called before the previous J2534 function call has completed
 * 	  ERR_DEVICE_NOT_OPEN            PassThruOpen has not successfully been called
 * 	  ERR_INVALID_DEVICE_ID          PassThruOpen has been successfully called, but the current <DeviceID> is not valid
 * 	  ERR_DEVICE_NOT_CONNECTED       Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has,
 * 	                                 at some point, failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 * 	  ERR_FAILED                     Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru Interface
 * 	                                 shall never return ERR_FAILED.
 * 	  STATUS_NOERROR                 Function call was successful
 */


/**
 * The PassThruConnect function is used to establish a logical communication channel between the User
 * Application and the vehicle network(via the PassThru device) using the specified network layer protocol
 * and selected protocol options. Protocols supported can be extended by the Scan Tool vendor.
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
 *  Return Values:
 *
 * 	  ERR_CONCURRENT_API_CALL        A J2534 API function has been called before the previous J2534 function call has completed
 * 	  ERR_DEVICE_NOT_OPEN            PassThruOpen has not successfully been called
 * 	  ERR_INVALID_DEVICE_ID          PassThruOpen has been successfully called, but the current <DeviceID> is not valid
 * 	  ERR_DEVICE_NOT_CONNECTED       Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has,
 * 	                                 at some point, failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 * 	  ERR_NOT_SUPPORTED              DLL does not support this API function. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 * 	  ERR_PROTOCOL_ID_NOT_SUPPORTED  <ProtocolID> value is not supported
 * 	  ERR_PIN_NOT_SUPPORTED          Pin number and/or connector specified is either invalid or unknown
 * 	  ERR_RESOURCE_CONFLICT          Request causes a resource conflict (such as a pin on a connector is already in use, a data link controller is already in use, or requested resource is not present, etc.)
 * 	  ERR_FLAG_NOT_SUPPORTED         <Flags> value(s) are either invalid, unknown, or not appropriate for the current channel (such as setting a flag that is only valid for ISO 9141 on a CAN channel)
 * 	  ERR_BAUDRATE_NOT_SUPPORTED     <BaudRate> is either invalid or unachievable for the current channel)
 * 	  ERR_NULL_PARAMETER             NULL pointer supplied where a valid pointer is required
 * 	  ERR_FAILED                     Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED.
 * 	  STATUS_NOERROR                 Function call was successful
 */
long PassThruConnect(unsigned long ProtocolID, unsigned long Flags, unsigned long *pChannelID) {

  if(pChannelID == NULL) {
    return ERR_NULL_PARAMETER;
  }

  if(j2534_is_connected) {
    return ERR_DEVICE_NOT_CONNECTED;
  }


  //bridge = iotbridge_new();
  //iotbridge_run(bridge);

  // TODO: Terminate all active protocol filters (PASS, BLOCK, and FLOW_CONTROL) and all active perioodic messages
  //       for the specified network protocol before establishing a new instance of the communications channel.
  //
  //       The J2534 API/DLL only supports one instance of a communications channel using a particular
  //       network layer protocol.

  return STATUS_NOERROR;
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
/*
  if(bridge == NULL) {
    return ERR_DEVICE_NOT_CONNECTED;
  }

  free(bridge);
*/
  return STATUS_NOERROR;
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
