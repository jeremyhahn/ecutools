/**
 * ecutools: IoT Automotive Tuning, Diagnostics & Analytics
 * Copyright (C) 2014 Jeremy Hahn
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

// not j2534 spec
static bool j2534_initialized = false;
static bool j2534_opened = false;
static unsigned long j2534_current_api_call = 0;
static long j2534_device_count = 0;
static char j2534_last_error[80] = {0};
static int *j2534_awsiot_error = NULL;

static SDEVICE j2534_device_list[25] = {0};
static vector j2534_client_vector;
static vector j2534_selected_channels;

unsigned long unless_concurrent_call(unsigned long status, unsigned long api_call) {
  syslog(LOG_DEBUG, "unless_concurrent_call: status=%x, api_call=%d", status, api_call);
  if(j2534_current_api_call != api_call) {
    strcpy(j2534_last_error, "ERR_CONCURRENT_API_CALL");
    return ERR_CONCURRENT_API_CALL;
  }
  return status;
}

void j2534_rxqueue_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData) {

  syslog(LOG_DEBUG, "j2534_rxqueue_handler: topicName=%s, topicNameLen=%u, payload=%s, payload_len=%i",
    topicName, (unsigned int)topicNameLen, params->payload, params->payloadLen);

  j2534_client *client = (j2534_client *)pData;
  vector_add(client->rxQueue, pData);
}

j2534_client* j2534_client_by_channel_id(unsigned long ChannelID) {
  int i;
  for(i=0; i<vector_count(&j2534_client_vector); i++) {
    j2534_client *client = (j2534_client *)vector_get(&j2534_client_vector, i);
    if(client->channelId == ChannelID) {
      return client;
    }
  }
}

j2534_client* j2534_client_by_device_id(unsigned long DeviceID) {
  int i;
  for(i=0; i<j2534_client_vector.count; i++) {
    j2534_client *client = (j2534_client *)vector_get(&j2534_client_vector, i);
    if(client == NULL) continue;
    if(client->deviceId == DeviceID) {
      return client;
    }
  }
  return NULL;
}

bool j2534_is_valid_device_id(unsigned long DeviceID) {
  return j2534_client_by_device_id(DeviceID) != NULL;
}

void j2534_onmessage(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData) {

  syslog(LOG_DEBUG, "j2534_onmessage: topicName=%s, topicNameLen=%u, payload=%s, payload_len=%i", 
    topicName, (unsigned int)topicNameLen, params->payload, params->payloadLen);

  j2534_client *client = (j2534_client *)pData;

  char json[params->payloadLen];
  memcpy(json, params->payload, params->payloadLen);
  json[params->payloadLen] = '\0';

  shadow_message *message = passthru_shadow_parser_parse_state(json);

  if(message->state->reported->j2534->state) {
    client->state = message->state->reported->j2534->state;
    if(client->state == J2534_PassThruOpen) {
      j2534_opened = true;
    }
    if(client->state == J2534_PassThruClose) {
      j2534_opened = false;
    }
  }

  if(message->state->reported->j2534->error) {
    syslog(LOG_DEBUG, "j2534_onmessage: [ERROR] hex=%x, decimal=%d", message->state->reported->j2534->error, message->state->reported->j2534->error);
    j2534_awsiot_error = message->state->reported->j2534->error;
  }

  passthru_shadow_parser_free_message(message);
}

void j2534_onerror(awsiot_client *awsiot, const char *message) {
  syslog(LOG_ERR, "j2534_onerror: message=%s", message);
}

char *filter_json(j2534_client *client) {

  unsigned int json_len = client->filters->count * 27;
  j2534_canfilter *canfilter = NULL;
  char *json = malloc(sizeof(char) * json_len);
  memset(json, '\0', sizeof(char) * json_len);
  strcpy(json, "[");

  int i;
  for(i=0; i<client->filters->count; i++) {

    char tmp_format[json_len];
    char tmp[json_len];

    canfilter = (j2534_canfilter *)vector_get(client->filters, i);

    strcpy(tmp_format, "{\"id\":\"");
    strcat(tmp_format, "%x");
    strcat(tmp_format, "\",");
    strcat(tmp_format, "\"mask\":\"");
    strcat(tmp_format, "%x");
    strcat(tmp_format, "\"}");

    snprintf(tmp, json_len, tmp_format, canfilter->can_id, canfilter->can_mask);
    strcat(json, tmp);

    if(i < client->filters->count-1) {
      strcat(json, ",");
    }
  }
  strcat(json, "]");

  return json;
}

unsigned int j2534_publish_state(j2534_client *client, int desired_state) {

  char *msgfilters = filter_json(client);

  char json_format[255] = "{\"state\":{\"desired\":{\"j2534\":{\"deviceId\":%i,\"state\":%i,\"filters\":%s}}}}";
  unsigned int json_format_len = strlen(json_format) - 4;
  unsigned int json_len = json_format_len + MYINT_LEN(desired_state) + MYINT_LEN(client->deviceId) + strlen(msgfilters);

  char json[json_len+1];
  snprintf(json, json_len+1, json_format, client->deviceId, desired_state, msgfilters);
  json[json_len+1] = '\0';
  free(msgfilters);

  if(awsiot_client_publish(client->awsiot, client->shadow_update_topic, (const char *)json) != 0) {
    syslog(LOG_ERR, "j2534_publish_state: failed to publish. topic=%s, rc=%d", client->shadow_update_topic, client->awsiot->rc);
    return ERR_DEVICE_NOT_CONNECTED;
  }

  unsigned int i = 0;
  while(client->state != desired_state) {

    if(i == 50) {
      syslog(LOG_ERR, "j2534_publish_state: TIMED OUT waiting for device ACK");
      return ERR_DEVICE_NOT_CONNECTED;
    }

    if(j2534_awsiot_error != NULL) {
      j2534_awsiot_error = NULL;
      return j2534_awsiot_error;
    }

    client->awsiot->rc = aws_iot_mqtt_yield(client->awsiot->client, 200);
    if(client->awsiot->rc == NETWORK_ATTEMPTING_RECONNECT) {
      syslog(LOG_DEBUG, "j2534_publish_state: waiting for network to reconnect");
      sleep(1);
      continue;
    }

    i++;
  }

  return STATUS_NOERROR;
} // end not J2534 spec

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
  j2534_current_api_call = J2534_PassThruScanForDevices;

  openlog("ecutools-j2534", LOG_CONS | LOG_PERROR, LOG_USER);

  if(pDeviceCount == NULL) {
    return ERR_NULL_PARAMETER;
  }

  char *response = apigateway_get("/j2534/passthruscanfordevices");

  syslog(LOG_DEBUG, "PassThruScanForDevices: response=%s", response);

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

  j2534_device_count = (unsigned long) json_array_size(things);

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

    strcpy(j2534_device_list[i].DeviceName, sThingName);
    j2534_device_list[i].DeviceAvailable = DEVICE_AVAILABLE;
    j2534_device_list[i].DeviceDLLFWStatus = DEVICE_DLL_FW_COMPATIBLE;
    j2534_device_list[i].DeviceConnectMedia = DEVICE_CONN_WIRELESS;
    j2534_device_list[i].DeviceConnectSpeed = 100000;
    j2534_device_list[i].DeviceSignalQuality = 100;
    j2534_device_list[i].DeviceSignalStrength = 100;
  }

  *pDeviceCount = j2534_device_count;
  syslog(LOG_DEBUG, "PassThruScanForDevices: pDeviceCount=%d, j2534_device_list[0].DeviceName=%s", (*pDeviceCount), j2534_device_list[0].DeviceName);

  return unless_concurrent_call(STATUS_NOERROR, J2534_PassThruScanForDevices);
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
  j2534_current_api_call = J2534_PassThruGetNextDevice;
  if(psDevice == NULL) {
    return unless_concurrent_call(ERR_NULL_PARAMETER, J2534_PassThruGetNextDevice);
  }
  if(j2534_device_count == 0) {
    return unless_concurrent_call(ERR_BUFFER_EMPTY, J2534_PassThruGetNextDevice);
  }
  if(psDevice->DeviceName == NULL || (strncmp("", psDevice->DeviceName, 2) == 0)) {
    *psDevice = j2534_device_list[0];
    return unless_concurrent_call(STATUS_NOERROR, J2534_PassThruGetNextDevice);
  }
  int i;
  for(i=0; i<j2534_device_count; i++) {
    if(strcmp(j2534_device_list[i].DeviceName, psDevice->DeviceName) == 0) {
      if(i+1 >= j2534_device_count) {
        return unless_concurrent_call(ERR_EXCEEDED_LIMIT, J2534_PassThruGetNextDevice);
      }
      *psDevice = j2534_device_list[i+1];
      return unless_concurrent_call(STATUS_NOERROR, J2534_PassThruGetNextDevice);
    }
  }
  return unless_concurrent_call(ERR_BUFFER_EMPTY, J2534_PassThruGetNextDevice);
}

/**
 * 7.3.3 PassThruOpen
 *
 * This function shall establish communications with the designated Pass-Thru Device verifying that it is connected to the PC
 * and initialize it. This function must be successfully called prior to all other function calls to the Pass-Thru Device (with the
 * exception of PassThruScanForDevices, PassThruGetNextDevice, and PassThruGetLastError). If the function call is
 * successful, the return value shall be STATUS_NOERROR, the value pointed to by <pDeviceID> shall be used as a
 * handle to the associated device, and the device shall be in a default state. Otherwise, the return value shall reflect the
 * error detected and the value pointed to by <pDeviceID> shall not be altered.
 *
 * When entering the default state, the first action shall be to return all pins (connecting physical communication channels to
 * the vehicle on the specified device) to the default state. In the default state: all logical communication channels shall be
 * disconnected (as described in PassThruLogicalDisconnect) and all physical communication channels shall be
 * disconnected (as described in PassThruDisconnect). Additionally, the Pass-Thru Interface shall detect when the PassThru
 * Device has become disconnected. Refer to Section 6.1 for the requirements of detecting and reporting a Pass-Thru
 * Device disconnect.
 *
 * SAE J2534-1 compliant applications are limited to a connection to a single Pass-Thru Interface. If <pName> or
 * <pDeviceID> are NULL, the return value shall be ERR_NULL_PARAMETER. If the Pass-Thru Device is not currently
 * connected or was previously disconnected without being closed, the return value shall be
 * ERR_DEVICE_NOT_CONNECTED. If the device is currently open, the return value shall be ERR_DEVICE_IN_USE. If
 * <pName> contains an API Designation that is not supported, an unknown device name, or there is a configuration error
 * with the device being opened (e.g., firmware/DLL mismatch, etc.), the return value shall be ERR_OPEN_FAILED. The
 * return value ERR_FAILED is a mechanism to return other failures, but this is intended for use during development. A fully
 * compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED.
 *
 * If PassThruOpen has never been successfully called or no Pass-Thru Devices are currently open then all subsequent
 * function calls (with the exception of PassThruScanForDevices, PassThruGetNextDevice, and PassThruGetLastError)
 * shall result in the return value ERR_DEVICE_NOT_OPEN.
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
 *     ERR_NULL_PARAMETER             NULL pointer supplied where a valid pointer is required
 *     ERR_DEVICE_NOT_CONNECTED       Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has,
 *                                    at some point, failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *     ERR_DEVICE_IN_USE              Device is currently open
 *     ERR_OPEN_FAILED                The specified name is invalid or there is a configuration issue (e.g., firmware/DLL mismatch, using an API
 *                                    Designation that is not supported, etc.) and the associated device could not be opened.Running the configuration
 *                                    application (for the PassThru Interface) may help identify the issue.
 *     ERR_FAILED                     Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru Interface
 *                                    shall never return ERR_FAILED.
 *     STATUS_NOERROR                 Function call was successful
 */
long PassThruOpen(const char *pName, unsigned long *pDeviceID) {

  syslog(LOG_ERR, "PassThruOpen: pName=%s, pDeviceID=%d", pName, *pDeviceID);

  j2534_current_api_call = J2534_PassThruOpen;

  unsigned int shadow_update_topic_len = PASSTHRU_SHADOW_UPDATE_TOPIC + strlen(pName) + 1;
  unsigned int shadow_update_accepted_topic_len = PASSTHRU_SHADOW_UPDATE_ACCEPTED_TOPIC + strlen(pName) + 1;
  unsigned int shadow_error_topic_len = J2534_ERROR_TOPIC + strlen(pName) + 1;
  unsigned int msg_rx_topic_len = J2534_MSG_RX_TOPIC + strlen(pName) + 1;
  unsigned int msg_tx_topic_len = J2534_MSG_TX_TOPIC + strlen(pName) + 1;

  if(pName == NULL || pDeviceID == NULL) {
    return unless_concurrent_call(ERR_NULL_PARAMETER, J2534_PassThruOpen);
  }

  if(!j2534_initialized) {
    vector_init(&j2534_client_vector);
    vector_init(&j2534_selected_channels);
    j2534_initialized = true;
  }

  if(j2534_client_by_device_id(*pDeviceID) != NULL) {
    return unless_concurrent_call(ERR_DEVICE_IN_USE, J2534_PassThruOpen);
  }

  j2534_client *client = malloc(sizeof(j2534_client));

  client->shadow_update_topic = malloc(sizeof(char) * shadow_update_topic_len);
  client->shadow_update_accepted_topic = malloc(sizeof(char) * shadow_update_accepted_topic_len);
  client->shadow_error_topic = malloc(sizeof(char) * shadow_error_topic_len);
  client->msg_tx_topic = malloc(sizeof(char) * msg_tx_topic_len);
  client->msg_rx_topic = malloc(sizeof(char) * msg_rx_topic_len);

  snprintf(client->shadow_update_topic, shadow_update_topic_len, PASSTHRU_SHADOW_UPDATE_TOPIC, pName);
  snprintf(client->shadow_update_accepted_topic, shadow_update_accepted_topic_len, PASSTHRU_SHADOW_UPDATE_ACCEPTED_TOPIC, pName);
  snprintf(client->shadow_error_topic, shadow_error_topic_len, J2534_ERROR_TOPIC, pName);
  snprintf(client->msg_rx_topic, msg_rx_topic_len, J2534_MSG_RX_TOPIC, pName);
  snprintf(client->msg_tx_topic, msg_tx_topic_len, J2534_MSG_TX_TOPIC, pName);

  client->name = malloc(sizeof(char) * strlen(pName)+1);
  memcpy(client->name, pName, strlen(pName));
  client->name[strlen(pName)] = '\0';

  client->device = malloc(sizeof(SDEVICE));
  client->deviceId = *pDeviceID;
  client->protocolId = 0;
  client->state = NULL;

  client->awsiot = malloc(sizeof(awsiot_client));
  client->awsiot->client = malloc(sizeof(AWS_IoT_Client));
  client->awsiot->certDir = PASSTHRU_CERT_DIR;
  client->awsiot->onopen = NULL;
  client->awsiot->onclose = NULL;
  client->awsiot->ondisconnect = NULL;
  client->awsiot->onmessage = &j2534_onmessage;
  client->awsiot->onerror = &j2534_onerror;

  client->rxQueue = malloc(sizeof(vector));
  client->txQueue = malloc(sizeof(vector));
  vector_init(client->rxQueue);
  vector_init(client->txQueue);

  client->filters = malloc(sizeof(vector));
  vector_init(client->filters);

  client->channelSet = malloc(sizeof(SCHANNELSET));
  client->channelSet->ChannelCount = 0;
  client->channelSet->ChannelThreshold = 0;
  client->channelSet->ChannelList = NULL;

  vector_add(&j2534_client_vector, client);

  // TODO: Check for ERR_OPEN_FAILED conditions: firmware/DLL mismatch, API Designation not supported, etc
  // TODO: Set all pins to default state, disconnect physical and logical channels
  // TODO: Detect and report disconnects

  if(awsiot_client_connect(client->awsiot) != 0) {
    syslog(LOG_ERR, "PassThruOpen: failed to awsiot_client_connect. rc=%d", client->awsiot->rc);
    return unless_concurrent_call(ERR_DEVICE_NOT_CONNECTED, J2534_PassThruOpen);
  }

  if(awsiot_client_subscribe(client->awsiot, client->shadow_update_accepted_topic, j2534_onmessage, client) != 0) {
    syslog(LOG_ERR, "j2534_publish_state: failed to subscribe. topic=%s, rc=%d", client->shadow_update_accepted_topic, client->awsiot->rc);
    return unless_concurrent_call(ERR_DEVICE_NOT_CONNECTED, J2534_PassThruOpen);
  }

  if(awsiot_client_subscribe(client->awsiot, client->shadow_error_topic, j2534_onmessage, client) != 0) {
    syslog(LOG_ERR, "j2534_publish_state: failed to subscribe. topic=%s, rc=%d", client->shadow_error_topic, client->awsiot->rc);
    return unless_concurrent_call(ERR_DEVICE_NOT_CONNECTED, J2534_PassThruOpen);
  }

  return unless_concurrent_call(
    j2534_publish_state(client, J2534_PassThruOpen),
    J2534_PassThruOpen
  );
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

  j2534_current_api_call = J2534_PassThruClose;

  if(!j2534_opened) {
    return unless_concurrent_call(ERR_DEVICE_NOT_OPEN, J2534_PassThruClose);
  }

  j2534_client *client = j2534_client_by_device_id(DeviceID);
  if(client == NULL) {
    return unless_concurrent_call(ERR_INVALID_DEVICE_ID, J2534_PassThruClose);
  }

  unsigned long publish_state = j2534_publish_state(client, J2534_PassThruClose);

  if(awsiot_client_unsubscribe(client->awsiot, client->shadow_update_accepted_topic) != 0) {
    syslog(LOG_ERR, "PassThruClose: failed to unsubscribe. topic=%s, rc=%d", client->shadow_update_accepted_topic, client->awsiot->rc);
    return ERR_DEVICE_NOT_CONNECTED;
  }

  if(awsiot_client_unsubscribe(client->awsiot, client->shadow_error_topic) != 0) {
    syslog(LOG_ERR, "PassThruClose: failed to unsubscribe. topic=%s, rc=%d", client->shadow_error_topic, client->awsiot->rc);
    return ERR_DEVICE_NOT_CONNECTED;
  }

  unsigned long response = unless_concurrent_call(
    j2534_publish_state(client, J2534_PassThruClose),
    J2534_PassThruClose
  );

  free(client->channelSet);
  free(client->txQueue);
  free(client->rxQueue);
  free(client->filters);
  free(client->name);
  free(client->device);
  free(client->awsiot);
  free(client);

  return response;
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
 * ERR_RESOURCE_CONFLICT. If either <pChannelID> or <ResourceStruct.ResourceListPtr> is NULL, the return value
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
 *       <Connector>                     is an input, set by the application, which identifies the connector to be used.
 *                                       Figure 30 specifies the list of valid connectors.
 *       <NumOfResources>                is an input, set by the application, which indicates the number of items in the array pointed to by <ResourceListPtr>.
 *       <ResourceListPtr>               is an input, set by the application, which points to an array of unsigned longs
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

  j2534_current_api_call = J2534_PassThruConnect;

  if(pChannelID == NULL) {
    return unless_concurrent_call(ERR_NULL_PARAMETER, J2534_PassThruConnect);
  }

  if(!j2534_opened) {
    return unless_concurrent_call(ERR_DEVICE_NOT_OPEN, J2534_PassThruConnect);
  }

  j2534_client *client = j2534_client_by_device_id(DeviceID);
  if(client == NULL) {
    return unless_concurrent_call(ERR_INVALID_DEVICE_ID, J2534_PassThruConnect);
  }

  client->protocolId = ProtocolID;

  /*
  if(ProtocolID != J1850VPW && ProtocolID != J1850PWM && ProtocolID != ISO9141 &&
     ProtocolID != ISO14230 && ProtocolID != CAN && ProtocolID != J2610 &&
     ProtocolID != ISO15765_LOGICAL) {

    return unless_concurrent_call(ERR_PROTOCOL_ID_NOT_SUPPORTED, J2534_PassThruConnect);
  }*/

  if(ProtocolID != CAN) {
    return unless_concurrent_call(ERR_PROTOCOL_ID_NOT_SUPPORTED, J2534_PassThruConnect);
  }

  if(ResourceStruct.Connector != J1962_CONNECTOR) {
    return unless_concurrent_call(ERR_PIN_NOT_SUPPORTED, J2534_PassThruConnect);
  }

  client->channelId = *pChannelID;

  return unless_concurrent_call(
    j2534_publish_state(client, J2534_PassThruConnect),
    J2534_PassThruConnect
  );
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

  j2534_current_api_call = J2534_PassThruDisconnect;
  j2534_client *client = j2534_client_by_channel_id(ChannelID);

  if(!j2534_opened) {
    return unless_concurrent_call(ERR_DEVICE_NOT_OPEN, J2534_PassThruDisconnect);
  }

  if(ChannelID != client->channelId) {
    return unless_concurrent_call(ERR_INVALID_CHANNEL_ID, J2534_PassThruDisconnect);
  }

  return unless_concurrent_call( 
    j2534_publish_state(client, J2534_PassThruDisconnect),
    J2534_PassThruDisconnect
  );
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
	return ERR_NOT_SUPPORTED;
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
	return ERR_NOT_SUPPORTED;
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
 *
 * The SCHANNELSET structure is defined in Section 9.20, where:
 *
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
	
  j2534_client *client = NULL;
  int i, j = 0;
  struct timeval start, stop;
  float milliseconds = 0;

  j2534_current_api_call = J2534_PassThruSelect;

  if(SelectType != READABLE_TYPE) {
    return unless_concurrent_call(ERR_SELECT_TYPE_NOT_SUPPORTED, J2534_PassThruSelect);
  }

  if(ChannelSetPtr == NULL) {
    return unless_concurrent_call(ERR_NULL_PARAMETER, J2534_PassThruSelect);
  }

  if(ChannelSetPtr->ChannelThreshold > ChannelSetPtr->ChannelCount) {
    return unless_concurrent_call(ERR_EXCEEDED_LIMIT, J2534_PassThruSelect);
  }

  for(i=0; i<ChannelSetPtr->ChannelCount; i++) {

    if(ChannelSetPtr->ChannelList[i] == NULL) {
      return unless_concurrent_call(ERR_NULL_PARAMETER, J2534_PassThruSelect);
    }

    client = j2534_client_by_channel_id(ChannelSetPtr->ChannelList[i]);
    if(client == NULL) {
      return unless_concurrent_call(ERR_INVALID_CHANNEL_ID, J2534_PassThruSelect);
    }

    vector_add(&j2534_selected_channels, client);
  }

  int publish_state_response = j2534_publish_state(client, J2534_PassThruSelect);
  if(publish_state_response != STATUS_NOERROR) return publish_state_response;

  gettimeofday(&start, NULL);
  while(milliseconds < Timeout && milliseconds < J2534_TIMEOUT_MILLIS) {

    for(i=0; i<j2534_selected_channels.count; i++) {

      client = (j2534_client *)vector_get(&j2534_selected_channels, i);

      if(client->rxQueue->count >= ChannelSetPtr->ChannelThreshold) {
        client->channelSet->ChannelCount = client->rxQueue->count;
        ChannelSetPtr = client->channelSet;
        return unless_concurrent_call(STATUS_NOERROR, J2534_PassThruSelect);
      }

      usleep(1000);
    }

    gettimeofday(&stop, NULL);
    milliseconds = (stop.tv_sec - start.tv_sec) * 1000.0f + (stop.tv_usec - start.tv_usec) / 1000.0f;

    syslog(LOG_DEBUG, "PassThruSelect: milliseconds=%f, ChannelSetPtr->ChannelThreshold=%d, client->ChannelSet->ChannelCount=%d",
      milliseconds, ChannelSetPtr->ChannelThreshold, client->channelSet->ChannelCount);
  }

  unsigned long response = (client->channelSet->ChannelCount) ? ERR_TIMEOUT : ERR_BUFFER_EMPTY;
  return unless_concurrent_call(response, J2534_PassThruSelect);
}

/**
 * 7.3.10 PassThruReadMsgs
 * 
 * This function shall read messages and Indications (special messages generated to report specific events) from the
 * designated channel. If the function call is successful, the return value shall be STATUS_NOERROR, the array pointed to
 * by <pMsgs> shall be updated, and the value pointed to by <pNumMsgs> shall be updated to reflect the number
 * messages read. When messages/Indications are returned, they shall be returned in chronological order, with the oldest at
 * offset 0. Only complete messages shall be returned. If an error occurs during this function call, the function shall
 * immediately return with the applicable return value. When the return value is ERR_BUFFER_OVERFLOW or
 * ERR_TIMEOUT, <pNumMsgs> shall be updated to reflect the number of messages returned and the array pointed to by
 * <pMsgs> shall be updated to contain the associated messages. Otherwise, <pNumMsgs> shall point to the value of zero
 * and the array pointed to by <pMsgs> shall not be altered. For physical communication channels, the Pass-Thru Interface
 * will, by default, block all received messages until a filter is set. However, this will not inhibit the reception of messages for
 * any corresponding logical communication channels. Filtering on the physical communication channel shall have no impact
 * on message reception for any corresponding logical communication channels (see PassThruStartMsgFilter for more
 * details).
 *
 * This function shall only return messages/Indications associated with the designated channel. Therefore, a physical
 * communication channel shall only return messages/Indications from its receive queue and not those from any associated
 * logical communication channels. Likewise, logical communication channels cannot return messages/Indications from the
 * underlying physical communication channel. 
 *
 * When using this function, the application can either choose to get only the messages/Indications that are available at the
 * time of the function call (known as a 'non-blocking read') or wait a specific amount of time for a specific number of
 * messages/Indications before returning (known as a 'blocking read').
 *
 * Non-blocking reads are specified by a value of zero for the parameter Timeout. During non-blocking reads, the function
 * shall retrieve the messages that are currently in the queue (up to the number requested) and return immediately. During a
 * non-blocking read, the function shall not wait for additional messages nor shall the return value ever be ERR_TIMEOUT.
 * Figure 41 details various conditions for returning data from a non-blocking read as long as there were no other errors.
 *
 * If the corresponding Pass-Thru Device is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If
 * <ChannelID> is not valid, the return value shall be ERR_INVALID_CHANNEL_ID. If the Pass-Thru Device is not currently
 * connected or was previously disconnected without being closed, the return value shall be
 * ERR_DEVICE_NOT_CONNECTED. If either <pMsg> or <pNumMsgs> are NULL, the return value shall be
 * ERR_NULL_PARAMETER. For either blocking or non-blocking reads, returning a message that has the
 * BUFFER_OVERFLOW bit in <RxStatus> set is considered an error and the return value shall be
 * ERR_BUFFER_OVERFLOW. If the message being read is larger than the size indicated by <DataBufferSize>, then only
 * the number of bytes indicated by <DataBufferSize> shall be placed in the array <DataBuffer> and the return value shall be
 * ERR_BUFFER_TOO_SMALL; the remainder of the message bytes will be discarded. If no messages were available to be
 * read, the return value shall be ERR_BUFFER_EMPTY. If, during a blocking read the requested timeout has expired and
 * more than one message but less than the total requested were available to be read, the return value shall be
 * ERR_TIMEOUT. If this function call is not supported for this channel, then the return value shall be
 * ERR_NOT_SUPPORTED. The return value ERR_FAILED is a mechanism to return other failures, but this is intended for
 * use during development. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED or
 * ERR_NOT_SUPPORTED.
 *
 * Parameters:
 *   <ChannelID>                      is an input, set by the application that contains the handle to the physical or logical
 *                                    communication channel from which messages are to be read. The Channel ID was assigned by
 *                                    a previous call to PassThruConnect or PassThruLogicalConnect. 
 *   <pMsg>                           is an input, set by the application, which points to the array of message structure(s)
 *                                    (allocated by the application) where the messages that have been read out of the Pass-Thru
 *                                    Interface are to be stored. The application must be sure to allocate memory for the <DataBuffer>
 *                                    and set the <DataBufferSize> according, for each message in the array.
 *   <pNumMsgs>                       is an input, set by the application, which initially points to an unsigned long that contains
 *                                    the maximum size of the <pMsg> array. Upon return, the unsigned long shall contain the actual 
 *                                    number of messages read.
 *   <Timeout>                        is an input, set by the application, which contains the minimum amount of time (in milliseconds)
 *                                    that the application is willing to wait to receive the desired number of messages/Indications.
 *                                    A value of 0 specifies a non-blocking read. A non-zero value specifies a blocking read. The
 *                                    valid range is 0-30000 milliseconds. The precision of the timeout is at least 10 milliseconds
 *                                    and the tolerance is 0 to +50 milliseconds.
 *
 * Return Values:
 *   ERR_CONCURRENT_API_CALL          A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN              PassThruOpen has not successfully been called
 *   ERR_INVALID_CHANNEL_ID           Invalid <ChannelID> value
 *   ERR_DEVICE_NOT_CONNECTED         Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has, at
 *                                    some point, failed to communicate with the Pass-Thru Device – even though it may not currently
 *                                    be disconnected.
 *   ERR_NOT_SUPPORTED                Device does not support this API function for the associated <ChannelID>. A fully compliant
 *                                    SAE J2534-1 Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_NULL_PARAMETER               NULL pointer supplied where a valid pointer is required
 *   ERR_BUFFER_OVERFLOW              Indicates a buffer overflow occurred and messages were lost (<pNumMsgs> points to the actual
 *                                    number of messages read)
 *   ERR_BUFFER_TOO_SMALL             The size of <DataBuffer>, as indicated by the parameter <DataBufferSize> in the PASSTHRU_MSG
 *                                    structure, is too small to accommodate the full message
 *   ERR_BUFFER_EMPTY                 The receive queue is empty, no messages available to read
 *   ERR_TIMEOUT                      Requested action could not be completed in the designated time (<pNumMsgs> point to the actual
 *                                    number of messages read) NOTE: This only applies when Timeout is non-zero and at least one
 *                                    message has been read.
 *   ERR_FAILED                       Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1
 *                                    Pass-Thru Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR                   Function call was successful
 */
long PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout) {
	return ERR_NOT_SUPPORTED;
}

/**
 * 7.3.11 PassThruQueueMsgs
 * 
 * This function shall queue messages to the Pass-Thru Interface for transmission on the designated channel. If the function
 * call is successful, the return value shall be STATUS_NOERROR and the value pointed to by <pNumMsgs> shall be
 * updated to reflect the actual number of messages queued as a result of this function call. If an error occurs during this
 * function call, the function shall immediately return with the applicable return value and the value pointed to by
 * <pNumMsgs> shall be updated to reflect the actual number of messages queued as a result of this function call. The
 * Pass-Thru Interface shall not modify structures pointed to by <pMsg>. This function replaces PassThruWriteMsgs, which
 * was used in earlier versions of the SAE J2534 API.
 *
 * This function shall place messages in the transmit queue in the order they were passed in (that is, the message at offset 0
 * shall be first, the message at offset 1 shall be second, and so on). These messages shall follow the format specified in
 * Section 7.2.4, including the actions to be taken when messages violate the specified format.
 *
 * The entire message must fit in the transmit queue for it to be transferred. If all the requested messages do not fit in the
 * queue, the return value shall be ERR_BUFFER_FULL and the value pointed to by <pNumMsgs> shall be updated to
 * reflect the number of messages that were queued as a result of this function call. The function shall not wait for space to
 * become available to queue messages.
 *
 * Note that messages passed to the Pass-Thru Interface using this function call have lower priority than those passed via
 * PassThruStartPeriodicMsg (see Section 6.10.2 for more details). Also, some protocols will generate Indications when
 * transmitting (see Section 7.2.6 for more details).
 *
 * ISO 15765 logical communication channels can queue a Single Frame whose network address or <TxFlags> do not
 * match the <RemoteAddress> or <RemoteTxFlags> provided during channel creation. However, an attempt to queue a
 * Segmented Message shall result in the return value ERR_MSG_NOT_ALLOWED. 
 *
 * For SAE J2610 channels, it shall be an error if the programmable voltage generator is in use when a message with
 * SCI_TX_VOLTAGE is set to 1 in <TxFlags> is becoming active. In this case, if a TxDone Indication was required for the
 * associated message then a TxFailed Indication shall be generated.
 *
 * If the corresponding Pass-Thru Device is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If
 * <ChannelID> is not valid, the return value shall be ERR_INVALID_CHANNEL_ID. If the Pass-Thru Device is not
 * currently connected or was previously disconnected without being closed, the return value shall be
 * ERR_DEVICE_NOT_CONNECTED. If either <pMsg> or <pNumMsgs> are NULL, the return value shall be
 * ERR_NULL_PARAMETER. If the <ProtocolID> in the PASSTHRU_MSG structure (for any message passed into this
 * function) does not match the <ProtocolID> for the associated channel, the return value shall be
 * ERR_MSG_PROTOCOL_ID. If any message passed into this function does not follow the format specified in Section
 * 7.2.4, the return value shall be ERR_INVALID_MSG. If the initial value pointed to by <pNumMsgs> is 0 then the function
 * shall take no action and immediately return the value STATUS_NOERROR. If this function call is not supported for this
 * channel, then the return value shall be ERR_NOT_SUPPORTED. The return value ERR_FAILED is a mechanism to
 * return other failures, but this is intended for use during development. A fully compliant SAE J2534-1 Pass-Thru Interface
 * shall never return ERR_FAILED or ERR_NOT_SUPPORTED.
 *
 * Parameters:
 *   <ChannelID>                     is an input, set by the application that contains the handle to the physical or logical
 *                                   communication channel where the messages are to be queued. The Channel ID was assigned 
 *                                   by a previous call to PassThruConnect or PassThruLogicalConnect.
 *   <pMsg>                          is an input, set by the application, which points to an array of message structure(s)
 *                                   (allocated by the application) to be queued by the Pass-Thru Interface.
 *   <pNumMsgs>                      is an input, set by the application, which initially points to an unsigned long that
 *                                   contains the number of messages to be queued. Upon return, the unsigned long shall
 *                                   contain the actual number of messages that were successfully queued.
 *
 * Return Values:
 *   ERR_CONCURRENT_API_CALL         A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN             PassThruOpen has not successfully been called
 *   ERR_DEVICE_NOT_CONNECTED        Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has,
 *                                   at some point, failed to communicate with the Pass-Thru Device – even though it may not
 *                                   currently be disconnected.
 *   ERR_NOT_SUPPORTED               Device does not support this API function for the associated <ChannelID>. A fully compliant
 *                                   SAE J2534-1 Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_NULL_PARAMETER              NULL pointer supplied where a valid pointer is required
 *   ERR_MSG_PROTOCOL_ID             <ProtocolID> in the PASSTHRU_MSG structure does not match the <ProtocolID> from the original
 *                                   call to PassThruConnect for the <ChannelID>
 *   ERR_INVALID_MSG                 Message structure is invalid for the given <ChannelID> (refer to Section 7.2.4 for more details)
 *   ERR_MSG_NOT_ALLOWED             Attempting to queue a Segmented Message whose network address and/or <TxFlags> does not match
 *                                   those defined for the <RemoteAddress> or <RemoteTxFlags> during channel creation on a logical
 *                                   communication channel (This Return Value is only applicable to ISO 15765 logical communication channels)
 *   ERR_BUFFER_FULL                 Transmit queue is full (<pNumMsgs> points to the actual number of messages queued)
 *   ERR_FAILED                      Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1
 *                                   Pass-Thru Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR                  Function call was successful
 */
long PassThruQueueMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs) {
	return ERR_NOT_SUPPORTED;
}

/**
 * 7.3.12 PassThruStartPeriodicMsg
 * 
 * This function shall direct the Pass-Thru Interface to create the specified periodic message transmission, at the specified
 * time interval, on the designated channel. If the function is successful, the return value shall be STATUS_NOERROR and
 * a new periodic message shall have been created (even if it duplicates an existing periodic message) and immediately
 * queued for transmission without delay. If an error occurs during this function call, the function shall immediately return with
 * the applicable return value. The Pass-Thru Interface shall not modify structures pointed to by <pMsg>. Periodic messages
 * can be started on both physical and logical channels. Each physical communication channel and all of its corresponding
 * logical communication channels (if any) shall share a common pool of exactly ten periodic messages. Physical
 * communication channels that do not support logical communication channels shall have exactly ten periodic messages.
 *
 * Messages passed to the Pass-Thru Interface using PassThruStartPeriodicMsg shall have a higher priority than those
 * passed via PassThruQueueMsgs (see Section 6.10.2 for more details). Periodic messages shall remain in the periodic
 * queue until they can be transmitted without violating bus idle timing parameters (like P3_MIN) or other protocol specific
 * requirements.
 * 
 * The time interval for a periodic message is the time between the end of a successful transmission and the start of its next
 * transmission. Once a periodic message has been queued for transmission, the associated time interval does not start until
 * that message has been successfully transmitted. (This eliminates the possibility that multiple copies of the same periodic
 * message will ‘stack-up’ in the event that the message cannot be immediately transmitted.)
 *
 * For ISO 15765 logical channels, periodic messages shall be interjected into a Segmented Message so long as it does not
 * violate the protocol. This means that a periodic message targeted for ‘Address Z shall have the following impact upon a
 * segmented transfer in progress between the Pass-Thru Device and the designated address (as detailed in Figure 45):
 *
 * In cases where no interjection is allowed, the periodic message shall remain in the periodic queue until it can be safely
 * transmitted without violating any protocol specific requirements. However, periodic messages from one logical
 * communication channel may be interleaved with Segmented Messages from other logical communication channels that
 * share the same physical communication channel. As noted in Section 6.10.2, periodic message transmit queues for a
 * given logical communication channel shall be serviced in a round-robin fashion.
 *
 * Applications must call PassThruStopPeriodicMsg to stop periodic messages individually or PassThruIoctl with the
 * IOCTL ID of CLEAR_PERIODIC_MSGS to stop all periodic messages.
 *
 * Periodic messages shall follow the format specified in Section 7.2.4, including the actions to be taken when messages
 * violate the specified format. If enabled, periodic messages shall generate TxDone/TxFailed Indications upon transmission.
 *
 * Logical communication channels for ISO 15765 can queue a Single Frame periodic message whose network address or
 * <TxFlags> do not match the <RemoteAddress> or <RemoteTxFlags> provided during channel creation. However, an
 * attempt to queue a Segmented Periodic Message shall result in the return value ERR_MSG_NOT_ALLOWED.
 *
 * If the corresponding Pass-Thru Device is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If
 * <ChannelID> is not is not valid, the return value shall be ERR_INVALID_CHANNEL_ID. If the Pass-Thru Device is not
 * currently connected or was previously disconnected without being closed, the return value shall be
 * ERR_DEVICE_NOT_CONNECTED. If either <pMsg> or <pMsgID> are NULL, the return value shall be
 * ERR_NULL_PARAMETER. If the <ProtocolID> in the PASSTHRU_MSG structure (for any message passed into this
 * function) does not match the <ProtocolID> for the associated channel, the return value shall be
 * ERR_MSG_PROTOCOL_ID. If any message passed into this function does not follow the format specified in Section
 * 7.2.4, the return value shall be ERR_INVALID_MSG. If the value for <TimeInterval> is outside the specified range, the
 * return value shall be ERR_TIME_INTERVAL_NOT_SUPPORTED. If the application attempts to create more periodic
 * messages than are allowed, the return value shall be ERR_EXCEEDED_LIMIT. If this function call is not supported for
 * this channel, then the return value shall be ERR_NOT_SUPPORTED. The return value ERR_FAILED is a mechanism to
 * return other failures, but this is intended for use during development. A fully compliant SAE J2534-1 Pass-Thru Interface
 * shall never return ERR_FAILED or ERR_NOT_SUPPORTED.
 *
 * Application developers should carefully consider how and when to use periodic messages. Developers should be aware
 * that periodic messages are asynchronous to application messages.
 *
 * Parameters:
 *   <ChannelID>                        is an input, set by the application that contains the handle to the physical or logical
 *                                      communication channel from which messages are to be read. The Channel ID was assigned
 *                                      by a previous call to PassThruConnect or PassThruLogicalConnect.
 *   <pMsg>                             is an input, set by the application, which points to the message structure (allocated
 *                                      by the application) containing the desired periodic message.
 *   <pMsgID>                           is an input, set by the application, which points to an unsigned long allocated by the application.
 *                                      Upon return, the unsigned long shall contain the Message ID, which is to be used as a 
 *                                      handle to this periodic message for future function calls.
 *   <TimeInterval>                     is an input, set by the application, which contains the time interval between the end of the periodic
 *                                      message transmission and the start of the next time this periodic message becomes active, in
 *                                      milliseconds. The valid range is 5 to 65535 milliseconds. The precision of the time interval is at least 1
 *                                      millisecond and the tolerance is ±1 millisecond (assuming no additional protocol delays are imposed –
 *                                      such as, delays due to arbitration, intra-message delays, etc.).
 * Return Values:
 *   ERR_CONCURRENT_API_CALL            A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN                PassThruOpen has not successfully been called
 *   ERR_INVALID_CHANNEL_ID             Invalid <ChannelID> value
 *   ERR_DEVICE_NOT_CONNECTED           Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has, at
 *                                      some point, failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *   ERR_NOT_SUPPORTED                  Device does not support this API function for the associated <ChannelID>. A fully compliant SAE J2534-1
 *                                      Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_NULL_PARAMETER                 NULL pointer supplied where a valid pointer is required
 *   ERR_MSG_PROTOCOL_ID                <ProtocolID> in the PASSTHRU_MSG structure does not match the <ProtocolID> from the original
 *                                      call to PassThruConnect for the <ChannelID>
 *   ERR_INVALID_MSG                    Message structure is invalid for the given <ChannelID> (refer to Section 7.2.4 for more details)
 *   ERR_MSG_NOT_ALLOWED                Attempting to queue a Segmented Message whose network address and/or <TxFlags> does not match
 *                                      those defined for the <RemoteAddress> or <RemoteTxFlags> during channel creation on a logical
 *                                      communication channel (This Return Value is only applicable to ISO 15765 logical communication channels)
 *   ERR_TIME_INTERVAL_NOT_SUPPORTED    Value for the <TimeInterval> is either invalid or out of range for the current channel)
 *   ERR_EXCEEDED_LIMIT                 Exceeded the maximum number of periodic message IDs
 *   ERR_FAILED                         Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru
 *                                      Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR                     Function call was successful
 */
long PassThruStartPeriodicMsg(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval) {
	return ERR_NOT_SUPPORTED;
}

/**
 * 7.3.13 PassThruStopPeriodicMsg
 * 
 * This function shall delete the specified periodic message transmission on the designated channel. If the function call is
 * successful, the return value shall be STATUS_NOERROR, the associated <MsgID> shall be invalidated, the interval timer
 * shall be stopped, and the associated periodic message shall be removed (if present) from the periodic queue. This
 * function shall not terminate the associated periodic message if it is currently active.
 *
 * If the corresponding Pass-Thru Device is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If
 * <ChannelID> is not is not valid, the return value shall be ERR_INVALID_CHANNEL_ID. If the Pass-Thru Device is not
 * currently connected or was previously disconnected without being closed, the return value shall be
 * ERR_DEVICE_NOT_CONNECTED. If <MsgID> is not is not valid, the return value shall be ERR_INVALID_MSG_ID. If
 * this function call is not supported for this channel, then the return value shall be ERR_NOT_SUPPORTED. The return
 * value ERR_FAILED is a mechanism to return other failures, but this is intended for use during development. A fully
 * compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED or ERR_NOT_SUPPORTED.
 * 
 * Parameters:
 *   <ChannelID>                       is an input, set by the application that contains the handle to the physical or logical communication channel
 *                                     from which messages are to be read. The Channel ID was assigned by a previous call to
 *                                     PassThruConnect or PassThruLogicalConnect. 
 *   <MsgID>                           is an input, set by the application, which contains the Message ID (assigned by PassThruStartPeriodicMsg) that is to be stopped.
 * Return Values:
 *   ERR_CONCURRENT_API_CALL           A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN               PassThruOpen has not successfully been called
 *   ERR_INVALID_CHANNEL_ID            Invalid <ChannelID> value
 *   ERR_DEVICE_NOT_CONNECTED          Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has, at some point, failed
 *                                     to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *   ERR_NOT_SUPPORTED                 Device does not support this API function for the associated <ChannelID>. A fully compliant SAE J2534-1 Pass-Thru
 *                                     Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_INVALID_MSG_ID                Invalid <MsgID> value
 *   ERR_FAILED                        Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru
 *                                     Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR                    Function call was successful
 */
long PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID) {
	return ERR_NOT_SUPPORTED;
}

/**
 * 7.3.14 PassThruStartMsgFilter
 *
 * This function shall add the specified filter to the message evaluation process for the designated physical communication
 * channel. (Message filters are not applicable to logical communication channels. Setting a filter on a physical
 * communication channel shall not impact the operation of any associated logical communication channels.) If the function
 * is successful, the return value shall be STATUS_NOERROR and a new filter shall be created and used to evaluate
 * messages. Indications are not processed by the filter mechanism and shall be placed directly in the receive queue. The
 * Pass-Thru Interface shall support exactly ten filters per physical communication channel. Message filters shall not affect
 * message transmissions.
 *
 * There are two types of filters: pass (PASS_FILTER) and block (BLOCK_FILTER). Pass and block filters are not valid on
 * ISO 15765 logical communication channels. They are used to determine which of the incoming messages shall be placed
 * in the receive queue. These filter types require two PASSTHRU_MSG structures, one for the pattern message
 * (<pPatternMsg>) and the other for the mask message (<pMaskMsg>). The <TxFlags>, <DataLength>, and the
 * <DataBuffer> elements will be use when defining a filter.
 *
 * Figure 48 details how pass and block filters affect incoming messages for CAN based protocols. Figure 49 details how
 * pass and block filters affect incoming messages for all other protocols. As noted in Figure 48, an incoming message is
 * processed by both the logical communcation channels and the physical communication channel. This dual path for
 * incoming messages will allow applications to receive an ISO 15765 message (on a logical communication channel) as
 * well as the Raw CAN frames that comprise it (on the physical communication channel).
 *
 * Once the Pass-Thru Interface has handled the Network and Transport Layer details, as described in Section 7.2.3, the
 * messages are evaluated by the filters. This means that things like the Checksum (for ISO 9141 and ISO 14230 channels -
 * if <Flags> have CHECKSUM_DISABLED set to 0), etc. will not be part of the message when the filter evaluation takes
 * place.
 * 
 * The mask message designates which bits in the pattern message are to be evaluated. The filter matches when the
 * designated bits in the incoming message match the corresponding bits in the pattern message. Incoming messages
 * whose <DataLength> is less than that of the pattern message shall be considered not to match, even if the mask byte(s)
 * indicate ‘Don’t Care’. Any bytes in the incoming messages that extend beyond the end pattern message shall be treated
 * as ‘Don’t Care’ and ignored during the evaluation of the filter.
 *
 * For a pass filter, matching the pattern message shall cause the incoming message to proceed on for evaluation by the
 * block filters. Otherwise, the incoming message shall be discarded. If no block filters exists, then the incoming message
 * shall be added to the receive queue. For a block filter, matching the pattern message shall cause the incoming message
 * to be discarded. Otherwise, the incoming message shall be added to the receive queue.
 *
 * When populating the mask and pattern messages, the <TxFlags> of both must be identical. Only CAN channels shall be
 * permitted to set any <TxFlags> and then only the CAN_29BIT_ID bit is valid - no other bits in <TxFlags> are valid. In this
 * case, the state of the CAN_29BIT_ID bit in <TxFlags> for the filter is compared against the attributes of the incoming
 * messages. Therefore, to create a filter for 29-bit CAN IDs (on a CAN channel that allows the reception of 29-bit CAN IDs),
 * the CAN_29BIT_ID field in the <TxFlags> of both mask and pattern messages would need to be set. Similarly, a separate
 * filter would need to be defined to filter 11-bit CAN IDs. Figure 51 details the valid <TxFlags> for pass and block filters as
 * well as describing their use.
 *
 * For SAE J1850 PWM, it may be physically impossible to create the requested filter due to hardware limitations of the data
 * link controller. (For example, some data link controllers must be set in a special mode of operation that allows them to
 * ‘receive’ messages that do not match their current Node Address and/or are not in the Functional Message Lookup
 * Table.) In these cases, if the Mask and Pattern messages follow the format specified in Section 7.2.4 and the return value
 * shall be STATUS_NOERROR. This means that a pass filter may only provide a subset of all messages on the physical
 * vehicle bus. See Appendix A2.16 for a more detailed description.
 *
 * By default, PassThruConnect, PassThruDisconnect, PassThruOpen, and PassThruClose clear all of the filters for the
 * associated channel(s), which results in blocking all incoming messages. Therefore, an application must set the
 * appropriate pass or block filters to allow incoming messages to be placed in the receive queue.
 *
 * This function does not clear any messages that may have been received and queued before a filter was set. Application
 * developers are cautioned to consider performing a CLEAR_RX_QUEUE after starting a message filter to be sure that
 * unwanted messages are purged.
 *
 * If the corresponding Pass-Thru Device is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If
 * <ChannelID> is not valid or identifies a logical communication channel, the return value shall be
 * ERR_INVALID_CHANNEL_ID. If the Pass-Thru Device is not currently connected or was previously disconnected without
 * being closed, the return value shall be ERR_DEVICE_NOT_CONNECTED. If the <pMaskMsg>, <pPatternMsg>, or
 * <pFilterId> are NULL, the return value shall be ERR_NULL_PARAMETER. If the filter type is not permitted for the given
 * Channel ID or is in the reserved range, then the return value shall be ERR_FILTER_TYPE_NOT_SUPPORTED. If the
 * <ProtocolID> in the PASSTHRU_MSG structure (for any message passed into this function) does not match the
 * <ProtocolID> for the associated channel, the return value shall be ERR_MSG_PROTOCOL_ID. If any message passed
 * into this function does not follow the format specified in Section 7.2.4, the return value shall be ERR_INVALID_MSG. If
 * the application attempts to create more filters than are allowed, the return value shall be ERR_EXCEEDED_LIMIT. If this
 * function call is not supported for this channel, then the return value shall be ERR_NOT_SUPPORTED. The return value
 * ERR_FAILED is a mechanism to return other failures, but this is intended for use during development. A fully compliant
 * SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED or ERR_NOT_SUPPORTED.
 *
 * Parameters:
 *   <ChannelID>                       is an input, set by the application that contains the handle to the physical communication channel on
 *                                     which the filter is to be set. The Channel ID was assigned by a previous call to PassThruConnect. 
 *   <FilterType>                      is an input, set by the application that designates the type of filter that will be started. Valid types are:
 *
 *                                     PASS_FILTER allows matching messages to proceed on for evaluation by the block filter(s), on
 *                                                 the way to the receive queue. (A block filter can still discard messages that match a
 *                                                 pass filter.) This filter type is valid for all physical communication channels.
 *                                     BLOCK_FILTER keeps matching messages out of the receive queue. This filter type is valid for all
 *                                                  physical communication channels.
 *   <pMaskMsg>                        is an input, set by the application that designates a pointer to the ‘mask message’. The <DataBuffer>,
 *                                     <DataLength>, and <TxFlags> elements in this PASSTHRU_MSG structure are used to determine the
 *                                     mask and mask size. Bits in the <DataBuffer> element that are set to ‘0’ are unimportant (or ‘Don’t
 *                                     Care’), while bits that are ‘1’ are important (or ‘Care’). The <RxStatus> field shall always be ignored.
 *                                     The <DataLength> and <TxFlags> in this PASSTHRU_MSG structure shall be identical to that from
 *                                     the pattern message. The structure shall follow the format specified in Section 7.2.4, including the
 *                                     actions to be taken when messages violate the specified format. The Pass-Thru Interface shall not
 *                                     modify this structure.
 *   <pPatternMsg>                     is an input, set by the application that designates a pointer to the ‘pattern message’. The
 *                                     <DataBuffer>, <DataLength>, and <TxFlags> elements in this PASSTHRU_MSG structure are used to
 *                                     determine the pattern and pattern size. The <RxStatus> element shall always be ignored. Otherwise,
 *                                     the structure shall follow the format specified in Section 7.2.4, including the actions to be taken when
 *                                     messages violate the specified format. The Pass-Thru Interface shall not modify this structure.
 *   <pFilterID>                       is an input, set by the application, which points to an unsigned long allocated by the application. Upon
 *                                     return, the unsigned long shall contain the Filter ID, which is to be used as a handle to the filter for
 *                                     future function calls.
 * Return Values:
 *   ERR_CONCURRENT_API_CALL           A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN               PassThruOpen has not successfully been called
 *   ERR_INVALID_CHANNEL_ID            Invalid <ChannelID> value
 *   ERR_DEVICE_NOT_CONNECTED          Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has, at some point, 
 *                                     failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *   ERR_NOT_SUPPORTED                 Device does not support this API function for the associated <ChannelID>. A fully compliant SAE J2534-1
 *                                     Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_NULL_PARAMETER                NULL pointer supplied where a valid pointer is required
 *   ERR_FILTER_TYPE_NOT_SUPPORTED     <FilterType> is either not permitted for the current channel or is in the reserved range
 *   ERR_MSG_PROTOCOL_ID               <ProtocolID> in the PASSTHRU_MSG structure does not match the <ProtocolID> from the original call to
 *                                     PassThruConnect for the <ChannelID>
 *   ERR_INVALID_MSG                   Message structure is invalid for the given Channel ID (refer to Section 7.2.4 for more details)
 *   ERR_EXCEEDED_LIMIT                Exceeded the maximum number of filter message IDs
 *   ERR_FAILED                        Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru
 *                                     Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR                    Function call was successful
 */
long PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, PASSTHRU_MSG *pMaskMsg, PASSTHRU_MSG *pPatternMsg, unsigned long *pFilterID) {

  j2534_current_api_call = J2534_PassThruStartMsgFilter;

  if(pMaskMsg == NULL || pPatternMsg == NULL || pFilterID == NULL) {
    return unless_concurrent_call(ERR_NULL_PARAMETER, J2534_PassThruStartMsgFilter);
  }

  if(!j2534_opened) {
    return unless_concurrent_call(ERR_DEVICE_NOT_OPEN, J2534_PassThruStartMsgFilter);
  }

  j2534_client *client = j2534_client_by_channel_id(ChannelID);
  if(client == NULL) {
    return unless_concurrent_call(ERR_INVALID_DEVICE_ID, J2534_PassThruStartMsgFilter);
  }

  if(FilterType != PASS_FILTER && FilterType != BLOCK_FILTER) {
    return unless_concurrent_call(ERR_FILTER_TYPE_NOT_SUPPORTED, J2534_PassThruStartMsgFilter);
  }

  if(pMaskMsg->ProtocolID != client->protocolId || pPatternMsg->ProtocolID != client->protocolId) {
    return unless_concurrent_call(ERR_MSG_PROTOCOL_ID, J2534_PassThruStartMsgFilter);
  }

  if(pMaskMsg->DataLength < 1 || pMaskMsg->DataLength > 12) {
    return unless_concurrent_call(ERR_INVALID_MSG, J2534_PassThruStartMsgFilter);
  }

  if(pPatternMsg->DataLength < 1 || pPatternMsg->DataLength > 12) {
    return unless_concurrent_call(ERR_INVALID_MSG, J2534_PassThruStartMsgFilter);
  }

  if(client->filters->size >= 10) {
    return unless_concurrent_call(ERR_EXCEEDED_LIMIT, J2534_PassThruStartMsgFilter); 
  }

  j2534_canfilter *filter = malloc(sizeof(j2534_canfilter));
  filter->id = pFilterID;
  filter->can_id = pPatternMsg->DataBuffer;
  filter->can_mask = pMaskMsg->DataBuffer;
  vector_add(client->filters, filter);

  return unless_concurrent_call(
    j2534_publish_state(client, J2534_PassThruStartMsgFilter),
    J2534_PassThruStartMsgFilter
  );
}

/**
 * 7.3.15 PassThruStopMsgFilter
 * 
 * This function shall delete the specified filter from the message evaluation process for the designated channel. If the
 * function call is successful, the return value shall be STATUS_NOERROR, the <FilterID> shall be invalidated, and the
 * associated filter shall no longer be used to evaluate incoming messages.
 *
 * If the corresponding Pass-Thru Device is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If
 * <ChannelID> is not is not valid or identifies a logical communication channel, the return value shall be
 * ERR_INVALID_CHANNEL_ID. If the Pass-Thru Device is not currently connected or was previously disconnected without
 * being closed, the return value shall be ERR_DEVICE_NOT_CONNECTED. If <FilterID> is not is not valid, the return
 * value shall be ERR_INVALID_FILTER_ID. If this function call is not supported for this channel, then the return value shall
 * be ERR_NOT_SUPPORTED. The return value ERR_FAILED is a mechanism to return other failures, but this is intended
 * for use during development. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED or
 * ERR_NOT_SUPPORTED.
 *
 * Parameters:
 *   <ChannelID>                       is an input, set by the application that contains the handle to the physical communication channel on which
 *                                     the filter is to be stopped. The Channel ID was assigned by a previous call to PassThruConnect. 
 *   <FilterID>                        is an input, set by the application, which contains the Filter ID that is to be stopped. The Filter ID was
 *                                     assigned by a previous call to PassThruStartMsgFilter. 
 * Return Values:
 *   ERR_CONCURRENT_API_CALL           A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN               PassThruOpen has not successfully been called
 *   ERR_INVALID_CHANNEL_ID            Invalid <ChannelID> value
 *   ERR_DEVICE_NOT_CONNECTED          Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has, at some point,
 *                                     failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *   ERR_NOT_SUPPORTED                 Device does not support this API function for the associated <ChannelID>. A fully compliant SAE J2534-1
 *                                     Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_INVALID_FILTER_ID             Invalid <FilterID> value
 *   ERR_FAILED                        Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru
 *                                     Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR                    Function call was successful
 */
long  PassThruStopMsgFilter(unsigned long ChannelID, unsigned long FilterID) {
  return ERR_NOT_SUPPORTED;
}

/**
 * 7.3.16 PassThruSetProgrammingVoltage
 *
 * This function shall set either a programming voltage on a single pin or short a single pin to ground on the designated
 * Pass-Thru Device. If the function call is successful, the return value shall be STATUS_NOERROR and the associated pin
 * shall be at the specified level. The default state for all pins shall be PIN_OFF. This function does not require a Channel ID,
 * so it may be used after a call to PassThruOpen for the corresponding Pass-Thru Device.
 *
 * The Pass-Thru Interface contains exactly one programmable voltage generator and one programmable ground, which
 * operate independently but cannot exist on the same pin (see Section 6.7 for more details). At any given time, the voltage
 * can be routed to at-maximum one pin and the ground can be routed to at-maximum one pin. After applying a voltage or
 * ground to a specific pin, that pin must be set to PIN_OFF before a new pin can be used. The pin must also be set to
 * PIN_OFF when switching between voltage and ground on the same pin. However, there is no need to set a pin to
 * PIN_OFF when selecting a different voltage on the same pin.
 * 
 * The application programmer shall ensure that voltages are not applied to any pins that may cause damage to a vehicle or
 * ECU. PassThruSetProgrammingVoltage does NOT protect from usage that might result in damage to a vehicle or ECU.
 * 
 * SAE J2610 channels are unique, as they require the programmable voltage generator to be on the Rx pin. For SAE J2610
 * channels ONLY, the programming voltage may be manually set with this function (for an immediate affect) on the Rx pin
 * without getting the return value ERR_PIN_IN_USE. Otherwise, the programmable voltage generator shall be
 * automatically controlled with the SCI_TX_VOLTAGE bit in <TxFlags> of the PASSTHRU_MSG structure during a call to
 * PassThruQueueMsgs. However, if the programmable voltage generator is in use at that time, the call to
 * PassThruQueueMsgs shall fail (see PassThruQueueMsgs for more details).
 *
 * Attempting to set an unused pin to PIN_OFF shall result in the return value STATUS_NOERROR. Attempting to set a pin
 * that is already in use for a different purpose (except as noted above) shall result in the return value ERR_PIN_IN_USE.
 * Attempting to set a voltage on a pin other than 0, 6, 9, 11, 12, 13, 14, or 15 of the J1962_CONNECTOR shall result in the
 * return value ERR_PIN_NOT_SUPPORTED. Attempting to set a ground on a pin other than 15 of the
 * J1962_CONNECTOR shall result in the return value ERR_PIN_NOT_SUPPORTED. Attempting to set
 * <ResourceStruct.NumOfResources> to a value other 1 shall result in the return value ERR_PIN_NOT_SUPPORTED.
 * Attempting to set a voltage on a valid pin when the voltage is currently being applied to another pin shall result in the
 * return value ERR_VOLTAGE_IN_USE. Attempting to set a voltage that is out of range on a valid pin shall result in the
 * return value ERR_EXCEEDED_LIMIT. If <ResourceStruct.ResourceListPtr> is NULL, the return value shall be
 * ERR_NULL_PARAMETER.
 *
 * If the corresponding Pass-Thru Device is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If
 * <DeviceID> is not valid, the return value shall be ERR_INVALID_DEVICE_ID. If the Pass-Thru Device is not
 * currently connected or was previously disconnected without being closed, the return value shall be
 * ERR_DEVICE_NOT_CONNECTED. If this function call is not supported for this device, then the return value shall be
 * ERR_NOT_SUPPORTED. The return value ERR_FAILED is a mechanism to return other failures, but this is intended for
 * use during development. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED or
 * ERR_NOT_SUPPORTED.
 *
 * Parameters:
 *   <DeviceID>                          is an input, set by the application that contains the handle to the Pass-Thru Device whose programmable
 *                                       voltage generator / programmable ground will be set. The Device ID was assigned by a previous call to
 *                                       PassThruOpen.
 *   <ResourceStruct>                    is an input, set by the application, that identifies the connector and pin on which the programmable
 *                                       voltage generator / programmable ground will be set. This structure is defined in Section 9.18. Valid
 *                                       values for each of the structure elements are:
 *                                         <ResourceStruct.Connector> shall be set to J1962_CONNECTOR
 *                                         <ResourceStruct.NumOfResources> shall be set to 1
 *                                         <ResourceStruct.ResourceListPtr> shall point to an unsigned long that contains one of these values:
 *                                           0 – Auxiliary output pin (the banana jack identified in Section 6.7)
 *                                           6 – Pin 6 on the SAE J1962 connector.
 *                                           9 – Pin 9 on the SAE J1962 connector.
 *                                           11 – Pin 11 on the SAE J1962 connector.
 *                                           12 – Pin 12 on the SAE J1962 connector.
 *                                           13 – Pin 13 on the SAE J1962 connector.
 *                                           14 – Pin 14 on the SAE J1962 connector.
 *                                           15 – Pin 15 on the SAE J1962 connector (short to ground only).
 * Return Values:
 *   ERR_CONCURRENT_API_CALL             A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN                 PassThruOpen has not successfully been called
 *   ERR_INVALID_DEVICE_ID               Invalid <DeviceID> value
 *   ERR_DEVICE_NOT_CONNECTED            Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has, at some point,
 *                                       failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *   ERR_NOT_SUPPORTED                   Device does not support this API function for the associated <DeviceID>. A fully compliant SAE J2534-1
 *                                       Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_PIN_NOT_SUPPORTED               One or more of the following is either invalid or unknown: The pin number specified by <ResourceStruct.ResourceListPtr>,
 *                                       the connector specified by <ResourceStruct.Connector>, or the number of resources.
 *   ERR_VOLTAGE_IN_USE                  Programming voltage is currently being applied to another pin
 *   ERR_PIN_IN_USE                      <PinNumber> specified is currently in use (either for voltage, ground, or by another channel)
 *   ERR_EXCEEDED_LIMIT                  Exceeded the allowed limits for programmable voltage generator
 *   ERR_NULL_PARAMETER                  NULL pointer supplied where a valid pointer is required
 *   ERR_FAILED                          Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru
 *                                       Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR                      Function call was successful
 */
 long  PassThruSetProgrammingVoltage(unsigned long DeviceID, RESOURCE_STRUCT ResourceStruct, unsigned long Voltage) {
  return ERR_NOT_SUPPORTED;
 }

 /**
  * 7.3.17 PassThruReadVersion
  *
  * This function shall return the Firmware, DLL, and API version information for the designated Pass-Thru Device. If the
  * function call is successful, the return value shall be STATUS_NOERROR and the associated version buffers shall each
  * contain a valid version string. This function does not require a Channel ID, so it may be used after a call to
  * PassThruOpen for the corresponding Pass-Thru Device.
  *
  * In cases where the return value is neither STATUS_NOERROR nor ERR_NULL_PARAMETER, the function shall return
  * the applicable return value and a valid DLL version string. If available, other valid version strings may also be returned. If
  * a version string is not available, the first byte shall be set to the NULL terminator by the Pass-Thru Interface.
  *
  * If the corresponding Pass-Thru Device is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If
  * <DeviceID> is not valid, the return value shall be ERR_INVALID_DEVICE_ID. If the Pass-Thru Device is not
  * currently connected or was previously disconnected without being closed, the return value shall be
  * ERR_DEVICE_NOT_CONNECTED. If <pFirmwareVersion>, <pDllVersion>, or <pApiVersion> are NULL, the return value
  * shall be ERR_NULL_PARAMETER. The return value ERR_FAILED is a mechanism to return other failures, but this is
  * intended for use during development. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return
  * ERR_FAILED.
  *
  * Parameters:
  *   <DeviceID>                          is an input, set by the application that contains the handle to the Pass-Thru Device whose version will be
  *                                       read. The Device ID was assigned by a previous call to PassThruOpen
  *   <pFirmwareVersion>                  is an input, allocated by the application that points to a buffer where the Firmware version will be
  *                                       placed. The version buffer is allocated by the application and must be eighty (80) characters, where
  *                                       one character shall consume one byte. Upon return, the array shall contain an ASCII character
  *                                       string that represents the version string. The Pass-Thru Interface vendor that supplies the device
  *                                       shall determine the version string. A valid version string shall contain 80 ASCII characters or less,
  *                                       including the NULL terminator ($00), and the first byte shall NOT be the NULL terminator (unless an
  *                                       error occurred).
  *   <pDllVersion>                       is an input, allocated by the application that points to a buffer where the DLL version will be placed. The
  *                                       version buffer is allocated by the application and must be eighty (80) characters, where one character
  *                                       shall consume one byte. Upon return, the array shall contain an ASCII character string that represents the
  *                                       version string. The Pass-Thru Interface vendor that supplies the DLL shall determine the version string. A
  *                                       valid version string shall contain 80 ASCII characters or less, including the NULL terminator ($00), and
  *                                       the first byte shall NOT be the NULL terminator.
  *   <pApiVersion>                       is an input, allocated by the application that points to a buffer where the API version will be placed. The
  *                                       version buffer is allocated by the application and must be eighty (80) characters, where one character
  *                                       shall consume one byte. Upon return, the array shall contain an ASCII character string that represents the
  *                                       version string. The API version string shall be of the “XX.YY” format (where XX is the major version
  *                                       number and YY is the minor version number). All API version strings shall contain 80 ASCII characters or
  *                                       less, including the NULL terminator ($00), and the first byte shall NOT be the NULL terminator (unless an
  *                                       error occurred). Valid API version strings are defined below:
  *                                         February 2002 publication = “02.02”
  *                                         December 2004 publication = “04.04”
  *                                         This version = “05.00”
  * 
  * Return Values:
  *   ERR_CONCURRENT_API_CALL             A J2534 API function has been called before the previous J2534 function call has completed
  *   ERR_DEVICE_NOT_OPEN                 PassThruOpen has not successfully been called
  *   ERR_INVALID_DEVICE_ID               Invalid <DeviceID> value
  *   ERR_DEVICE_NOT_CONNECTED            Pass-Thru Device communication error. This indicates that the Pass-Thru Interface DLL has, at some point,
  *                                       failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
  *   ERR_NULL_PARAMETER                  NULL pointer supplied where a valid pointer is required
  *   ERR_FAILED                          Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru
  *                                       Interface shall never return ERR_FAILED.
  *   STATUS_NOERROR                      Function call was successful
  */
long PassThruReadVersion(unsigned long DeviceID, char *pFirmwareVersion, char *pDllVersion, char *pApiVersion) {
  int api_call = 7317;
  j2534_current_api_call = api_call;
  strcpy(pFirmwareVersion, PASSTHRU_FIRMWARE_VERSION);
  strcpy(pDllVersion, J2534_DLL_VERSION);
  strcpy(pApiVersion, J2534_API_VERSION);
  return unless_concurrent_call(STATUS_NOERROR, api_call);
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
 * 7.3.19 PassThruIoctl
 * 
 * This function shall be used to configure and control various features of the designated channel or device. If the function
 * call is successful, the return value shall be STATUS_NOERROR and the requested action shall have been completed. 
 *
 * If the corresponding Pass-Thru Device is not currently open, the return value shall be ERR_DEVICE_NOT_OPEN. If the
 * <IoctlID> is unknown, not applicable for the <ControlTarget> (for example, trying to initiate a Five-Baud Initialization on a
 * CAN channel), or required by this document but not supported by the Pass-Thru Interface, the return value shall be
 * ERR_IOCTL_ID_NOT_SUPPORTED. If the <ControlTarget> is required to be the Device ID and that value is not valid,
 * the return value shall be ERR_INVALID_DEVICE_ID. If the <ControlTarget> is required to be the Channel ID and that
 * value is not valid, the return value shall be ERR_INVALID_CHANNEL_ID. If the Pass-Thru Device is not currently
 * connected or was previously disconnected without being closed, the return value shall be
 * ERR_DEVICE_NOT_CONNECTED. If either <InputPtr> or <OutputPtr> are NULL when they are required to be nonNULL,
 * the return value shall be ERR_NULL_PARAMETER. If either <InputPtr> or <OutputPtr> are non-NULL when they
 * are required to be NULL, the return value shall be ERR_NULL_REQUIRED. If the <ProtocolID> in the PASSTHRU_MSG
 * structure (for any message passed into this function) does not match the <ProtocolID> for the associated channel, the
 * return value shall be ERR_MSG_PROTOCOL_ID. If any message passed into this function does not follow the format
 * specified in Section 7.2.4, the return value shall be ERR_INVALID_MSG. If the configuration Parameter ID (for the IOCTL
 * ID values GET_CONFIG and SET_CONFIG) is unknown or not applicable, the return value shall be ERR_IOCTL_PARAM_ID_NOT_SUPPORTED.
 * If the configuration <Value> (for the IOCTL ID values GET_CONFIG and SET_CONFIG) is unknown, invalid, out of range, or not applicable,
 * the return value shall be ERR_IOCTL_VALUE_NOT_SUPPORTED. If the result of this function call will cause the Pass-Thru Interface to exceed a
 * prescribed limit, the return value shall be ERR_EXCEEDED_LIMIT. If an initialization is requested and it is unsuccessful,
 * the return value shall be ERR_INIT_FAILED. If a buffer overflow occurred during this function call, the return value shall
 * be ERR_BUFFER_OVERFLOW. If a message being returned in the structure PASSTHRU_MSG is larger than the size
 * indicated by <DataBufferSize>, then only the number of bytes indicated by <DataBufferSize> shall be placed in the array
 * <DataBuffer> and the return value shall be ERR_BUFFER_TOO_SMALL. If this function call is not supported for this
 * device, then the return value shall be ERR_NOT_SUPPORTED. The return value ERR_FAILED is a mechanism to return
 * other failures, but this is intended for use during development. A fully compliant SAE J2534-1 Pass-Thru Interface shall
 * never return ERR_FAILED or ERR_NOT_SUPPORTED.
 *
 * Parameters:
 *   <ControlTarget>                  is an input, set by the application that contains the handle of channel/device to be controlled. For
 *                                    IOCTL ID values that act on channels, this will be the corresponding Channel ID assigned by the
 *                                    PassThruConnect or PassThruLogicalConnect functions. For IOCTL ID values that act on devices,
 *                                    this will be the corresponding the Device ID assigned by the PassThruOpen function. (See the
 *                                    specific IOCTL Section to determine the content of the <ConrolTarget>.)
 *   <IoctlID>                        is an input, set by the application that contains the desired IOCTL ID value (see Figure 60 for a list of valid values).
 *   <InputPtr>                       is an input, set by the application that points to the location of the input. The structure of the input is
 *                                    specific to each IOCTL ID value and shall be allocated and initialized by the calling application (see the
 *                                    specific IOCTL Section for more details).
 *   <OutputPtr>                      is an input, set by the application that points to the location of the output. The structure of the output is
 *                                    specific to each IOCTL ID value and shall be allocated by the calling application (see the specific
 *                                    IOCTL Section for more details). Upon successful completion, the output field shall be updated as
 *                                    indicated.
 * 
 * Return Values:
 *   ERR_CONCURRENT_API_CALL          A J2534 API function has been called before the previous J2534 function call has completed
 *   ERR_DEVICE_NOT_OPEN              PassThruOpen has not successfully been called
 *   ERR_IOCTL_ID_NOT_SUPPORTED       <IoctlID> value is not supported (either invalid, unknown, or not applicable for the current channel)
 *   ERR_INVALID_DEVICE_ID            PassThruOpen has been successfully called, but the current Device ID is not valid
 *   ERR_INVALID_CHANNEL_ID           Invalid Channel ID value
 *   ERR_DEVICE_NOT_CONNECTED         Pass-Thru Device communication error. This indicates that the PassThru Interface DLL has, at some point,
 *                                    failed to communicate with the Pass-Thru Device – even though it may not currently be disconnected.
 *   ERR_NOT_SUPPORTED                Device does not support this API function for the associated Channel ID. A fully compliant SAE J2534-1
 *                                    Pass-Thru Interface shall never return ERR_NOT_SUPPORTED.
 *   ERR_NULL_PARAMETER               NULL pointer supplied where a valid pointer is required
 *   ERR_NULL_REQUIRED                A parameter that is required to be NULL is not set to NULL
 *   ERR_MSG_PROTOCOL_ID              <ProtocolID> in the PASSTHRU_MSG structure does not match the <ProtocolID> from the original call to PassThruConnect for the Channel ID
 *   ERR_INVALID_MSG                  Message structure is invalid for the given Channel ID (refer to Section 7.2.4 for more details)
 *   ERR_IOCTL_PARAM_ID_NOT_SUPPORTED Parameter referenced in the SCONFIG_LIST structure is not supported (either invalid, unknown, or not applicable for the current channel)
 *   ERR_IOCTL_VALUE_NOT_SUPPORTED    Value referenced in the SCONFIG_LIST structure is not supported (either unknown, invalid, out of range, or not applicable for the current channel)
 *   ERR_PIN_NOT_SUPPORTED            One or more of the following is either invalid or unknown: The pin number specified by <InputPtr->ResourceListPtr>, the connector specified
 *                                    by <InputPtr->Connector>, or the number of resources specified by <InputPtr->NumOfResources>
 *   ERR_EXCEEDED_LIMIT               Exceeded the allowed limits
 *   ERR_INIT_FAILED                  Physical vehicle bus initialization failed
 *   ERR_BUFFER_OVERFLOW              Indicates a buffer overflow occurred and messages were lost
 *   ERR_BUFFER_TOO_SMALL             The size of <DataBuffer>, as indicated by the parameter <DataBufferSize> in the PASSTHRU_MSG structure, is too small to accommodate the full message
 *   ERR_FAILED                       Undefined error, use PassThruGetLastError for text description. A fully compliant SAE J2534-1 Pass-Thru Interface shall never return ERR_FAILED.
 *   STATUS_NOERROR                   Function call was successful
 */
long PassThruIoctl(unsigned long ControlTarget, unsigned long IoctlID, void *InputPtr, void *OutputPtr) {
  return ERR_NOT_SUPPORTED;
}
