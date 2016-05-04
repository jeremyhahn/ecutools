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

#include "passthru_iotbridge.h"
/*
int iotbridge_awsiot_onmessage(MQTTCallbackParams params) {

  uint32_t payload_len = params.MessageParams.PayloadLen;
  char* payload = (char *) params.MessageParams.pPayload;

  syslog(LOG_DEBUG, "iotbridge_awsiot_onmessage: payload_len=%d, payload=%s", payload_len, payload);

  if(payload == NULL || payload_len <= 0) return 0;

  while((bridge->canbus->state & CANBUS_STATE_CONNECTED) == 0) {
    syslog(LOG_DEBUG, "iotbridge_awsiot_onmessage: waiting for CAN connection");
    sleep(1);
  }

  if(strcmp(payload, "cmd:log") == 0) {
    if(pthread_create(&bridge->canbus_thread, NULL, iotbridge_canbus_logger_thread, (void *)bridge) == -1) {
      syslog(LOG_ERR, "iotbridge_awsiot_onmessage: %s", strerror(errno));
      return 0;
    }
    syslog(LOG_DEBUG, "iotbridge_awsiot_onmessage: canbus logger thread created");
    return 0;
  }
  else if(strcmp(payload, "cmd:nolog") == 0) {
    if(pthread_cancel(bridge->canbus_thread) == -1) {
      syslog(LOG_ERR, "iotbridge_awsiot_onmessage: %s", strerror(errno));
      return 0;
    }
    return 0;
  }
  else if(strstr(payload, "cmd:filter:") != NULL) {
    strtok(payload, ":");
    strtok(NULL, ":");
    char *pch2 = strtok(NULL, ":");
    unsigned hex = (int)strtol(pch2, NULL, 16);
    struct can_filter filter[1];
    filter[0].can_id = hex;
    filter[0].can_mask = 0x000007FF;
    if(setsockopt(bridge->canbus->socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) == -1) {
      syslog(LOG_ERR, "iotbridge_awsiot_onmessage: %s", strerror(errno));
      return 0;
    }
    syslog(LOG_DEBUG, "iotbridge_awsiot_onmessage: filter applied");
    return 0;
  }
  else if(strcmp(payload, "cmd:nofilter") == 0) {
	canbus_close(bridge->canbus);
    if(canbus_connect(bridge->canbus) != 0) {
      syslog(LOG_CRIT, "iotbridge_awsiot_onmessage: unable to connect to CAN");
      return 0;
    }
    syslog(LOG_DEBUG, "iotbridge_awsiot_onmessage: filter cleared");
    return 0;
  }
  else {

    if(strstr(payload, "#") == NULL) {
      syslog(LOG_ERR, "iotbridge_awsiot_onmessage: dropping invalid CAN payload: %s", payload);
	    return 0;
    }

    char *can_id = strsep(&payload, "#");
    char *can_message = strsep(&payload, "#");

    struct can_frame *frame = malloc(sizeof(struct can_frame));
    memset(frame, 0, sizeof(struct can_frame));
    frame->can_id = strtol(can_id, NULL, 16);
    frame->can_dlc = 8;

    size_t count = 0;
    for(count = 0; count < sizeof(frame->data)/sizeof(frame->data[0]); count++) {
      sscanf(can_message, "%2hhx", &frame->data[count]);
	    can_message += 2 * sizeof(char);
    }

    //int i;
    //for(i=0; i<sizeof(bridge->filters); i++) {
      //if(bridge->filters[i] != NULL) {
         //iotbridge_process_filter(bridge, frame);
      //}
    //}

    if(canbus_write(bridge->canbus, frame) == -1) {
      syslog(LOG_ERR, "iotbridge_awsiot_onmessage: unable to write frame to CAN bus. error: %s", strerror(errno));
    }

	free(frame);
	return 0;
  }
}*/

void iotbridge_shadow_onopen(passthru_shadow *shadow) {
  syslog(LOG_DEBUG, "iotbridge_shadow_onopen");
}

void iotbridge_shadow_onerror(passthru_shadow *shadow, const char *message) {
  syslog(LOG_DEBUG, "iotbridge_shadow_onerror: message=%s", message);
}

void iotbridge_shadow_onget(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t) {
  syslog(LOG_DEBUG, "iotbridge_shadow_onget: message=%s", pJsonValueBuffer);
}

void iotbridge_shadow_ondelta(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t) {
  syslog(LOG_DEBUG, "iotbridge_shadow_ondelta pJsonValueBuffer=%.*s", valueLength, pJsonValueBuffer);
  if(passthru_shadow_build_report_json(DELTA_REPORT, SHADOW_MAX_SIZE_OF_RX_BUFFER, pJsonValueBuffer, valueLength)) {
    messageArrivedOnDelta = true;
  }
}

void iotbridge_shadow_onupdate(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
    const char *pReceivedJsonDocument, void *pContextData) {

  if (status == SHADOW_ACK_TIMEOUT) {
    syslog(LOG_DEBUG, "Update Timeout--");
  }
  else if(status == SHADOW_ACK_REJECTED) {
    syslog(LOG_DEBUG, "Update Rejected");
  }
  else if(status == SHADOW_ACK_ACCEPTED) {
    syslog(LOG_DEBUG, "Update Accepted !!");
  }
}

void *iotbridge_shadow_yield_thread(void *ptr) {

  syslog(LOG_DEBUG, "iotbridge_shadow_yield_thread: started");
  iotbridge *bridge = (iotbridge *)ptr;

  while(1) {
    bridge->shadow->rc = aws_iot_shadow_yield(bridge->shadow->mqttClient, 1000);
    if(NETWORK_ATTEMPTING_RECONNECT == bridge->shadow->rc) {
      syslog(LOG_DEBUG, "Attempting to reconnect to AWS IoT shadow service");
      sleep(1);
      continue;
    }

    if(messageArrivedOnDelta) {
      syslog(LOG_DEBUG, "Sending delta message back. message=%s\n", DELTA_REPORT);
      bridge->shadow->rc = aws_iot_shadow_update(bridge->shadow->mqttClient, AWS_IOT_MY_THING_NAME, DELTA_REPORT, bridge->shadow->onupdate, NULL, 2, true);
      messageArrivedOnDelta = false;
    }

    syslog(LOG_DEBUG, "iotbridge_shadow_yield_thread: waiting for delta");
    sleep(1);
  }

  syslog(LOG_DEBUG, "iotbridge_shadow_yield_thread: stopping");
  return NULL;

}

iotbridge *iotbridge_new() {
  iotbridge *bridge = malloc(sizeof(iotbridge));
  memset(bridge, 0, sizeof(iotbridge));

  bridge->canbus = malloc(sizeof(canbus_client));
  memset(bridge->canbus, 0, sizeof(canbus_client));

  bridge->shadow = malloc(sizeof(passthru_shadow));
  memset(bridge->shadow, 0, sizeof(passthru_shadow));
  bridge->shadow->onerror = &iotbridge_shadow_onerror;
  bridge->shadow->onopen = &iotbridge_shadow_onopen;
  bridge->shadow->ondelta = &iotbridge_shadow_ondelta;
  bridge->shadow->onupdate = &iotbridge_shadow_onupdate;

  return bridge;
}

int iotbridge_run(iotbridge *bridge) {
  canbus_connect(bridge->canbus);
  if(!canbus_isconnected(bridge->canbus)) {
    syslog(LOG_CRIT, "iotbridge_run: unable to connect to CAN");
    return 1;
  }
  passthru_shadow_connect(bridge->shadow);
  if(bridge->shadow->rc != NONE_ERROR) {
    syslog(LOG_CRIT, "iotbridge_run: unable to connect to AWS IoT shadow service");
    return 3;
  }
  pthread_create(&bridge->shadow->yield_thread, NULL, iotbridge_shadow_yield_thread, (void *)bridge);
  pthread_join(bridge->shadow->yield_thread, NULL);
  syslog(LOG_DEBUG, "iotbridge_run: bridge closed");
  return 0;
}

void iotbridge_process_filter(iotbridge *bridge, struct can_frame *frame) {
  syslog(LOG_DEBUG, "iotbridge_process_filter: ");
  canbus_print_frame(frame);
}

void iotbridge_close(iotbridge *bridge, const char *message) {
  syslog(LOG_DEBUG, "iotbridge_close: closing bridge");
  canbus_close(bridge->canbus);
  passthru_shadow_disconnect(bridge->shadow);
  iotbridge_destroy(bridge);
  syslog(LOG_DEBUG, "iotbridge_close: bridge closed");
}

void iotbridge_destroy(iotbridge *bridge) {
  free(bridge->shadow);
  free(bridge->canbus);
}
