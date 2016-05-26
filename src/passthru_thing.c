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

#include "passthru_thing.h"

passthru_thing *thing;
awsiot_client *awsiot;

static char my_shadow_get_topic[121];
static char my_shadow_update_topic[121];
static char my_shadow_get_accepted_topic[121];

void passthru_thing_sync_initial_state(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData) {
  syslog(LOG_DEBUG, "passthru_thing_initial_state_callback: topicName=%s, topicNameLen=%u, pData=%s", topicName, (unsigned int)topicNameLen, (char *)pData);

  thing->state = THING_STATE_CONNECTING;

  size_t json_len = strlen(topicName)-topicNameLen;
  char json[json_len+1];
  memcpy(json, &topicName[topicNameLen], json_len);
  json[json_len] = '\0';

  shadow_message *message = passthru_shadow_parser_parse(json);
  // clear connection=2 to prevent connection handler from disconnecting
  if(message->state->reported->connection) message->state->reported->connection = NULL;
  passthru_shadow_router_route(thing, message);
  passthru_shadow_parser_free_message(message);

  //free(awsiot->client); segfault?
  free(awsiot);
}

unsigned int passthru_thing_set_initial_state() {

  syslog(LOG_DEBUG, "passthru_thing_set_initial_state: getting state");

  thing->state = THING_STATE_INITIALIZING;

  awsiot = malloc(sizeof(awsiot_client));
  awsiot->client = malloc(sizeof(AWS_IoT_Client));
  awsiot->onmessage = &passthru_thing_sync_initial_state;
  awsiot_client_connect(awsiot);
  awsiot_client_subscribe(awsiot, my_shadow_get_accepted_topic);
  awsiot_client_publish(awsiot, my_shadow_get_topic, "{}");
  awsiot_client_publish(awsiot, my_shadow_update_topic, "{\"state\":{\"desired\":{\"j2534\":null},\"reported\":{\"j2534\":null}}}");

  unsigned int i = 0;
  while(thing->state & THING_STATE_INITIALIZING) {

    if(i == 10) { // timeout
      syslog(LOG_DEBUG, "passthru_thing_set_initial_state: no state to sync");
      return 1;
    }

    aws_iot_mqtt_yield(awsiot->client, 200);
    if(awsiot->rc == NETWORK_ATTEMPTING_RECONNECT) {
      syslog(LOG_DEBUG, "passthru_thing_set_initial_state: waiting for network to reconnect");
      sleep(1);
      continue;
    }

    i++;
  }

  return 0;
}

void passthru_thing_shadow_onopen(passthru_shadow *shadow) {
  syslog(LOG_DEBUG, "passthru_thing_shadow_onopen");
}

void passthru_thing_shadow_ondelta(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t) {
  syslog(LOG_DEBUG, "passthru_thing_shadow_ondelta: pJsonValueBuffer=%.*s", valueLength, pJsonValueBuffer);
  if(passthru_shadow_build_report_json(DELTA_REPORT, SHADOW_MAX_SIZE_OF_RX_BUFFER, pJsonValueBuffer, valueLength)) {
    messageArrivedOnDelta = true;
  }
}

void passthru_thing_shadow_onupdate(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
    const char *pReceivedJsonDocument, void *pContextData) {

  syslog(LOG_DEBUG, "passthru_thing_shadow_onupdate: pThingName=%s, pReceivedJsonDocument=%s, pContextData=%s", pThingName, pReceivedJsonDocument, (char *) pContextData);

  if(action == SHADOW_GET) {
    syslog(LOG_DEBUG, "passthru_thing_shadow_onupdate: SHADOW_GET");
  } 
  else if(action == SHADOW_UPDATE) {
    syslog(LOG_DEBUG, "passthru_thing_shadow_onupdate: SHADOW_UPDATE");
  } 
  else if(action == SHADOW_DELETE) {
    syslog(LOG_DEBUG, "passthru_thing_shadow_onupdate: SHADOW_DELETE");
  }

  if (status == SHADOW_ACK_TIMEOUT) {
    syslog(LOG_DEBUG, "passthru_thing_shadow_onupdate: Update Timeout");
  }
  else if(status == SHADOW_ACK_REJECTED) {
    syslog(LOG_DEBUG, "passthru_thing_shadow_onupdate: Update Rejected");
  }
  else if(status == SHADOW_ACK_ACCEPTED) {
    syslog(LOG_DEBUG, "passthru_thing_shadow_onupdate: Update Accepted");
  }

  // Handle messages sent by this device
  if(strncmp(pThingName, AWS_IOT_MY_THING_NAME, strlen(AWS_IOT_MY_THING_NAME)) == 0) {
    shadow_message *message = passthru_shadow_parser_parse(pReceivedJsonDocument);
    passthru_shadow_router_route(thing, message);
    passthru_shadow_parser_free_message(message);
  }

}

void passthru_thing_shadow_onget(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t) {
  syslog(LOG_DEBUG, "passthru_thing_shadow_onget: message=%s", pJsonValueBuffer);
}

void passthru_thing_shadow_ondisconnect() {
  thing->state = THING_STATE_DISCONNECTED;
  syslog(LOG_DEBUG, "passthru_thing_shadow_ondisconnect: disconnected");
}

void passthru_thing_shadow_onerror(passthru_shadow *shadow, const char *message) {
  syslog(LOG_ERR, "passthru_thing_shadow_onerror: message=%s", message);
}

void *passthru_thing_shadow_yield_thread(void *ptr) {

  passthru_thing_send_connect_report(thing);

  while((thing->state & THING_STATE_INITIALIZING) || (thing->state & THING_STATE_CONNECTED) || (thing->state & THING_STATE_CLOSING)) {

    thing->shadow->rc = aws_iot_shadow_yield(thing->shadow->mqttClient, 200);
    if(thing->shadow->rc == NETWORK_ATTEMPTING_RECONNECT) {
      syslog(LOG_DEBUG, "Attempting to reconnect to AWS IoT shadow service");
      sleep(1);
      continue;
    }

    if(messageArrivedOnDelta) {
      syslog(LOG_DEBUG, "passthru_thing_shadow_yield_thread: Sending delta. message=%s", DELTA_REPORT);
      passthru_shadow_update(thing->shadow, DELTA_REPORT);
      messageArrivedOnDelta = false;
    }

    if(thing->state & THING_STATE_CLOSING) {
      if(passthru_thing_send_disconnect_report(thing) != 0) {
        syslog(LOG_ERR, "passthru_thing_shadow_yield_thread: failed to send disconnect report!");
        sleep(2);
        continue;
      }
    }

    //syslog(LOG_DEBUG, "passthru_thing_shadow_yield_thread: waiting for delta");
    sleep(1);
  }

  syslog(LOG_DEBUG, "passthru_thing_shadow_yield_thread: stopping");
  return NULL;
}

int passthru_thing_send_connect_report() {
  char pJsonDocument[SHADOW_MAX_SIZE_OF_RX_BUFFER];
  char state[255] = "{\"connection\": 1}";
  if(!passthru_shadow_build_report_json(pJsonDocument, SHADOW_MAX_SIZE_OF_RX_BUFFER, state, strlen(state))) {
    syslog(LOG_ERR, "passthru_thing_send_connect_report: failed to build JSON state message. state=%s", state);
    return 1;
  }
  return passthru_shadow_update(thing->shadow, pJsonDocument);
}

int passthru_thing_send_disconnect_report() {
  char pJsonDocument[SHADOW_MAX_SIZE_OF_RX_BUFFER];
  char state[255] = "{\"connection\": 2}";
  if(!passthru_shadow_build_report_json(pJsonDocument, SHADOW_MAX_SIZE_OF_RX_BUFFER, state, strlen(state))) {
    syslog(LOG_ERR, "passthru_thing_send_disconnect_report: failed to build JSON state message. state=%s", state);
    return 1;
  }
  passthru_shadow_update(thing->shadow, pJsonDocument);
}

void passthru_thing_init(thing_init_params *params) {

  thing = malloc(sizeof(passthru_thing));
  thing->params = params;
  thing->name = malloc(sizeof(char) * strlen(AWS_IOT_MY_THING_NAME)+1);
  strcpy(thing->name, AWS_IOT_MY_THING_NAME);

  thing->shadow = malloc(sizeof(passthru_shadow));
  thing->shadow->mqttClient = malloc(sizeof(AWS_IoT_Client));
  thing->shadow->clientId = thing->name;

  thing->shadow->onopen = &passthru_thing_shadow_onopen;
  thing->shadow->ondelta = &passthru_thing_shadow_ondelta;
  thing->shadow->onupdate = &passthru_thing_shadow_onupdate;
  thing->shadow->onget = &passthru_thing_shadow_onget;
  thing->shadow->ondisconnect = &passthru_thing_shadow_ondisconnect;
  thing->shadow->onerror = &passthru_thing_shadow_onerror;

  snprintf(my_shadow_get_topic, 120, PASSTHRU_SHADOW_GET_TOPIC, thing->name);
  snprintf(my_shadow_update_topic, 120, PASSTHRU_SHADOW_UPDATE_TOPIC, thing->name);
  snprintf(my_shadow_get_accepted_topic, 120, PASSTHRU_SHADOW_GET_ACCEPTED_TOPIC, thing->name);
}

int passthru_thing_run() {
  passthru_thing_set_initial_state();
  thing->state = THING_STATE_CONNECTING;
  if(passthru_shadow_connect(thing->shadow) != 0) {
    syslog(LOG_CRIT, "passthru_thing_run: unable to connect to AWS IoT shadow service");
    return 1;
  }
  if(thing->shadow->rc != SUCCESS) {
    syslog(LOG_CRIT, "passthru_thing_run: unable to connect to AWS IoT shadow service");
    return 2;
  }
  thing->state = THING_STATE_CONNECTED;
  pthread_create(&thing->shadow->yield_thread, NULL, passthru_thing_shadow_yield_thread, NULL);
  pthread_join(thing->shadow->yield_thread, NULL);
  syslog(LOG_DEBUG, "passthru_passthru_thing: run loop complete");
  return 0;
}

void passthru_thing_disconnect() {
  syslog(LOG_DEBUG, "passthru_thing_disconnect: disconnecting");
  thing->state = THING_STATE_DISCONNECTING;
  if(passthru_shadow_disconnect(thing->shadow) != 0) {
    syslog(LOG_ERR, "passthru_thing_disconnect: failed to disconnect from AWS IoT. rc=%d", thing->shadow->rc);
  }
}

void passthru_thing_close() {
  if(!(thing->state & THING_STATE_CONNECTED)) return; 
  syslog(LOG_DEBUG, "passthru_thing_close: closing thing. name=%s", thing->params->thingId);
  thing->state = THING_STATE_CLOSING;
  while(!(thing->state & THING_STATE_DISCONNECTED)) {
    syslog(LOG_DEBUG, "passthru_thing_close: waiting for thing to disconnect");
    sleep(1);
  }
  thing->state = THING_STATE_CLOSED;
  syslog(LOG_DEBUG, "passthru_thing_close: closed");
}

void passthru_thing_destroy() {
  syslog(LOG_DEBUG, "passthru_thing_destroy");
  passthru_shadow_destroy(thing->shadow);
  free(thing->name);
  free(thing->shadow->mqttClient);
  free(thing->shadow);
  free(thing);
}
