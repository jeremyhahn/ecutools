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

void passthru_thing_shadow_onopen(passthru_shadow *shadow) {
  syslog(LOG_DEBUG, "passthru_thing_shadow_onopen");
}

void passthru_thing_shadow_ondelta(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t) {
  syslog(LOG_DEBUG, "passthru_thing_shadow_ondelta: pJsonValueBuffer=%s, valueLength=%d", pJsonValueBuffer, valueLength);
  const char *json = MYSTRING_COPY(pJsonValueBuffer, valueLength);
  shadow_desired *desired = passthru_shadow_parser_parse_delta(json);
  passthru_shadow_router_route_delta(thing, desired);
  passthru_shadow_parser_free_desired(desired);
  free(json);
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

  passthru_shadow_state_open(thing->params->cacheDir, "w");
  passthru_shadow_state_write(pReceivedJsonDocument);
  passthru_shadow_state_close();

  if(strncmp(pThingName, AWS_IOT_MY_THING_NAME, strlen(AWS_IOT_MY_THING_NAME)) == 0) {
    shadow_message *message = passthru_shadow_parser_parse_state(pReceivedJsonDocument);
    passthru_shadow_router_route_message(thing, message);
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

  while((thing->state & THING_STATE_INITIALIZING) || (thing->state & THING_STATE_CONNECTED) || 
        (thing->state & THING_STATE_CLOSING) || thing->shadow->rc == NETWORK_ATTEMPTING_RECONNECT) {

    thing->shadow->rc = aws_iot_shadow_yield(thing->shadow->mqttClient, 200);
    if(thing->shadow->rc == NETWORK_ATTEMPTING_RECONNECT) {
      syslog(LOG_DEBUG, "Attempting to reconnect to AWS IoT shadow service");
      sleep(1);
      continue;
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

  syslog(LOG_DEBUG, "passthru_thing_shadow_yield_thread: stopping. thing->shadow->rc=%d", thing->shadow->rc);
  return NULL;
}

int passthru_thing_send_connect_report() {
  char pJsonDocument[SHADOW_MAX_SIZE_OF_RX_BUFFER];
  char state[255] = "{\"connection\": 1}";
  if(!passthru_shadow_build_report_json(pJsonDocument, SHADOW_MAX_SIZE_OF_RX_BUFFER, state, strlen(state))) {
    syslog(LOG_ERR, "passthru_thing_send_connect_report: failed to build JSON state message. state=%s", state);
    return 1;
  }
  return passthru_shadow_update(thing->shadow, pJsonDocument, NULL);
}

int passthru_thing_send_disconnect_report() {
  char pJsonDocument[SHADOW_MAX_SIZE_OF_RX_BUFFER];
  char state[255] = "{\"connection\": 2}";
  if(!passthru_shadow_build_report_json(pJsonDocument, SHADOW_MAX_SIZE_OF_RX_BUFFER, state, strlen(state))) {
    syslog(LOG_ERR, "passthru_thing_send_disconnect_report: failed to build JSON state message. state=%s", state);
    return 1;
  }
  passthru_shadow_update(thing->shadow, pJsonDocument, NULL);
}

void passthru_thing_send_report(const char *json) {
  syslog(LOG_DEBUG, "passthru_thing_send_report: json=%s", json);
  char msgbuf[SHADOW_MAX_SIZE_OF_RX_BUFFER];
  if(passthru_shadow_build_report_json(msgbuf, SHADOW_MAX_SIZE_OF_RX_BUFFER, json, strlen(json))) {
    syslog(LOG_DEBUG, "passthru_thing_send_report: sending report: %s", msgbuf);
    passthru_shadow_update(thing->shadow, msgbuf, NULL);
  }
}

void passthru_thing_init(passthru_thing_params *params) {

  thing = malloc(sizeof(passthru_thing));
  thing->params = params;
  thing->name = params->thingName;

  thing->shadow = malloc(sizeof(passthru_shadow));
  memset(thing->shadow, 0, sizeof(passthru_shadow));
  thing->shadow->mqttClient = malloc(sizeof(AWS_IoT_Client));
  thing->shadow->thingName = thing->name;
  thing->shadow->certDir = params->certDir;

  thing->shadow->onopen = &passthru_thing_shadow_onopen;
  thing->shadow->ondelta = &passthru_thing_shadow_ondelta;
  thing->shadow->onupdate = &passthru_thing_shadow_onupdate;
  thing->shadow->onget = &passthru_thing_shadow_onget;
  thing->shadow->ondisconnect = &passthru_thing_shadow_ondisconnect;
  thing->shadow->onerror = &passthru_thing_shadow_onerror;

  thing->shadow->get_topic = MYSTRING_COPYF(PASSTHRU_SHADOW_GET_TOPIC, 255, thing->name);
  thing->shadow->get_accepted_topic = MYSTRING_COPYF(PASSTHRU_SHADOW_GET_ACCEPTED_TOPIC, 255, thing->name);
  thing->shadow->update_topic = MYSTRING_COPYF(PASSTHRU_SHADOW_UPDATE_TOPIC, 255, thing->name);
  thing->shadow->update_accepted_topic = MYSTRING_COPYF(PASSTHRU_SHADOW_GET_ACCEPTED_TOPIC, 255, thing->name);

  thing->j2534 = malloc(sizeof(passthru_j2534));
  thing->j2534->clients = malloc(sizeof(vector));
  vector_init(thing->j2534->clients);
}

int passthru_thing_run() {

  thing->state = THING_STATE_INITIALIZING;
  passthru_shadow_state_sync(thing);

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
  syslog(LOG_DEBUG, "passthru_thing_close: closing thing. name=%s", thing->params->thingName);
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
  free(thing->shadow->mqttClient);
  free(thing->shadow);
  vector_free(thing->j2534->clients);
  free(thing->j2534->clients);
  free(thing->j2534);
  free(thing);
}
