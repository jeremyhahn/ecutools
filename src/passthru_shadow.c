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

#include "passthru_shadow.h"

bool messageArrivedOnDelta = false;

int passthru_shadow_connect(passthru_shadow *shadow) {

  shadow->mqttClient = malloc(sizeof(AWS_IoT_Client));
  shadow->rc = SUCCESS;
  
  char errmsg[255];
  char rootCA[255];
  char clientCRT[255];
  char clientKey[255];

  sprintf(rootCA, "%s/%s", shadow->certDir, AWS_IOT_ROOT_CA_FILENAME);
  sprintf(clientCRT, "%s/%s", shadow->certDir, AWS_IOT_CERTIFICATE_FILENAME);
  sprintf(clientKey, "%s/%s", shadow->certDir, AWS_IOT_PRIVATE_KEY_FILENAME); 

  syslog(LOG_DEBUG, "rootCA %s", rootCA);
  syslog(LOG_DEBUG, "clientCRT %s", clientCRT);
  syslog(LOG_DEBUG, "clientKey %s", clientKey);

  ShadowInitParameters_t sp = ShadowInitParametersDefault;
  sp.pHost = AWS_IOT_MQTT_HOST;
  sp.port = AWS_IOT_MQTT_PORT;
  sp.pClientCRT = clientCRT;
  sp.pClientKey = clientKey;
  sp.pRootCA = rootCA;
  sp.enableAutoReconnect = true;
  sp.disconnectHandler = shadow->ondisconnect;

  shadow->rc = aws_iot_shadow_init(shadow->mqttClient, &sp);
  if(shadow->rc != SUCCESS) {
    sprintf(errmsg, "aws_iot_shadow_init error rc=%d", shadow->rc);
    shadow->onerror(shadow, errmsg);
    return 1;
  }

  ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
  scp.pMyThingName = shadow->thingName;
  scp.pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;

  shadow->rc = aws_iot_shadow_connect(shadow->mqttClient, &scp);
  if(shadow->rc != SUCCESS) {
    sprintf(errmsg, "aws_iot_shadow_connect error rc=%d", shadow->rc);
    shadow->onerror(shadow, errmsg);
    return 2;
  }

  shadow->rc = aws_iot_shadow_set_autoreconnect_status(shadow->mqttClient, true);
  if(shadow->rc != SUCCESS) {
    sprintf(errmsg, "aws_iot_shadow_set_autoreconnect_status error rc=%d", shadow->rc);
    shadow->onerror(shadow, errmsg);
    return 3;
  }

  jsonStruct_t deltaObject;
  deltaObject.pData = DELTA_REPORT;
  deltaObject.pKey = "state";
  deltaObject.type = SHADOW_JSON_OBJECT;
  deltaObject.cb = shadow->ondelta;

  shadow->rc = aws_iot_shadow_register_delta(shadow->mqttClient, &deltaObject);
  if(shadow->rc != SUCCESS) {
    sprintf(errmsg, "aws_iot_shadow_register_delta error rc=%d", shadow->rc);
    shadow->onerror(shadow, errmsg);
    return 4;
  }

  shadow->onopen(shadow);
  return 0;
}

int passthru_shadow_report_delta(passthru_shadow *shadow) {
  syslog(LOG_DEBUG, "Sending delta report: %s", DELTA_REPORT);
  return passthru_shadow_update(shadow, DELTA_REPORT, NULL);
}

void passthru_shadow_get(passthru_shadow *shadow) {
  syslog(LOG_DEBUG, "passthru_shadow_get");
  shadow->rc = aws_iot_shadow_get(shadow->mqttClient, shadow->thingName, shadow->onget, NULL, 2, true);
  if(shadow->rc != SUCCESS) {
    char errmsg[255];
    sprintf(errmsg, "aws_iot_shadow_get error rc=%d", shadow->rc);
    shadow->onerror(shadow, errmsg);
  }
}

int passthru_shadow_update(passthru_shadow *shadow, char *message, void *pContextData) {
  syslog(LOG_DEBUG, "passthru_shadow_update: message=%s", message);
  shadow->rc = aws_iot_shadow_update(shadow->mqttClient, shadow->thingName, message, shadow->onupdate, pContextData, 2, true);
  if(shadow->rc != SUCCESS) {
    char errmsg[255];
    sprintf(errmsg, "aws_iot_shadow_update error rc=%d", shadow->rc);
    shadow->onerror(shadow, errmsg);
    return 1;
  }
  return 0;
}

int passthru_shadow_disconnect(passthru_shadow *shadow) {
  syslog(LOG_DEBUG, "passthru_shadow_disconnect");
  shadow->rc = aws_iot_shadow_disconnect(shadow->mqttClient);
  if(shadow->rc != SUCCESS) {
    char errmsg[255];
    sprintf(errmsg, "aws_iot_shadow_disconnect error rc=%d", shadow->rc);
    shadow->onerror(shadow, errmsg);
    return 1;
  }
  shadow->ondisconnect();
}

void passthru_shadow_destroy(passthru_shadow *shadow) {
  syslog(LOG_DEBUG, "passthru_shadow_destroy: clientId=%s", shadow->thingName);
  if(shadow->mqttClient != NULL) {
    free(shadow->mqttClient);
    shadow->mqttClient = NULL;
  }
}

bool passthru_shadow_build_report_json(char *pJsonDocument, size_t maxSizeOfJsonDocument, const char *pData, uint32_t pDataLen) {

  int32_t ret;

  if (pJsonDocument == NULL) {
    return false;
  }

  char tempClientTokenBuffer[MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE];

  if(aws_iot_fill_with_client_token(tempClientTokenBuffer, MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE) != SUCCESS) {
    return false;
  }

  ret = snprintf(pJsonDocument, maxSizeOfJsonDocument, "{\"state\":{\"reported\":%.*s}, \"clientToken\":\"%s\"}", pDataLen, pData, tempClientTokenBuffer);

  if (ret >= maxSizeOfJsonDocument || ret < 0) {
    return false;
  }

  return true;
}
