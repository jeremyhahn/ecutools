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

  syslog(LOG_DEBUG, "passthru_thing_shadow_onupdate: pThingName=%s, pReceivedJsonDocument=%s, pContextData=%s", pThingName, pReceivedJsonDocument, (char *) pContextData);

  if(strncmp(pThingName, AWS_IOT_MY_THING_NAME, strlen(AWS_IOT_MY_THING_NAME)) == 0) {
    
    syslog(LOG_DEBUG, "passthru_thing_shadow_onupdate: processing message that I sent");

    shadow_message *message = passthru_shadow_parser_parse(pReceivedJsonDocument);
    if(message && message->state && message->state->reported && message->state->reported->connected) {
      if(strncmp(message->state->reported->connected, "false", strlen("false"))) {
        syslog(LOG_DEBUG, "passthru_thing_shadow_onupdate: TODO: DISCONNECT");  
      }
      passthru_shadow_parser_free(message);
    }
  }

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

  syslog(LOG_DEBUG, "passthru_thing_shadow_yield_thread: sending connect report to AWS IoT");
  passthru_thing_send_connect_report(thing);

  while((thing->state & THING_STATE_CONNECTED) || (thing->state & THING_STATE_DISCONNECTING)) {

    thing->shadow->rc = aws_iot_shadow_yield(thing->shadow->mqttClient, 200);
    if(NETWORK_ATTEMPTING_RECONNECT == thing->shadow->rc) {
      syslog(LOG_DEBUG, "Attempting to reconnect to AWS IoT shadow service");
      sleep(1);
      continue;
    }

    if(messageArrivedOnDelta) {
      syslog(LOG_DEBUG, "Sending delta message back. message=%s\n", DELTA_REPORT);
      passthru_shadow_update(&thing->shadow, DELTA_REPORT);
      messageArrivedOnDelta = false;
    }

    if(thing->state & THING_STATE_DISCONNECTING) {
      syslog(LOG_DEBUG, "passthru_thing_shadow_yield_thread: disconnecting thing clientId=%s", thing->clientId);
      if(passthru_thing_send_disconnect_report(thing)) {
        syslog(LOG_ERR, "passthru_thing_shadow_yield_thread: unable to send disconnect report!");
        sleep(1);
        continue;
      }
    }

    syslog(LOG_DEBUG, "passthru_thing_shadow_yield_thread: waiting for delta");
    sleep(1);
  }

  syslog(LOG_DEBUG, "passthru_thing_shadow_yield_thread: stopping");
  return NULL;
}

int passthru_thing_send_connect_report(passthru_thing *thing) {
  syslog(LOG_DEBUG, "passthru_thing_send_connect_report: thing->clientId=%s", thing->clientId);
  char pJsonDocument[SHADOW_MAX_SIZE_OF_RX_BUFFER];
  char state[255] = "{\"connected\": \"true\"}";
  if(!passthru_shadow_build_report_json(pJsonDocument, SHADOW_MAX_SIZE_OF_RX_BUFFER, state, strlen(state))) {
    syslog(LOG_ERR, "passthru_thing_send_connect_report: failed to build JSON state message. state=%s", state);
    return 1;
  }
  return passthru_shadow_update(thing->shadow, pJsonDocument);
}

int passthru_thing_send_disconnect_report(passthru_thing *thing) {
  syslog(LOG_DEBUG, "passthru_thing_send_disconnect_report: thing->clientId=%s", thing->clientId);
  char pJsonDocument[SHADOW_MAX_SIZE_OF_RX_BUFFER];
  char state[255] = "{\"connected\": \"false\"}";
  if(!passthru_shadow_build_report_json(pJsonDocument, SHADOW_MAX_SIZE_OF_RX_BUFFER, state, strlen(state))) {
    syslog(LOG_ERR, "passthru_thing_send_disconnect_report: failed to build JSON state message. state=%s", state);
    return 1;
  }
  return passthru_shadow_update(thing->shadow, pJsonDocument);
}

passthru_thing* passthru_thing_new() {

  passthru_thing *thing = malloc(sizeof(passthru_thing));
  memset(thing, 0, sizeof(passthru_thing));
  thing->clientId = "test";

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
  if(passthru_shadow_connect(thing->shadow) != 0) {
    syslog(LOG_CRIT, "passthru_thing_run: unable to connect to AWS IoT shadow service");
    return 1;
  }
  if(thing->shadow->rc != SUCCESS) {
    syslog(LOG_CRIT, "passthru_thing_run: unable to connect to AWS IoT shadow service");
    return 2;
  }
  thing->state = THING_STATE_CONNECTED;

syslog(LOG_DEBUG, "passthru_thing_run: thing->clientId=%s", thing->clientId);

  pthread_create(&thing->shadow->yield_thread, NULL, passthru_thing_shadow_yield_thread, thing);
  pthread_join(thing->shadow->yield_thread, NULL);
  syslog(LOG_DEBUG, "passthru_passthru_thing: run loop complete");
  return 0;
}

void passthru_thing_close(passthru_thing *thing) {
  syslog(LOG_DEBUG, "passthru_thing_close: closing clientId=%s", thing->clientId);
  thing->state = THING_STATE_DISCONNECTING;
  while(!(thing->state & THING_STATE_DISCONNECTING)) {
    syslog(LOG_DEBUG, "passthru_thing_close: waiting for thing connection to close");
    sleep(1);
  }
  syslog(LOG_DEBUG, "passthru_thing_close: closed");
}

void passthru_thing_destroy(passthru_thing *thing) {
  syslog(LOG_DEBUG, "passthru_thing_destroy");
  passthru_shadow_destroy(thing->shadow);
  free(thing->shadow);
  free(thing);
}
