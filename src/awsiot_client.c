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

#include "awsiot_client.h"

unsigned int awsiot_client_connect(awsiot_client *awsiot) {

  char errmsg[255];
  char rootCA[255];
  char clientCRT[255];
  char clientKey[255];
  awsiot->rc = SUCCESS;

  sprintf(rootCA, "%s/%s", awsiot->certDir, AWS_IOT_ROOT_CA_FILENAME);
  sprintf(clientCRT, "%s/%s", awsiot->certDir, AWS_IOT_CERTIFICATE_FILENAME);
  sprintf(clientKey, "%s/%s", awsiot->certDir, AWS_IOT_PRIVATE_KEY_FILENAME); 

  syslog(LOG_DEBUG, "rootCA %s", rootCA);
  syslog(LOG_DEBUG, "clientCRT %s", clientCRT);
  syslog(LOG_DEBUG, "clientKey %s", clientKey);

  IoT_Client_Init_Params mqttInitParams;
  mqttInitParams.enableAutoReconnect = false; // We enable this later below
  mqttInitParams.pHostURL =  AWS_IOT_MQTT_HOST;
  mqttInitParams.port = AWS_IOT_MQTT_PORT;
  mqttInitParams.pRootCALocation = rootCA;
  mqttInitParams.pDeviceCertLocation = clientCRT;
  mqttInitParams.pDevicePrivateKeyLocation = clientKey;
  mqttInitParams.mqttCommandTimeout_ms = 7000;
  mqttInitParams.tlsHandshakeTimeout_ms = 5000;
  mqttInitParams.isSSLHostnameVerify = true;
  mqttInitParams.disconnectHandler = awsiot->ondisconnect;
  mqttInitParams.disconnectHandlerData = awsiot->client;

  awsiot->rc = aws_iot_mqtt_init(awsiot->client, &mqttInitParams);
  if(awsiot->rc != SUCCESS) {
    syslog(LOG_ERR, "awsiot_client_connect: aws_iot_mqtt_init error=%d", awsiot->rc);
    return 1;
  }

  IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

  connectParams.keepAliveIntervalInSec = 10;
  connectParams.isCleanSession = true;
  connectParams.MQTTVersion = MQTT_3_1_1;
  connectParams.pClientID = awsiot->clientId;
  connectParams.isWillMsgPresent = false;

  awsiot->rc = aws_iot_mqtt_connect(awsiot->client, &connectParams);
  if(awsiot->rc != SUCCESS) {
    sprintf(errmsg, "awsiot_client_connect: Error(%d) connecting to %s:%d", awsiot->rc, mqttInitParams.pHostURL, mqttInitParams.port);
    if(awsiot->onerror) awsiot->onerror(awsiot, errmsg);
    return 2;
  }

  awsiot->rc = aws_iot_mqtt_autoreconnect_set_status(awsiot->client, true);
  if(SUCCESS != awsiot->rc) {
    sprintf(errmsg, "awsiot_client_connect: Unable to set autoreconnect to true. error=%d", awsiot->rc);
    if(awsiot->onerror) awsiot->onerror(awsiot, errmsg);
    return 3;
  }

  if(awsiot->onopen != NULL) {
    awsiot->onopen(awsiot);
  }

  syslog(LOG_DEBUG, "awsiot_client_connect: connected to MQTT server %s:%d", AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT);
  return 0;
}

bool awsiot_client_isconnected(awsiot_client *awsiot) {
  return (awsiot->rc == NETWORK_RECONNECTED || awsiot->rc == SUCCESS);
}

unsigned int awsiot_client_subscribe(awsiot_client *awsiot, const char *topic, void *pApplicationHandler, void *pApplicationHandlerData) {
  syslog(LOG_DEBUG, "awsiot_client_subscribe: topic=%s.", topic);
  void* callback = (pApplicationHandler == NULL) ? awsiot->onmessage : pApplicationHandler;
  awsiot->rc = aws_iot_mqtt_subscribe(awsiot->client, topic, strlen(topic), QOS0, callback, pApplicationHandlerData);
  if(SUCCESS != awsiot->rc) {
    char errmsg[255];
    sprintf(errmsg, "awsiot_client_subscribe: error subscribing to topic %s. IoT_Error_t: %d", topic, awsiot->rc);
    if(awsiot->onerror) awsiot->onerror(awsiot, errmsg);
    return 1;
  }
  return 0;
}

unsigned int awsiot_client_unsubscribe(awsiot_client *awsiot, const char *topic) {
  syslog(LOG_DEBUG, "awsiot_client_unsubscribe: topic=%s", topic);
  awsiot->rc = aws_iot_mqtt_unsubscribe(awsiot->client, topic, strlen(topic));
  if(SUCCESS != awsiot->rc) {
    char errmsg[255];
    sprintf(errmsg, "awsiot_client_unsubscribe: error unsubscribing from topic %s. IoT_Error_t: %d", topic, awsiot->rc);
    if(awsiot->onerror) awsiot->onerror(awsiot, errmsg);
    return 1;
  }
  return 0;
}

unsigned int awsiot_client_publish(awsiot_client *awsiot, const char *topic, char *payload) {

  int payload_len = strlen(payload);
  syslog(LOG_DEBUG, "awsiot_client_publish: topic=%s, payload_len=%d, payload=%s", topic, payload_len, payload);

  IoT_Publish_Message_Params paramsQOS0;
  paramsQOS0.qos = QOS0;
  paramsQOS0.payloadLen = payload_len;
  paramsQOS0.payload = (void *) payload;
  paramsQOS0.isRetained = 0;

  awsiot->rc = aws_iot_mqtt_publish(awsiot->client, topic, strlen(topic), &paramsQOS0);
  if(SUCCESS != awsiot->rc) {
    char errmsg[255];
    sprintf(errmsg, "awsiot_client_publish: error publishing to topic %s. IoT_Error_t: %d", topic, awsiot->rc);
    if(awsiot->onerror) awsiot->onerror(awsiot, errmsg);
    return 1;
  }
  return 0;
}

void awsiot_client_close(awsiot_client *awsiot) {
  syslog(LOG_DEBUG, "awsiot_client_close: closing connection");
  awsiot->rc = aws_iot_mqtt_disconnect(&awsiot->client);
  if(awsiot->onclose) awsiot->onclose(awsiot);
}
