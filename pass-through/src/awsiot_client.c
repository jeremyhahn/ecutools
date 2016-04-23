#include "awsiot_client.h"

IoT_Error_t awsiot_client_connect(awsiot_client *awsiot) {

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
  connectParams.pClientID = "ecutools-data-logger";
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
    syslog(LOG_ERR, "Error(%d) connecting to %s:%d", awsiot->rc, connectParams.pHostURL, connectParams.port);
  }

  awsiot->rc = aws_iot_mqtt_autoreconnect_set_status(true);
  if(NONE_ERROR != awsiot->rc) {
    syslog(LOG_ERR, "Unable to set Auto Reconnect to true. IoT_Error_t=%d", awsiot->rc);
  }

  awsiot->onopen(awsiot);
  return awsiot->rc;
}

bool awsiot_client_isconnected() {
  return aws_iot_is_mqtt_connected();
}

IoT_Error_t awsiot_client_subscribe(awsiot_client *awsiot) {

  syslog(LOG_DEBUG, "awsiot_client_subscribe");

  MQTTSubscribeParams subParams = MQTTSubscribeParamsDefault;
  subParams.mHandler = awsiot->onmessage;
  subParams.pTopic = "ecutools/datalogger";
  subParams.qos = QOS_0;

  if(NONE_ERROR == awsiot->rc) {
    awsiot->rc = aws_iot_mqtt_subscribe(&subParams);
    if (NONE_ERROR != awsiot->rc) {
      syslog(LOG_ERR, "Error subscribing to %s queue: %d", subParams.pTopic, awsiot->rc);
    }
  }

  return awsiot->rc;
}

IoT_Error_t awsiot_client_publish(awsiot_client *awsiot, const char *payload) {

  int payload_len = strlen(payload) + 1;

  MQTTMessageParams Msg = MQTTMessageParamsDefault;
  Msg.qos = QOS_0;
  Msg.PayloadLen = payload_len;
  Msg.pPayload = (void *) payload;

  MQTTPublishParams Params = MQTTPublishParamsDefault;
  Params.pTopic = "ecutools/datalogger";
  Params.MessageParams = Msg;

  if(NONE_ERROR == awsiot->rc) {
    awsiot->rc = aws_iot_mqtt_publish(&Params);
    if (NONE_ERROR != awsiot->rc) {
      char errmsg[255];
      sprintf(errmsg, "Error subscribing to %s queue", Params.pTopic);
      awsiot->onerror(awsiot, errmsg);
    }
  }

  syslog(LOG_DEBUG, "awsiot_client_publish: payload_len=%d, payload=%s", payload_len, payload);

  return aws_iot_mqtt_publish(&Params);
}

void awsiot_client_close(awsiot_client *awsiot, const char *payload) {
  if(payload != NULL) {
    awsiot_client_publish(awsiot, payload);
  }
  awsiot->rc = aws_iot_mqtt_disconnect();
  awsiot->onclose(awsiot, payload);
}
