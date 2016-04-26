/**
 * ecutools: IoT Automotive Tuning, Diagnostics & Analytics
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
static char j2534_last_error[80] = "";
static int j2534_current_api_call = 0;
static unsigned long j2534_device_count = 0;
static SDEVICE j2534_device_list[50] = {0};

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
 * Parameters:
 *   <pDeviceCount>            An input set by the application, which points to an unsigned long allocated by the application.
 *                             Upon return, the unsigned long shall contain the actual number of devices found.
 *
 * Return Values:
 *   ERR_CONCURRENT_API_CALL   DLL does not support this API function. A J2534 API function has been called before the
 *                             previous J2534 function call has completed.
 *   ERR_NOT_SUPPORTED         A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_NOT_SUPPORTED
 *   ERR_NULL_PARAMETER        NULL pointer supplied where a valid pointer is required
 *   ERR_FAILED                Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1
 *                             Pass-Thru Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR            Function call was successful
 */
long PassThruScanForDevices(unsigned long *pDeviceCount) {

  j2534_current_api_call = 731;

  if(pDeviceCount == NULL) {
    return ERR_NULL_PARAMETER;
  }

  const char *response = apigateway_get("/j2534/passthruscanfordevices");

  json_t *root;
  json_error_t error;

  root = json_loads(response, 0, &error);
  free(response);

  if(!root) {
    sprintf(j2534_last_error, "j2534_PassThruScanForDevices JSON error on line %d: %s", error.line, error.text);
    return ERR_FAILED;
  }

  if(!json_is_object(root)) {
    strcpy(j2534_last_error, "Expected REST response to be a JSON object");
    json_decref(root);
    return ERR_FAILED;
  }

  json_t *things = json_object_get(root, "things");
  if(!json_is_array(things)) {
    strcpy(j2534_last_error, "things JSON element is not an array");
	  json_decref(root);
	  return ERR_FAILED;
  }

  j2534_device_count = json_array_size(things);

  int i;
  for(i=0; i<j2534_device_count; i++) {

	  json_t *thing, *thingName;
	  thing = json_array_get(things, i);
    if(!json_is_object(thing)) {
      strcpy(j2534_last_error, "thing JSON element is not an object");
      json_decref(root);
      return ERR_FAILED;
    }

    thingName = json_object_get(thing, "thingName");
    if(!json_is_string(thingName)) {
      strcpy(j2534_last_error, "thingName is not a string");
      json_decref(root);
      return ERR_FAILED;
	}

    const char *sThingName = json_string_value(thingName);

    if(strlen(sThingName) >= 80) {
      strcpy(j2534_last_error, "thingName must be less than 80 characters");
      return ERR_FAILED;
    }

    SDEVICE sdevice;
    strcpy(sdevice.DeviceName, sThingName);
    sdevice.DeviceDLLFWStatus = DEVICE_DLL_FW_COMPATIBLE;
    sdevice.DeviceConnectMedia = DEVICE_CONN_WIRELESS;
    sdevice.DeviceConnectSpeed = 100000;
    sdevice.DeviceSignalQuality = 100;
    sdevice.DeviceSignalStrength = 100;

    j2534_device_list[i] = sdevice;
  }

  *pDeviceCount = j2534_device_count;

  return unless_concurrent_call(STATUS_NOERROR, 731);
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
 * If the call to PassThruScanForDevices indicated that more than one device is present, then each name in the list shall
 * be unique. When generating device names, manufacturers should take into consideration that a given device may be
 * available to more than one PC. These names should also be persistent, as calling
 * PassThruScanForDevices/PassThruGetNextDevice is NOT a prerequisite for calling PassThruOpen. Manufacturers
 * should consider documenting how their device names are formulated, so that time-critical applications can know the name
 * in advance without calling PassThruScanForDevices/PassThruGetNextDevice.
 *
 * If <psDevice> is NULL, then the return value shall be ERR_NULL_PARAMETER. If there is no more device information to
 * return, then the return value shall be ERR_EXCEEDED_LIMIT. If the last call to PassThruScanForDevices did not locate
 * any devices or PassThruScanForDevices was never called, then the return value shall be ERR_BUFFER_EMPTY. If the
 * Pass-Thru Interface does not support this function call, then the return value shall be ERR_NOT_SUPPORTED. The
 * return value ERR_FAILED is a mechanism to return other failures, but this is intended for use during development. A fully
 * compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED or ERR_NOT_SUPPORTED.
 *
 * Parameters:
 *   <psDevice>                   An input, set by the application, which points to the structure SDEVICE allocated by the application. 
 *                                Upon return, the structure shall have been updated the as required.
 *
 * Return Values:
 *   ERR_CONCURRENT_API_CALL      A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_NOT_SUPPORTED            DLL does not support this API function. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_NULL_PARAMETER           NULL pointer supplied where a valid pointer is required
 *   ERR_EXCEEDED_LIMIT           All the device information collected by the last call to PassThruScanForDevices has been returned
 *   ERR_BUFFER_EMPTY             The last call to PassThruScanForDevices found no devices to be present or PassThruScanForDevices was never called
 *   ERR_FAILED                   Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR               Function call was successful
 */
long PassThruGetNextDevice(SDEVICE *psDevice) {
  int api_call = 732;
  j2534_current_api_call = api_call;
  if(psDevice == NULL) {
    return ERR_NULL_PARAMETER;
  }
  if(j2534_device_count == 0) {
    return ERR_BUFFER_EMPTY;
  }
  int i;
  for(i=0; i<j2534_device_count; i++) {
    if(j2534_device_list[i].DeviceName == *psDevice->DeviceName) {
      *psDevice = j2534_device_list[i];
      return unless_concurrent_call(STATUS_NOERROR, api_call);
    }
    if(i == j2534_device_count) {
      return unless_concurrent_call(ERR_EXCEEDED_LIMIT, api_call);
    }
  }
  return unless_concurrent_call(ERR_BUFFER_EMPTY, api_call);
}

/**
 * 7.3.3 PassThruOpen
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
 * Parameters:
 *     <pName>                        is an input, set by the application, which points to an array of characters allocated by the application where
 *                                    one character shall consume one byte. The application shall set this array to the ASCII character string
 *                                    that consists of an 'API Designation' concatenated with the 'Device name' (either known in advance or
 *                                    obtained via PassThruGetNextDevice). This string shall contain 100 characters or less, including the
 *                                    NULL terminator ($00) and the first byte shall NOT be the NULL terminator. For SAE J2534-1 compliant
 *                                    application, the API Designation is "J2534-1:"" and indicates that the device shall strictly adhere to the
 *                                    functionality specified in this document. (For example, a valid <pName> for a SAE J2534-1 compliant
 *                                    device could be "J2534-1:Acme #25" or "J2534-1:Acme #500 (USB)".)
 *
 *     <pDeviceID>                    is an input, set by the application, which points to an unsigned long allocated by the application. Upon
 *                                    return, the unsigned long shall contain the Device ID, which is to be used as a handle to the device for
 *                                    future function calls.
 *
 * Return Values:
 *     ERR_CONCURRENT_API_CALL        A J2534 API function has been called before the previous J2534 function call has completed
 *     ERR_DEVICE_NOT_OPEN            PassThruOpen has not successfully been called
 *     ERR_INVALID_DEVICE_ID          PassThruOpen has been successfully called, but the current <DeviceID> is not valid
 *     ERR_DEVICE_NOT_CONNECTED       Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has,
 *                                   at some point, failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *     ERR_FAILED                     Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru Interface
 *                                   shall never return ERR_FAILED.
 *     STATUS_NOERROR                 Function call was successful
 */
long PassThruOpen(const char *pName, unsigned long *pDeviceID) {
  return 1;
}

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
 * Parameters:
 *   <DeviceID>                     An input, set by the application, which contains the Device ID returned from PassThruOpen. 
 *
 * Return Values:
 *   ERR_CONCURRENT_API_CALL        A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN            PassThruOpen has not successfully been called
 *   ERR_INVALID_DEVICE_ID          PassThruOpen has been successfully called, but the current <DeviceID> is not valid
 *   ERR_DEVICE_NOT_CONNECTED       Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has,
 * 	                                at some point, failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *   ERR_FAILED                     Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru Interface
 * 	                                shall never return ERR_FAILED.
 * 	 STATUS_NOERROR                 Function call was successful
 */
long PassThruClose(unsigned long DeviceID) {
  return 1;
}

/**
 * 7.3.5 PassThruConnect
 *
 * This function shall establish a physical connection to the vehicle using the specified interface hardware in the designated
 * Pass-Thru Device. If the function call is successful, the return value shall be STATUS_NOERROR, the value pointed to by
 * <pChannelID> shall be used as a handle to the newly established physical communication channel and the channel shall
 * be in an initialized state. Otherwise, the return value shall reflect the error detected and the value pointed to by
 * <pChannelID> shall not be altered. The last action in the process of establishing a physical connection shall be to connect
 * the specified interface hardware in the Pass-Thru Device to the vehicle.
 *
 * The initialized state for a physical communication channel shall be defined to be:
 *   - No logical communication channels shall be associated with the physical communication channel
 *   - All periodic messages shall be cleared and all Periodic Message IDs invalidated
 *   - All filters shall be in the default state and all Filter IDs invalidated
 *   - All transmit and receive queues shall be cleared
 *   - Any active message transmission shall be terminated
 *   - Any message reception in progress shall be terminated
 *   - All configurable parameters for the associated physical communication channel shall be at the default values
 *
 * If the designated <DeviceID> is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If
 * PassThruOpen was successfully called but the <DeviceID> is currently not valid, the return value shall be
 * ERR_INVALID_DEVICE_ID. If the Pass-Thru Device is not currently connected or was previously disconnected without
 * being closed, the return value shall be ERR_DEVICE_NOT_CONNECTED. If the physical communication channel
 * attempting to be established would cause a resource conflict, such as an attempt to use a pin on the SAE J1962
 * connector that is already in use, the return value shall be ERR_RESOURCE_CONFLICT. If the designated pin(s) are not
 * valid for the designated <ProtocolID> or the <Connector> is not J1962_CONNECTOR, the return value shall be
 * ERR_PIN_NOT_SUPPORTED. If the number in <NumOfResources> is invalid, the return value shall be
 * ERR_RESOURCE_CONFLICT. If either <pChannelID> or <ResourceStruct. ResourceListPtr> is NULL, the return value
 * shall be ERR_NULL_PARAMETER. Attempts to use <ProtocolID> values designated as 'reserved' shall result in the
 * return value ERR_PROTOCOL_ID_NOT_SUPPORTED. Attempts to use bits in <Flags> designated as 'reserved' or bits
 * that are not applicable for the <ProtocolID> indicated shall result in the return value ERR_FLAG_NOT_SUPPORTED.
 * Attempts to use a baud rate that is not specified for the <ProtocolID> designated shall result in the return value
 * ERR_BAUDRATE_NOT_SUPPORTED. If the Pass-Thru Interface does not support this function call, then the return
 * value shall be ERR_NOT_SUPPORTED. The return value ERR_FAILED is a mechanism to return other failures, but this
 * is intended for use during development. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return
 * ERR_FAILED or ERR_NOT_SUPPORTED.
 *
 * By default, the Pass-Thru Interface will block all received messages for this physical communication channel until a filter is
 * set. However, this shall not block messages on logical communication channels when they are created on this physical
 * communication channel (see PassThruStartMsgFilter for more details).
 *
 * Parameters:
 *   <DeviceID>                      is an input, set by the application, which contains the Device ID returned from PassThruOpen.
 *   <ProtocolID>                    is an input, set by the application, which contains the Protocol ID for the physical communication
 *                                   channel to be connected (see Figure 27 for valid options). This ID defines how this physical
 *                                   communication channel will interact with the vehicle. Figure 32 defines the J1962 pin usage for each
 *                                   Protocol ID.
 *   <Flags>                         is an input, set by the application, which contains the Flags for the physical communication channel
 *                                   to be connected (see Figure 28 for valid options). These bits can be ORed together and represent
 *                                   the desired physical communication channel configuration.
 *   <BaudRate>                      is an input, set by the application, which contains the initial baud rate value for the physical
 *                                   communication channel to be connected (see Figure 29 for valid options).
 *   <ResourceStruct>                is an input structure, set by the application, which contains connector/pin-out information. Figure 32
 *                                   identifies the specific values that shall be used by an SAE J2534-1 Interface.
 *                                   The RESOURCE_STRUCT, defined in Section 9.18, where:
 *   <Connector>                     is an input, set by the application, which identifies the connector to be used.
 *                                   Figure 30 specifies the list of valid connectors.
 *   <NumOfResources>                is an input, set by the application, which indicates the number of items in the array pointed to by <ResourceListPtr>.
 *   <ResourceListPtr>               is an input, set by the application, which points to an array of unsigned longs
 *                                   also allocated by the application. This array represents the pins used by the
 *                                   protocol. In the array, offset 0 will contain the primary pin and the remainder of
 *                                   the array (starting at offset 1) will contain the secondary/additional pins (if
 *                                   applicable). The contents of the array are protocol/connector specific. Figure
 *                                   31 specifies the pin identification convention that shall be followed for each
 *                                   protocol.
 *   <pChannelID>                    is an input, which points to an unsigned long allocated by the application. Upon return, the unsigned
 *                                   long shall contain the Channel ID, which is to be used as a handle to the physical communication
 *                                   channel for future function calls.
 * Return Values:
 * 	 ERR_CONCURRENT_API_CALL        A J2534 API function has been called before the previous J2534 function call has completed
 * 	 ERR_DEVICE_NOT_OPEN            PassThruOpen has not successfully been called
 * 	 ERR_INVALID_DEVICE_ID          PassThruOpen has been successfully called, but the current <DeviceID> is not valid
 * 	 ERR_DEVICE_NOT_CONNECTED       Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has,
 * 	                                at some point, failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 * 	 ERR_NOT_SUPPORTED              DLL does not support this API function. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 * 	 ERR_PROTOCOL_ID_NOT_SUPPORTED  <ProtocolID> value is not supported
 * 	 ERR_PIN_NOT_SUPPORTED          Pin number and/or connector specified is either invalid or unknown
 * 	 ERR_RESOURCE_CONFLICT          Request causes a resource conflict (such as a pin on a connector is already in use, a data link controller is already in use, or requested resource is not present, etc.)
 * 	 ERR_FLAG_NOT_SUPPORTED         <Flags> value(s) are either invalid, unknown, or not appropriate for the current channel (such as setting a flag that is only valid for ISO 9141 on a CAN channel)
 * 	 ERR_BAUDRATE_NOT_SUPPORTED     <BaudRate> is either invalid or unachievable for the current channel)
 * 	 ERR_NULL_PARAMETER             NULL pointer supplied where a valid pointer is required
 * 	 ERR_FAILED                     Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED.
 * 	 STATUS_NOERROR                 Function call was successful
 */
long PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long BaudRate, RESOURCE_STRUCT ResourceStruct, unsigned long *pChannelID) {

  if(pChannelID == NULL) {
    return ERR_NULL_PARAMETER;
  }

  return STATUS_NOERROR;
}

/**
 * 7.3.6 PassThruDisconnect
 *
 * This function shall terminate a physical connection to the vehicle on the designated Pass-Thru Device. If the function call
 * is successful, the return value shall be STATUS_NOERROR and the physical communication channel shall be in a
 * disconnected state. The first action in the process of terminating a physical connection shall be to disconnect specified
 * interface hardware in the Pass-Thru Device from the vehicle and return the associated pins to the default state (as
 * specified in Section 6.8).
 *
 * The disconnected state for a physical communication channel is defined to be:
 *   - The specified interface hardware in the Pass-Thru Device shall be disconnected from the vehicle and the
 *     associated pins shall be in the default state (as specified in Section 6.8)
 *   - The Physical Communication Channel ID as well as any associated Logical Communication Channel IDs shall be invalidated
 *   - All periodic messages for the associated channel(s) shall be cleared and Periodic Message IDs invalidated
 *   - All filters for the associated channel(s) shall be in the default state and Filter IDs invalidated
 *   - All transmit and receive queues for the associated channel(s) shall be cleared
 *   - Any active message transmission by the associated channel(s) shall be terminated
 *   - Any message reception in progress by the associated channel(s) shall be terminated, and all configurable
 *     parameters for the associated channel(s) shall be at the default values
 *
 * If the corresponding Pass-Thru Device is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If the
 * physical communication channel attempting to be disconnected is invalid, the return value shall be
 * ERR_INVALID_CHANNEL_ID. If the Pass-Thru Device is not currently connected or was previously disconnected without
 * being closed, the return value shall be ERR_DEVICE_NOT_CONNECTED. If the Pass-Thru Interface does not support
 * this function call, then the return value shall be ERR_NOT_SUPPORTED. The return value ERR_FAILED is a mechanism
 * to return other failures, but this is intended for use during development. A fully compliant SAE J2534-1 Pass-Thru
 * Interface shall never return ERR_FAILED or ERR_NOT_SUPPORTED.
 *
 * Parameters:
 *   <ChannelID>                  is an input, set by the application, which contains the Physical Communication Channel ID returned from
 *                                PassThruConnect. 
 * Return Values:
 *   ERR_CONCURRENT_API_CALL      A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN          PassThruOpen has not successfully been called
 *   ERR_INVALID_CHANNEL_ID       Invalid <ChannelID> value
 *   ERR_DEVICE_NOT_CONNECTED     Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has, at some point, failed to
 *                                communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *   ERR_NOT_SUPPORTED            DLL does not support this API function. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_FAILED                   Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru
 *                                Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR               Function call was successful
 */
long PassThruDisconnect(unsigned long ChannelID) {
  return STATUS_NOERROR;
}

/**
 * 7.3.7 PassThruLogicalConnect
 *
 * This function shall establish a logical communication channel to the vehicle on the designated Pass-Thru Device. This
 * logical communication channel overlays an additional protocol scheme on an existing physical communication channel. If
 * the function call is successful, the return value shall be STATUS_NOERROR, the value pointed to by <pChannelID> shall
 * be used as a handle to the newly established channel and that channel shall be in an initialized state. Otherwise, the
 * return value shall reflect the error detected and the value pointed to by <pChannelID> shall not be altered. There can be
 * up to ten logical communication channels per physical communication channel. The act of establishing a logical
 * communication channel shall have no effect on the operation the underlying physical communication channel or any of its
 * currently associated logical communication channels.
 *
 * The initialized state for a logical communication channel shall be defined to be:
 *   - No periodic messages or Periodic Message IDs shall be associated with this logical communication channel
 *   - The transmit and receive queues for this logical communication channel shall be clear
 *   - All configurable parameters for this logical communication channel shall be at the default values
 * 
 * A protocol specific Channel Descriptor structure, pointed to by <pChannelDescriptor>, is used to define the two end points
 * of the logical connection. If the <pChannelDescriptor> is NULL, then the return value shall be ERR_NULL_PARAMETER.
 *
 * If <PhysicalChannelID> is not valid, the return value shall be ERR_INVALID_CHANNEL_ID. If the Pass-Thru Device is
 * not currently connected or was previously disconnected without being closed, the return value shall be
 * ERR_DEVICE_NOT_CONNECTED. If <pChannelID> is NULL, the return value shall be ERR_NULL_PARAMETER. If the
 * designated physical communication channel and <ProtocolID> combination does not allow the creation of a logical
 * communication channel, the return value shall be ERR_LOG_CHAN_NOT_ALLOWED. If the maximum number of logical
 * communication channels for the specified physical communication channel is exceeded, the return value shall be
 * ERR_EXCEEDED_LIMIT. If the <ProtocolID> value is invalid, unknown, or designated as ‘reserved’, the return value shall
 * be ERR_PROTOCOL_ID_NOT_SUPPORTED. If bits in the parameter Flags that are designated as ‘reserved’ or that are
 * not applicable for the <ProtocolID> specified are set, the return value shall be ERR_FLAG_NOT_SUPPORTED. If the
 * network address information in the Channel Descriptor Structure duplicates any previous addresses in the list, the return
 * value shall be ERR_NOT_UNIQUE. If the <LocalAddress> and <RemoteAddress> parameters in the Channel Descriptor
 * Structure are the same, the return value shall be ERR_NOT_UNIQUE. If the Pass-Thru Interface does not support this
 * function call, then the return value shall be ERR_NOT_SUPPORTED. The return value ERR_FAILED is a mechanism to
 * return other failures, but this is intended for use during development. A fully compliant SAE J2534-1 Pass-Thru Interface
 * shall never return ERR_FAILED or ERR_NOT_SUPPORTED.
 *
 * Parameters:
 *   <PhysicalChannelID>             is an input, set by the application, which contains the physical Channel ID
 *                                   returned from PassThruConnect. 
 *   <ProtocolID>                    is an input, set by the application, which contains the Protocol ID for the
 *                                   logical communication channel to be connected (see Figure 35 for valid options).
 *                                   This ID defines how this logical communication channel will interact with the vehicle.
 *                                   This ID is also used to determine the type of structure being pointed to by the parameter pChannelDescriptor.
 *   <Flags>                         is an input, set by the application, which contains the flags for logical communication channel to
 *                                   be connected (see Figure 36 for valid options). These bits can be ORed together and represent
 *                                   the desired logical communication channel configuration.
 *   <pChannelDescriptor>            is an input, set by the application, which points to a structure that defines the logical
 *                                   communication channel to be connected (see Section 7.3.7.5 for more details).
 *   <pChannelID>                    is an input, set by the application, which points to an unsigned long allocated by the application.
 *                                   Upon return, the unsigned long shall contain the Logical Communication Channel ID, which is to
 *                                   be used as a handle to the logical communication channel for future function calls.
 * Return Values: *
 *   ERR_CONCURRENT_API_CALL         A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN             PassThruOpen has not successfully been called
 *   ERR_INVALID_CHANNEL_ID          Invalid <ChannelID> value
 *   ERR_DEVICE_NOT_CONNECTED        Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has, at some point, 
 *                                   failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *   ERR_NOT_SUPPORTED               DLL does not support this API function. A fully compliant SAE J2534-1 PassThru Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_LOG_CHAN_NOT_ALLOWED        Logical communication channel is not allowed for the designated physical communication channel and <ProtocolID> combination
 *   ERR_PROTOCOL_ID_NOT_SUPPORTED   <ProtocolID> value is not supported (either invalid or unknown)
 *   ERR_FLAG_NOT_SUPPORTED          <Flags> value(s) are either invalid, unknown, or not appropriate for the current channel (such as setting a flag that is only
 *                                   valid for ISO 9141 on a CAN channel)
 *   ERR_NULL_REQUIRED               A parameter that is required to be NULL is not set to NULL
 *   ERR_NULL_PARAMETER              NULL pointer supplied where a valid pointer is required
 *   ERR_NOT_UNIQUE                  Attempt was made to create a logical communication channel that duplicates the <SourceAddress> and/or the <TargetAddress> of
 *                                   an existing logical communication channel for the specified physical communication channel
 *   ERR_EXCEEDED_LIMIT              Exceeded the maximum number of logical communication channels for the designated physical communication channel
 *   ERR_FAILED                      Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR                  Function call was successful
 */
long PassThruLogicalConnect(unsigned long PhysicalChannelID, unsigned long ProtocolID, unsigned long flags, void *pChannelDescriptor, unsigned long *pChannelID) {
	return 1;
}

/**
 * 7.3.8 PassThruLogicalDisconnect
 * 
 * This function shall terminate a logical connection to the vehicle on the designated Pass-Thru Device. If the function call is
 * successful, the return value shall be STATUS_NOERROR and the logical communication channel shall be in a
 * disconnected state.
 *
 * The disconnected state for a logical communication channel is defined to be:
 *   - The associated Channel ID shall be invalidated
 *   - All periodic messages shall be cleared and Periodic Message IDs invalidated
 *   - All configurable parameters shall be at the default values
 *   - All messages in the transmit queue shall be discarded and no TxFailed Indication shall be generated
 *   - All messages in the receive queue shall be discarded
 *   - Any active message transmission shall be terminated and no TxFailed Indication shall be generated
 *
 * If the corresponding Pass-Thru Device is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If the
 * logical communication channel attempting to be disconnected is invalid, the return value shall be
 * ERR_INVALID_CHANNEL_ID. If the Pass-Thru Device is not currently connected or was previously disconnected without
 * being closed, the return value shall be ERR_DEVICE_NOT_CONNECTED. If the Pass-Thru Interface does not support
 * this function call, then the return value shall be ERR_NOT_SUPPORTED. The return value ERR_FAILED is a mechanism
 * to return other failures, but this is intended for use during development. A fully compliant SAE J2534-1 Pass-Thru
 * Interface shall never return ERR_FAILED or ERR_NOT_SUPPORTED.
 *
 * Parameters:
 *   <ChannelID>                     is an input, set by the application, which contains the Logical Communication Channel ID returned from PassThruLogicalConnect.
 *
 * Return Values:
 *   ERR_CONCURRENT_API_CALL         A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN             PassThruOpen has not successfully been called
 *   ERR_INVALID_CHANNEL_ID          Invalid <ChannelID> value
 *   ERR_DEVICE_NOT_CONNECTED        Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has, at some point, failed to
 *                                   communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *   ERR_NOT_SUPPORTED               DLL does not support this API function. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_FAILED                      Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR                  Function call was successful
 */
long PassThruLogicalDisconnect(unsigned long ChannelID) {
	return 1;
}

/**
 * 7.3.9 PassThruSelect
 * 
 * This function allows the application to select specific channels to be monitored for available messages (including
 * Indications). The application can specify any combination of Physical or Logical Communication Channels, the minimum
 * number of channels that must have at least one message available, and a timeout value. When PassThruSelect is
 * called, the function shall not return until one of the following happens:
 *
 *   - The <Timeout> expires.
 *   - The number of channels from the <ChannelList> that have at least one message available to be read meets or
 *     exceeds the number of channels specified in <ChannelThreshold>.
 *   - An error occurs.
 *
 * If the function call is successful, the return value shall be STATUS_NOERROR and the channel(s) from the original list
 * that contain at least one message shall be identified in the SCHANNELSET structure.
 *
 * The purpose of PassThruSelect is to allow the application to check and/or wait for messages to become available on a
 * given set of channels without having to constantly call PassThruReadMsgs. This will minimize the transactions between
 * the application and the Pass Thru Interface, thus improving performance. It is important to note, that PassThruSelect will
 * NOT return the messages available, this must still be done with a call to PassThruReadMsgs. However, the advantage is
 * that the application now knows which channels have messages to be read.
 *
 * If the corresponding Pass-Thru Device is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If any
 * communication channel attempting to be monitored is invalid, the return value shall be ERR_INVALID_CHANNEL_ID and
 * the SCHANNELSET structure shall not be altered. If the Pass-Thru Device is not currently connected or was previously
 * disconnected without being closed, the return value shall be ERR_DEVICE_NOT_CONNECTED. If the value of
 * <SelectType> is not set to READABLE_TYPE, the return value shall be ERR_SELECT_TYPE_NOT_SUPPORTED. If the
 * value of <ChannelThreshold> is greater than the value of <ChannelCount>, the return value shall be
 * ERR_EXCEEDED_LIMIT. If either <ChannelSetPtr> or the structure element <ChannelList> is NULL, the the return value
 * shall be ERR_NULL_PARAMETER. If no messages were available to be read on any of the channels specified in the
 * <ChannelList>, the return value shall be ERR_BUFFER_EMPTY. If a non-zero <Timeout> value was specified, the
 * <Timeout> expired, and the number of channels (from the <ChannelList>) that have at least one message available to be
 * read is less than <ChannelThreshold>, then the return value shall be ERR_TIMEOUT. If the Pass-Thru Interface does not
 * support this function call, then the return value shall be ERR_NOT_SUPPORTED. The return value ERR_FAILED is a
 * mechanism to return other failures, but this is intended for use during development. A fully compliant SAE J2534-1 PassThru
 * Interface shall never return ERR_FAILED or ERR_NOT_SUPPORTED.
 *
 * Parameters:
 *   <ChannelSetPtr>                is an input, set by the application, which points to a SCHANNELSET structure (also allocated by the
 *                                  application).
 *   <SelectType>                   is an input, set by the application, which indicates the purpose of channel selection. The only
 *                                  valid value is READABLE_TYPE.
 *                                  READABLE_TYPE selects the associated channels to be monitored for available messages (either
 *                                  incomming messages or Indications.
 *   <Timeout>                      is an input, set by the application, which contains the minimum amount of time (in milliseconds) that the
 *                                  application is willing to wait to see if the desired number of messages are available to be read. A value of 0
 *                                  shall cause the function to return immediately with the list of channels that have messages to be read at
 *                                  the time of the function call – this is equivalent to setting the < ChannelThreshold > to 0. The valid range is
 *                                  0-30000 milliseconds. The precision of the timeout is at least 10 milliseconds and the tolerance is 0 to +50
 *                                  milliseconds.
 * The SCHANNELSET structure is defined in Section 9.20, where:
 *   <ChannelCount>                 is an input, set by the application, which contains the number of Logical and/or Physical
 *                                  Communication Channel IDs contained in the array <ChannelList>. Upon return, this element will contain
 *                                  the number of channels that are left in the array <ChannelList>.
 *   <ChannelThreshold>             is an input, set by the application, which contains the minimum number of channels
 *                                  that have at least one message available to be read. Setting this to a value of 0 will cause the function to
 *                                  return immediately with the list of channels that have at least one message available to be read at the time
 *                                  of the function call – this is equivalent to setting the <Timeout> to 0. The value of this parameter shall be
 *                                  less than or equal to the value of <ChannelCount>.
 *   <ChannelList>                  is an input, set by the application, which is a pointer to an array (also allocated by the
 *                                  application) that contains the list of Logical and/or Physical Communication Channel IDs (obtained from
 *                                  previous calls to PassThruConnect and/or PassThruLogicalConnect) to be monitored. Upon return, this
 *                                  array will contain a subset of the original list of Channel IDs (in no particular order) that have at least one
 *                                  or more messages available to be read.
 *
 * Return Values:
 *   ERR_CONCURRENT_API_CALL        A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN            PassThruOpen has not successfully been called
 *   ERR_INVALID_CHANNEL_ID         Invalid <ChannelID> value
 *   ERR_DEVICE_NOT_CONNECTED       Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has, at some point,
 *                                  failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *   ERR_NOT_SUPPORTED              DLL does not support this API function. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_NULL_PARAMETER             NULL pointer supplied where a valid pointer is required
 *   ERR_SELECT_TYPE_NOT_SUPPORTED  <SelectType> is either invalid or unknown
 *   ERR_EXCEEDED_LIMIT             Exceeded the allowed limits, the value of <ChannelThreshold> is greater than the value of <ChannelCount>
 *   ERR_BUFFER_EMPTY               The receive queues for the requested channels are empty, no messages available to read
 *   ERR_TIMEOUT                    Requested action could not be completed in the designated time NOTE: This only applies when Timeout is non-zero and at least
 *                                  one message on a requested channel is available to be read.
 *   ERR_FAILED                     Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru Interface shall 
 *                                  never return ERR_FAILED.
 *   STATUS_NOERROR                 Function call was successful
 */
long PassThruSelect(SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout) {
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
 * 7.3.18 PassThruGetLastError
 *
 * This function returns the text string description for an error detected during the last function call (except
 * PassThruGetLastError). This mechanism is intended for use during development, as the description may only be
 * meaningful to the application developer and is NOT intended for the user of the application. If the function call is
 * successful, the return value shall be STATUS_NOERROR and the description buffer shall contain an error description
 * string. For all other return values, the contents of the error description string shall not be valid. This function does not
 * require a Device ID or Channel ID, so it may be called at any time.
 *
 * The last error returned is not specific to any particular Channel ID or Device ID and is related to the last function called. It
 * is expected that the application will call this function immediately after a function fails.
 *
 * If <pErrorDescription> is NULL, the return value shall be ERR_NULL_PARAMETER. If this function call is not supported
 * for this device, then the return value shall be ERR_NOT_SUPPORTED. A fully compliant SAE J2534-1 Pass-Thru
 * Interface shall never return ERR_NOT_SUPPORTED.
 *
 * Parameters:
 *   <pErrorDescription>          An input, allocated by the application that points a buffer where the error description will be
 *                                placed. The error description is allocated by the application and must be eighty (80) bytes. The
 *                                Pass-Thru Interface vendor that supplies the DLL shall determine the error description string. A
 *                                valid error description string shall contain 80 ASCII characters (bytes) or less, including the NULL
 *                                terminator ($00), and the first byte shall NOT be the NULL terminator.
 *
 * Return Values:
 *    ERR_CONCURRENT_API_CALL     A J2534 API function has been called before the previous J2534 function call has completed
 *    ERR_NOT_SUPPORTED           Device does not support this API function. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 *    ERR_NULL_PARAMETER          NULL pointer supplied where a valid pointer is required
 *    STATUS_NOERROR              Function call was successful
 */
long PassThruGetLastError(char *pErrorDescription) {
  j2534_current_api_call = 2;
	if(pErrorDescription == NULL) {
    return ERR_NULL_PARAMETER;
  }
  strcpy(pErrorDescription, j2534_last_error);
  return (j2534_current_api_call != 2) ? ERR_CONCURRENT_API_CALL : STATUS_NOERROR;
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


// private helpers; not part of j2534 spec
unsigned long unless_concurrent_call(unsigned long status, int api_call) {
  if(j2534_current_api_call != api_call) {
    strcpy(j2534_last_error, "ERR_CONCURRENT_API_CALL");
    return ERR_CONCURRENT_API_CALL;
  }
  return status;
}