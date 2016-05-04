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

#include "awsiot_client.h"

void awsiot_client_connect(awsiot_client *awsiot) {

  char errmsg[255];
  char rootCA[255];
  char clientCRT[255];
  char clientKey[255];
  char CurrentWD[255];
  char certDirectory[10] = "certs";
  char cafileName[] = AWS_IOT_ROOT_CA_FILENAME;
  char clientCRTName[] = AWS_IOT_CERTIFICATE_FILENAME;
  char clientKeyName[] = AWS_IOT_PRIVATE_KEY_FILENAME;
  awsiot->rc = NONE_ERROR;

  getcwd(CurrentWD, sizeof(CurrentWD));
  sprintf(rootCA, "%s/%s/%s", CurrentWD, certDirectory, cafileName);
  sprintf(clientCRT, "%s/%s/%s", CurrentWD, certDirectory, clientCRTName);
  sprintf(clientKey, "%s/%s/%s", CurrentWD, certDirectory, clientKeyName);

  syslog(LOG_DEBUG, "rootCA %s", rootCA);
  syslog(LOG_DEBUG, "clientCRT %s", clientCRT);
  syslog(LOG_DEBUG, "clientKey %s", clientKey);

  MQTTConnectParams connectParams = MQTTConnectParamsDefault;
  connectParams.KeepAliveInterval_sec = 10;
  connectParams.isCleansession = true;
  connectParams.MQTTVersion = MQTT_3_1_1;
  connectParams.pClientID = "ecutools-datalogger";
  connectParams.pHostURL = AWS_IOT_MQTT_HOST;
  connectParams.port = AWS_IOT_MQTT_PORT;
  connectParams.isWillMsgPresent = false;
  connectParams.pRootCALocation = rootCA;
  connectParams.pDeviceCertLocation = clientCRT;
  connectParams.pDevicePrivateKeyLocation = clientKey;
  connectParams.mqttCommandTimeout_ms = 2000;
  connectParams.tlsHandshakeTimeout_ms = 5000;
  connectParams.isSSLHostnameVerify = true;
  connectParams.disconnectHandler = awsiot->ondisconnect;

  awsiot->rc = aws_iot_mqtt_connect(&connectParams);
  if(awsiot->rc != NONE_ERROR) {
    sprintf(errmsg, "Error(%d) connecting to %s:%d", awsiot->rc, connectParams.pHostURL, connectParams.port);
    awsiot->onerror(awsiot, errmsg);
    return NULL;
  }

  awsiot->rc = aws_iot_mqtt_autoreconnect_set_status(true);
  if(NONE_ERROR != awsiot->rc) {
    sprintf(errmsg, "Unable to set Auto Reconnect to true. IoT_Error_t=%d", awsiot->rc);
    awsiot->onerror(awsiot, errmsg);
    return NULL;
  }

  awsiot->onopen(awsiot);
}

bool awsiot_client_isconnected() {
  return aws_iot_is_mqtt_connected();
}

void awsiot_client_subscribe(awsiot_client *awsiot, const char *topic) {

  syslog(LOG_DEBUG, "awsiot_client_subscribe: subscribing to topic %s.", topic);

  MQTTSubscribeParams subParams = MQTTSubscribeParamsDefault;
  subParams.mHandler = awsiot->onmessage;
  subParams.pTopic = topic;
  subParams.qos = QOS_0;

  if(NONE_ERROR == awsiot->rc) {
    awsiot->rc = aws_iot_mqtt_subscribe(&subParams);
    if (NONE_ERROR != awsiot->rc) {
      char errmsg[255];
      sprintf(errmsg, "awsiot_client_subscribe: error subscribing to topic %s. IoT_Error_t: %d", subParams.pTopic, awsiot->rc);
      awsiot->onerror(awsiot, errmsg);
      return NULL;
    }
  }
}

void awsiot_client_publish(awsiot_client *awsiot, const char *topic, const char *payload) {

  int payload_len = strlen(payload) + 1;
  syslog(LOG_DEBUG, "awsiot_client_publish: topic=%s, payload_len=%d, payload=%s", topic, payload_len, payload);

  MQTTMessageParams Msg = MQTTMessageParamsDefault;
  Msg.qos = QOS_0;
  Msg.PayloadLen = payload_len;
  Msg.pPayload = (void *) payload;

  MQTTPublishParams Params = MQTTPublishParamsDefault;
  Params.pTopic = topic;
  Params.MessageParams = Msg;

  if(NONE_ERROR == awsiot->rc) {
    awsiot->rc = aws_iot_mqtt_publish(&Params);
    if (NONE_ERROR != awsiot->rc) {
      char errmsg[255];
      sprintf(errmsg, "awsiot_client_publish: error publishing to topic %s. IoT_Error_t: %d", Params.pTopic, awsiot->rc);
      awsiot->onerror(awsiot, errmsg);
      return NULL;
    }
  }

  aws_iot_mqtt_publish(&Params);
}

void awsiot_client_close(awsiot_client *awsiot, const char *topic, const char *payload) {
  if(payload != NULL) {
    awsiot_client_publish(awsiot, topic, payload);
  }
  awsiot->rc = aws_iot_mqtt_disconnect();
  awsiot->onclose(awsiot, payload);
}
