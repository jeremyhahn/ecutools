#include "iotbridge.h"

void iotbridge_awsiot_onopen(awsiot_client *awsiot) {
  syslog(LOG_DEBUG, "iotbridge_awsiot_onopen");
}

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

    /*
	int i;
	for(i=0; i<sizeof(bridge->filters); i++) {
	  if(bridge->filters[i] != NULL) {
        iotbridge_process_filter(bridge, frame);
      }
	}*/

    if(canbus_write(bridge->canbus, frame) == -1) {
      syslog(LOG_ERR, "iotbridge_awsiot_onmessage: unable to write frame to CAN bus. error: %s", strerror(errno));
    }

	free(frame);
	return 0;
  }

}

void iotbridge_awsiot_onclose(awsiot_client *awsiot) {
  syslog(LOG_DEBUG, "iotbridge_awsiot_onclose: connection closed");
}

void iotbridge_awsiot_ondisconnect(void) {
  syslog(LOG_DEBUG, "iotbridge_awsiot_ondisconnect: MQTT Disconnect");
  if(aws_iot_is_autoreconnect_enabled()){
    syslog(LOG_DEBUG, "Auto Reconnect is enabled, Reconnecting attempt will start now");
  }
  else {
    syslog(LOG_DEBUG, "Auto Reconnect not enabled. Starting manual reconnect...");
    IoT_Error_t rc = aws_iot_mqtt_attempt_reconnect();
    if(RECONNECT_SUCCESSFUL == rc){
      syslog(LOG_DEBUG, "Manual reconnect successful");
    }
    else {
      syslog(LOG_ERR, "Manual reconnect failed. IoT_Error_t: %d", rc);
    }
  }
}

void iotbridge_awsiot_onerror(IoT_Error_t *awsiot, const char *message) {
  syslog(LOG_DEBUG, "iotbridge_awsiot_onerror: message=%s", message);
}

void *iotbridge_awsiot_canbus_subscribe_thread(void *ptr) {

  syslog(LOG_DEBUG, "iotbridge_awsiot_subscribe_thread: started");
  iotbridge *bridge = (iotbridge *)ptr;
  awsiot_client_canbus_subscribe(bridge->awsiot);

  while(1) {
	bridge->awsiot->rc = aws_iot_mqtt_yield(100);
	if(NETWORK_ATTEMPTING_RECONNECT == bridge->awsiot->rc) {
	  syslog(LOG_DEBUG, "Attempting to reconnect to AWS IoT service");
	  sleep(1);
	  continue;
    }
	sleep(1);
  }

  syslog(LOG_DEBUG, "iotbridge_awsiot_canbus_subscribe_thread: stopping");
  return NULL;
}

void *iotbridge_awsiot_canbus_publish_thread(void *ptr) {
  syslog(LOG_DEBUG, "iotbridge_awsiot_publish_thread: started");
  iotbridge__publish_thread_args *args = (iotbridge__publish_thread_args *)ptr;
  awsiot_client_canbus_publish(args->awsiot, args->payload);
  syslog(LOG_DEBUG, "iotbridge_awsiot_canbus_publish_thread: stopping");
  return NULL;
}

void *iotbridge_canbus_logger_thread(void *ptr) {

  syslog(LOG_DEBUG, "iotbridge_canbus_logger_thread: running");

  iotbridge *bridge = (iotbridge *)ptr;

  int can_frame_len = sizeof(struct can_frame);
  struct can_frame frame;
  memset(&frame, 0, can_frame_len);

  int data_len = can_frame_len + 25;
  char data[data_len];
  memset(data, 0, data_len);

  while(!awsiot_client_isconnected()) {
	syslog(LOG_DEBUG, "iotbridge_canbus_logger_thread: waiting for connection to AWS IoT service");
	sleep(1);
  }

  while(awsiot_client_isconnected() &&
    (bridge->canbus->state & CANBUS_STATE_CONNECTED) &&
    canbus_read(bridge->canbus, &frame) > 0) {

    memset(data, 0, data_len);
    canbus_framecpy(&frame, data);

    if(frame.can_id & CAN_ERR_FLAG) {
      syslog(LOG_ERR, "iotbridge_canbus_logger_thread: CAN ERROR: %s", data);
      continue;
    }

    iotbridge__publish_thread_args *args = malloc(sizeof(iotbridge__publish_thread_args));
    memset(args, 0, sizeof(iotbridge__publish_thread_args));
    args->awsiot = bridge->awsiot;
    args->payload = data;

    if(pthread_create(&bridge->awsiot->canbus_publish_thread, NULL, iotbridge_awsiot_canbus_publish_thread, (void *)args) == -1) {
      syslog(LOG_ERR, "cwebsocket_read_data: %s", strerror(errno));
      return -1;
    }

  }

  syslog(LOG_DEBUG, "iotbridge_canbus_logger_thread: stopping");
  return NULL;
}

iotbridge *iotbridge_new() {
  iotbridge *bridge = malloc(sizeof(iotbridge));
  memset(bridge, 0, sizeof(iotbridge));
  bridge->canbus = malloc(sizeof(canbus_client));
  memset(bridge->canbus, 0, sizeof(canbus_client));
  bridge->awsiot = malloc(sizeof(awsiot_client));
  memset(bridge->awsiot, 0, sizeof(awsiot_client));
  bridge->awsiot->onopen = &iotbridge_awsiot_onopen;
  bridge->awsiot->onmessage = &iotbridge_awsiot_onmessage;
  bridge->awsiot->ondisconnect = &iotbridge_awsiot_ondisconnect;
  bridge->awsiot->onclose = &iotbridge_awsiot_onclose;
  bridge->awsiot->onerror = &iotbridge_awsiot_onerror;
  return bridge;
}

int iotbridge_run(iotbridge *bridge) {
  canbus_connect(bridge->canbus);
  if(!canbus_isconnected(bridge->canbus)) {
    syslog(LOG_CRIT, "iotbridge_run: unable to connect to CAN");
    return -1;
  }
  awsiot_client_connect(bridge->awsiot);
  if(bridge->awsiot->rc != NONE_ERROR) {
    syslog(LOG_CRIT, "iotbridge_run: unable to connect to AWS IoT service");
    return -1;
  }
  pthread_create(&bridge->awsiot->canbus_subscribe_thread, NULL, iotbridge_awsiot_canbus_subscribe_thread, (void *)bridge);
  pthread_join(bridge->awsiot->canbus_subscribe_thread, NULL);
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
  awsiot_client_close(bridge->awsiot, message);
  iotbridge_destroy(bridge);
  syslog(LOG_DEBUG, "iotbridge_close: bridge closed");
}

void iotbridge_destroy(iotbridge *bridge) {
  free(bridge->awsiot);
  free(bridge->canbus);
}
