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

#include "passthru_thing.h"

void passthru_thing_shadow_onopen(passthru_shadow *shadow) {
  syslog(LOG_DEBUG, "passthru_thing_shadow_onopen");
}

void passthru_thing_shadow_ondelta(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t) {
  syslog(LOG_DEBUG, "passthru_thing_shadow_ondelta pJsonValueBuffer=%.*s", valueLength, pJsonValueBuffer);
  if(passthru_shadow_build_report_json(DELTA_REPORT, SHADOW_MAX_SIZE_OF_RX_BUFFER, pJsonValueBuffer, valueLength)) {
    messageArrivedOnDelta = true;
  }
}

void passthru_thing_shadow_onupdate(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
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

void passthru_thing_shadow_onget(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t) {
  syslog(LOG_DEBUG, "passthru_thing_shadow_onget: message=%s", pJsonValueBuffer);
}

void passthru_thing_shadow_ondisconnect() {
  syslog(LOG_DEBUG, "passthru_thing_shadow_ondisconnect: disconnected");
}

void passthru_thing_shadow_onerror(passthru_shadow *shadow, const char *message) {
  syslog(LOG_ERR, "passthru_thing_shadow_onerror: message=%s", message);
}

void *passthru_thing_shadow_yield_thread(void *ptr) {

  syslog(LOG_DEBUG, "passthru_thing_shadow_yield_thread: started");

  passthru_thing *thing = (passthru_thing *)ptr;

  syslog(LOG_DEBUG, "passthru_thing_shadow_yield_thread: reporting connection");
  passthru_thing_send_connect_report(thing);

  while(thing->state & THING_STATE_CONNECTED) {

    thing->shadow->rc = aws_iot_shadow_yield(thing->shadow->mqttClient, 200);
    if(NETWORK_ATTEMPTING_RECONNECT == thing->shadow->rc) {
      syslog(LOG_DEBUG, "Attempting to reconnect to AWS IoT shadow service");
      sleep(1);
      continue;
    }

    if(messageArrivedOnDelta) {
      syslog(LOG_DEBUG, "Sending delta message back. message=%s\n", DELTA_REPORT);
      thing->shadow->rc = aws_iot_shadow_update(thing->shadow->mqttClient, AWS_IOT_MY_THING_NAME, DELTA_REPORT, thing->shadow->onupdate, NULL, 2, true);
      messageArrivedOnDelta = false;
    }

    syslog(LOG_DEBUG, "passthru_thing_shadow_yield_thread: waiting for delta");
    sleep(1);
  }

  thing->state = THING_STATE_DISCONNECTING;
  passthru_thing_send_disconnect_report(thing);

  syslog(LOG_DEBUG, "passthru_thing_shadow_yield_thread: stopping");
  return NULL;
}

void passthru_thing_send_connect_report(passthru_thing *thing) {
  char pJsonDocument[SHADOW_MAX_SIZE_OF_RX_BUFFER];
  char state[255] = "{\"connected\": \"true\"}";
  if(!passthru_shadow_build_report_json(pJsonDocument, SHADOW_MAX_SIZE_OF_RX_BUFFER, state, strlen(state))) {
    syslog(LOG_ERR, "passthru_thing_send_connect_report: failed to build JSON state message. state=%s", state);
    return;
  }
  passthru_shadow_update(thing->shadow, pJsonDocument);
}

void passthru_thing_send_disconnect_report(passthru_thing *thing) {
  char pJsonDocument[SHADOW_MAX_SIZE_OF_RX_BUFFER];
  char state[255] = "{\"connected\": \"false\"}";
  if(!passthru_shadow_build_report_json(pJsonDocument, SHADOW_MAX_SIZE_OF_RX_BUFFER, state, strlen(state))) {
    syslog(LOG_ERR, "passthru_thing_send_disconnect_report: failed to build JSON state message. state=%s", state);
    return;
  }
  passthru_shadow_update(thing->shadow, pJsonDocument);
}

passthru_thing* passthru_thing_new() {

  passthru_thing *thing = malloc(sizeof(passthru_thing));
  memset(thing, 0, sizeof(passthru_thing));

  thing->shadow = malloc(sizeof(passthru_shadow));
  memset(thing->shadow, 0, sizeof(passthru_shadow));
  thing->shadow->onopen = &passthru_thing_shadow_onopen;
  thing->shadow->ondelta = &passthru_thing_shadow_ondelta;
  thing->shadow->onupdate = &passthru_thing_shadow_onupdate;
  thing->shadow->onget = &passthru_thing_shadow_onget;
  thing->shadow->ondisconnect = &passthru_thing_shadow_ondisconnect;
  thing->shadow->onerror = &passthru_thing_shadow_onerror;

  return thing;
}

int passthru_thing_run(passthru_thing *thing) {
  thing->state = THING_STATE_CONNECTING;
  passthru_shadow_connect(thing->shadow);
  if(thing->shadow->rc != NONE_ERROR) {
    syslog(LOG_CRIT, "passthru_thing_run: unable to connect to AWS IoT shadow service");
    return 3;
  }
  thing->state = THING_STATE_CONNECTED;
  pthread_create(&thing->shadow->yield_thread, NULL, passthru_thing_shadow_yield_thread, (void *)thing);
  pthread_join(thing->shadow->yield_thread, NULL);
  syslog(LOG_DEBUG, "passthru_passthru_thing: run loop complete");
  return 0;
}

void passthru_thing_close(passthru_thing *thing) {
  syslog(LOG_DEBUG, "passthru_thing_close: closing");
  thing->state = THING_STATE_CLOSING;
  sleep(5); // let thread clean up
  passthru_shadow_disconnect(thing->shadow);
  thing->state = THING_STATE_DISCONNECTED;
  passthru_thing_destroy(thing);
  syslog(LOG_DEBUG, "passthru_thing_close: closed");
}

void passthru_thing_destroy(passthru_thing *thing) {
  syslog(LOG_DEBUG, "passthru_thing_destroy");
  free(thing->shadow);
  //free(thing);
}
