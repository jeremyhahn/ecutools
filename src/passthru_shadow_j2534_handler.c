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

#include "passthru_shadow_j2534_handler.h"

void passthru_shadow_j2534_onmessage(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData) {

  syslog(LOG_DEBUG, "passthru_shadow_j2534_onmessage: topicName=%s, topicNameLen=%u, payload=%s, payload_len=%i", 
    topicName, (unsigned int)topicNameLen, params->payload, params->payloadLen);

  j2534_client *client = (j2534_client *)pData;

  char data[params->payloadLen];
  memcpy(data, params->payload, params->payloadLen);
  data[params->payloadLen] = '\0';

  syslog(LOG_DEBUG, "passthru_shadow_j2534_onmessage: json=%s", data);
}

void passthru_shadow_j2534_onerror(awsiot_client *awsiot, const char *message) {
  syslog(LOG_ERR, "passthru_shadow_j2534_onerror: message=%s", message);
}

j2534_client* passthru_shadow_j2534_handler_get_client(passthru_thing *thing, int *deviceId) {
  syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_get_client: deviceId=%d", deviceId);
  int i;
  for(i=0; i<thing->j2534->clients->size; i++) {
    j2534_client *client = (j2534_client *)vector_get(thing->j2534->clients, i);
    if(client == NULL) continue;
    if(client->deviceId == deviceId) {
      return client;
    }
  }
  return NULL;
}

unsigned int passthru_shadow_j2534_handler_delete_client(passthru_thing *thing, int *deviceId) {
  int i;
  for(i=0; i<thing->j2534->clients->size; i++) {
    j2534_client *client = (j2534_client *)vector_get(thing->j2534->clients, i);
    if(client == NULL) continue;
    if(client->deviceId == deviceId) {
      vector_delete(thing->j2534->clients, i);
      syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_delete_client: deleted client deviceId=%d", deviceId);
      return 0;
    }
  }
  syslog(LOG_ERR, "passthru_shadow_j2534_handler_delete_client: unable to delete client. deviceId=%s", deviceId);
  return 1;
}

void passthru_shadow_j2534_handler_send_error(passthru_thing *thing, unsigned int state, unsigned int error) {
  syslog(LOG_ERR, "passthru_shadow_j2534_handler_send_error: error=%x", error);
  unsigned int state_len = (state < 10) ? 1 : 2;
  unsigned int json_len = state_len + 33;
  char json[json_len+1];
  snprintf(json, json_len, "{\"j2534\":{\"state\":%i,\"error\":\"%x\"}}", state, error);
  json[json_len] = '\0';
  passthru_thing_send_report(json);
}

void passthru_shadow_j2534_handler_send_report(int state) {
  unsigned int state_len = (state < 10) ? 1 : 2;
  unsigned int json_len = state_len + 34;
  char json[json_len+1];
  snprintf(json, json_len, "{\"j2534\":{\"state\":%i,\"error\":null}}", state);
  json[json_len] = '\0';
  passthru_thing_send_report(json);
}

void passthru_shadow_j2534_handler_desired_open(passthru_thing *thing, shadow_j2534 *j2534) {

syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_desired_open: j2534->deviceId=%d", j2534->deviceId);

  j2534_client *client = passthru_shadow_j2534_handler_get_client(thing, j2534->deviceId);
  if(client != NULL) {
    syslog(LOG_ERR, "passthru_shadow_j2534_handler_desired_open: ERR_DEVICE_IN_USE");
    return passthru_shadow_j2534_handler_send_error(thing, J2534_PassThruOpen, ERR_DEVICE_IN_USE);
  }

  client = malloc(sizeof(j2534_client));
  client->state = J2534_PassThruOpen;
  client->deviceId = MYINT_DUP(j2534->deviceId);
  client->opened = true;

  unsigned int shadow_update_topic_len = PASSTHRU_SHADOW_UPDATE_TOPIC + strlen(client->name) + 1;
  unsigned int shadow_update_accepted_topic_len = PASSTHRU_SHADOW_UPDATE_ACCEPTED_TOPIC + strlen(client->name) + 1;
  unsigned int shadow_error_topic_len = J2534_ERROR_TOPIC + strlen(client->name) + 1;
  unsigned int msg_rx_topic_len = J2534_MSG_RX_TOPIC + strlen(client->name) + 1;
  unsigned int msg_tx_topic_len = J2534_MSG_TX_TOPIC + strlen(client->name) + 1;

  client->shadow_update_topic = malloc(sizeof(char) * shadow_update_topic_len);
  client->shadow_update_accepted_topic = malloc(sizeof(char) * shadow_update_accepted_topic_len);
  client->shadow_error_topic = malloc(sizeof(char) * shadow_error_topic_len);
  client->msg_tx_topic = malloc(sizeof(char) * msg_tx_topic_len);
  client->msg_rx_topic = malloc(sizeof(char) * msg_rx_topic_len);

  snprintf(client->shadow_update_topic, shadow_update_topic_len, PASSTHRU_SHADOW_UPDATE_TOPIC, client->name);
  snprintf(client->shadow_update_accepted_topic, shadow_update_accepted_topic_len, PASSTHRU_SHADOW_UPDATE_ACCEPTED_TOPIC, client->name);
  snprintf(client->shadow_error_topic, shadow_error_topic_len, J2534_ERROR_TOPIC, client->name);
  snprintf(client->msg_rx_topic, msg_rx_topic_len, J2534_MSG_RX_TOPIC, client->name);
  snprintf(client->msg_tx_topic, msg_tx_topic_len, J2534_MSG_TX_TOPIC, client->name);

  client->awsiot = malloc(sizeof(awsiot_client));
  client->awsiot->client = malloc(sizeof(AWS_IoT_Client));
  client->awsiot->certDir = PASSTHRU_CERT_DIR;
  client->awsiot->onopen = NULL;
  client->awsiot->onclose = NULL;
  client->awsiot->ondisconnect = NULL;
  client->awsiot->onmessage = &passthru_shadow_j2534_onmessage;
  client->awsiot->onerror = &passthru_shadow_j2534_onerror;

  vector_add(thing->j2534->clients, client);
  passthru_shadow_j2534_handler_send_report(J2534_PassThruOpen);
}

void passthru_shadow_j2534_handler_desired_close(passthru_thing *thing, shadow_j2534 *j2534) {

  syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_desired_close: j2534->deviceId=%d", j2534->deviceId);

  j2534_client *client = passthru_shadow_j2534_handler_get_client(thing, j2534->deviceId);

  if(client == NULL) {
    syslog(LOG_ERR, "passthru_shadow_j2534_handler_desired_close: unable to locate client. j2534->deviceId=%s", j2534->deviceId);
    return passthru_shadow_j2534_handler_send_error(thing, J2534_PassThruConnect, ERR_INVALID_DEVICE_ID);
  }

  awsiot_client_close(client->awsiot);

  passthru_shadow_j2534_handler_delete_client(thing, j2534->deviceId);
  passthru_shadow_j2534_handler_send_report(J2534_PassThruClose);

  free(client->shadow_update_topic);
  free(client->shadow_update_accepted_topic);
  free(client->shadow_error_topic);
  free(client->msg_tx_topic);
  free(client->msg_rx_topic);
  free(client->name);
  free(client);
}

void passthru_shadow_j2534_handler_desired_connect(passthru_thing *thing, shadow_j2534 *j2534) {

  j2534_client *client = passthru_shadow_j2534_handler_get_client(thing, j2534->deviceId);

  client->state = J2534_PassThruConnect;

  if(awsiot_client_connect(client->awsiot) != 0) {
    syslog(LOG_ERR, "passthru_shadow_j2534_handler_desired_connect: failed to connect. rc=%d", client->awsiot->rc);
    return passthru_shadow_j2534_handler_send_error(thing, J2534_PassThruConnect, ERR_DEVICE_NOT_CONNECTED);
  }

  client->canbus = malloc(sizeof(canbus_client));
  client->canbus->iface = thing->params->iface;

  canbus_init(client->canbus);
  if(canbus_connect(client->canbus) == 0) {
    return passthru_shadow_j2534_handler_send_report(J2534_PassThruConnect);
  }

  syslog(LOG_ERR, "passthru_shadow_j2534_handler_desired_connect: Failed to establish CAN connection");
}

void passthru_shadow_j2534_handler_desired_disconnect(passthru_thing *thing, shadow_j2534 *j2534) {

  syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_desired_disconnect: disconnecting");

  j2534_client *client = passthru_shadow_j2534_handler_get_client(thing, j2534->deviceId);
  if(client == NULL) {
    syslog(LOG_ERR, "passthru_shadow_j2534_handler_desired_disconnect: ERR_DEVICE_NOT_OPEN");
    return passthru_shadow_j2534_handler_send_error(thing, J2534_PassThruOpen, ERR_DEVICE_NOT_CONNECTED);
  }

  if(client == NULL) {
    syslog(LOG_ERR, "passthru_shadow_j2534_handler_desired_disconnect: unable to locate client. j2534->deviceId=%s", j2534->deviceId);
    return passthru_shadow_j2534_handler_send_error(thing, J2534_PassThruDisconnect, ERR_DEVICE_NOT_CONNECTED);
  }

  canbus_close(client->canbus);
  canbus_free(client->canbus);
  client->canbus = NULL;

  passthru_shadow_j2534_handler_send_report(J2534_PassThruDisconnect);
}

void passthru_shadow_j2534_handler_desired_select(passthru_thing *thing, shadow_j2534 *j2534) {

  j2534_client *client = passthru_shadow_j2534_handler_get_client(thing, j2534->deviceId);
  if(client == NULL) {
    syslog(LOG_ERR, "passthru_shadow_j2534_handler_get_client: unable to locate client: %s", j2534->deviceId);
    return passthru_shadow_j2534_handler_send_error(thing, J2534_PassThruSelect, ERR_DEVICE_NOT_CONNECTED);
  }

  client->state = J2534_PassThruSelect;
  passthru_shadow_j2534_handler_send_report(client->state);
}

void passthru_shadow_j2534_handler_desired_startMsgFilter(passthru_thing *thing, shadow_j2534 *j2534) {

  int i;

  j2534_client *client = passthru_shadow_j2534_handler_get_client(thing, j2534->deviceId);

  if(client == NULL) {
    return passthru_shadow_j2534_handler_send_error(thing, J2534_PassThruSelect, ERR_DEVICE_NOT_CONNECTED);
  }

  if(!client->opened) {
    return passthru_shadow_j2534_handler_send_error(thing, J2534_PassThruStartMsgFilter, ERR_DEVICE_NOT_OPEN);
  }

  client->state = J2534_PassThruStartMsgFilter;

  syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_desired_startMsgFilter: j2534->filters->count=%i", j2534->filters->count);

  struct can_filter *filters = malloc(sizeof(struct can_filter) * j2534->filters->count);
  for(i=0; i<j2534->filters->count; i++) {

    shadow_j2534_filter *shadow_filter = vector_get(j2534->filters, i);
    filters[i].can_id = shadow_filter->can_id;
    filters[i].can_mask = shadow_filter->can_mask;
  }

  if(j2534->filters->count) {
    canbus_filter(client->canbus, filters, j2534->filters->count);
  }

  passthru_shadow_j2534_handler_send_report(J2534_PassThruStartMsgFilter);
}

void passthru_shadow_j2534_handler_handle_desired_state(passthru_thing *thing, shadow_j2534 *j2534) {

  if(j2534->error != 0) return; // prevent endless message loop; fix!

  syslog(LOG_ERR, "passthru_shadow_j2534_handler_handle_desired_state: routing state: %d", j2534->state);

  if(j2534->state == J2534_PassThruOpen) {
    return passthru_shadow_j2534_handler_desired_open(thing, j2534);
  }

  if(j2534->state == J2534_PassThruClose) {
    return passthru_shadow_j2534_handler_desired_close(thing, j2534);
  }

  if(j2534->state == J2534_PassThruConnect) {
    return passthru_shadow_j2534_handler_desired_connect(thing, j2534);
  }

  if(j2534->state == J2534_PassThruDisconnect) {
    return passthru_shadow_j2534_handler_desired_disconnect(thing, j2534);
  }

  if(j2534->state == J2534_PassThruSelect) {
    return passthru_shadow_j2534_handler_desired_select(thing, j2534);
  }

  if(j2534->state == J2534_PassThruStartMsgFilter) {
    return passthru_shadow_j2534_handler_desired_startMsgFilter(thing, j2534);
  }

  syslog(LOG_ERR, "passthru_shadow_j2534_handler_handle_desired_state: invalid state: %d", j2534->state);
}

void passthru_shadow_j2534_handler_handle_delta(passthru_thing *thing, shadow_j2534 *j2534) {
  passthru_shadow_j2534_handler_handle_desired_state(thing, j2534);
}

void passthru_shadow_j2534_handler_handle_state(passthru_thing *thing, shadow_state *state) {

  if(state->reported->j2534->state) {
    syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_handle: REPORT");
  }

  if(state->desired->j2534->state) {
    syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_handle: DESIRED");
    passthru_shadow_j2534_handler_handle_desired_state(thing, state->desired->j2534);
  }

}
