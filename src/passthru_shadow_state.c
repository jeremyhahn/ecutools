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

#include "passthru_shadow_state.h"

static FILE *passthru_shadow_state;
static bool passthru_shadow_state_awsiot_syncing = false;

void passthru_shadow_state_restore(passthru_thing *thing, shadow_message *message) {

  if(message == NULL || message->state == NULL) return;

  if(message->state->reported->log->type) {
    return passthru_shadow_log_handler_handle(thing, message->state->reported->log);
  }
}

void passthru_shadow_state_read_awsiot_state_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, IoT_Publish_Message_Params *params, void *pData) {
 
  syslog(LOG_DEBUG, "passthru_shadow_state_read_awsiot_state_handler: topicName=%s, topicNameLen=%u, pData=%s", topicName, (unsigned int)topicNameLen, (char *)pData);

  passthru_thing *thing = (passthru_thing *)pData;

  char json[params->payloadLen];
  memcpy(json, params->payload, params->payloadLen);
  json[params->payloadLen] = '\0';

  passthru_shadow_state_write(json);

  shadow_message *message = passthru_shadow_parser_parse_state(json);

  // clear connection=2 to prevent connection handler from disconnecting
  if(message->state->reported->connection) message->state->reported->connection = NULL;

  passthru_shadow_state_restore(thing, message);

  /*
  // clear j2534 state
  if(message->state->desired->j2534->state) {
    const char json = "{\"state\":{\"desired\":{\"j2534\":null}},{\"reported\":{\"j2534\":null}}}";
    if(awsiot_client_publish(thing->awsiot, thing->shadow->update_topic, json) != 0) {
      syslog(LOG_DEBUG, "passthru_shadow_state_read_awsiot_state_handler: unable to clear j2534 state. rc=%d", thing->awsiot->rc);
    }
  }*/

  passthru_shadow_parser_free_message(message);

  passthru_shadow_state_awsiot_syncing = false;
}

void passthru_shadow_state_sync_awsiot_state(passthru_thing *thing) {
  thing->awsiot = malloc(sizeof(awsiot_client));
  thing->awsiot->client = malloc(sizeof(AWS_IoT_Client));
  thing->awsiot->onopen = NULL;
  thing->awsiot->onerror = NULL;
  thing->awsiot->onclose = NULL;
  thing->awsiot->ondisconnect = NULL;
  thing->awsiot->certDir = thing->params->certDir;
  awsiot_client_connect(thing->awsiot);
  awsiot_client_subscribe(thing->awsiot, thing->shadow->get_accepted_topic, passthru_shadow_state_read_awsiot_state_handler, thing);
  unsigned int i = 0;
  while(passthru_shadow_state_awsiot_syncing) {

    if(i == 3) break;

    aws_iot_mqtt_yield(thing->awsiot->client, 200);
    if(thing->awsiot->rc == NETWORK_ATTEMPTING_RECONNECT) {
      syslog(LOG_DEBUG, "passthru_shadow_state_sync_awsiot_state: waiting for network to reconnect");
      sleep(1);
      continue;
    }

    sleep(1);
    i++;
  }

  awsiot_client_close(thing->awsiot);
  free(thing->awsiot->client);
  free(thing->awsiot);
}

void passthru_shadow_state_sync(passthru_thing *thing) {

  shadow_message *message = NULL;
  ssize_t bytes = 0;
  size_t len = 0;
  char *line = NULL;

  if(passthru_shadow_state_open(thing->params->cacheDir, "r") == 0) {
    bytes = getline(&line, &len, passthru_shadow_state);
  }

  if(bytes) {
    syslog(LOG_DEBUG, "passthru_shadow_state_sync: syncing with local cache");
    message = passthru_shadow_parser_parse_state(line);
    // clear connection=2 to prevent connection handler from disconnecting
    if(message->state->reported->connection) message->state->reported->connection = NULL;
    passthru_shadow_state_restore(thing, message);
    passthru_shadow_parser_free_message(message);
  }
  else {
    syslog(LOG_DEBUG, "passthru_shadow_state_sync: syncing with AWS IoT shadow");
    passthru_shadow_state_awsiot_syncing = true;
    passthru_shadow_state_sync_awsiot_state(thing);
  }

  passthru_shadow_state_close();
}

unsigned int passthru_shadow_state_open(const char *cacheDir, const char *mode) {

  syslog(LOG_DEBUG, "passthru_shadow_state_open: cacheDir=%s, mode=%s", cacheDir, mode);

  if(passthru_shadow_state != NULL) {
    syslog(LOG_ERR, "passthru_shadow_state_open: already opened");
    return 1;
  }

  char filename[360] = "";
  if(cacheDir == NULL) {
    strcpy(filename, PASSTHRU_CACHE_DIR);
  }
  else {
    strcpy(filename, cacheDir);
    if(filename[strlen(filename)] != '/') {
      strcat(filename, "/");
    }
  }
  strcat(filename, "state_log");

  passthru_shadow_state = fopen(filename, mode);
  if(passthru_shadow_state == NULL) {
    syslog(LOG_ERR, "passthru_shadow_state_open: Unable to open %s. error=%s", filename, strerror(errno));
    passthru_shadow_state = NULL;
    return errno;
  }

  return 0;
}

unsigned passthru_shadow_state_write(char *data) {
  if(passthru_shadow_state == NULL) return;
  if(strlen(data) > 1000) {
    syslog(LOG_ERR, "passthru_shadow_state_write: data must not be larger than 1000 chars");
    return 1;
  }
  unsigned int len = strlen(data);
  char d[len+2];
  strncpy(d, data, len+1);
  syslog(LOG_DEBUG, "passthru_shadow_state_write: %s", d);
  return fprintf(passthru_shadow_state, d);
}

void passthru_shadow_state_close() {
  if(passthru_shadow_state != NULL) {
    fclose(passthru_shadow_state);
    passthru_shadow_state = NULL;
    syslog(LOG_DEBUG, "passthru_shadow_state_close: cache closed");
  }
}
