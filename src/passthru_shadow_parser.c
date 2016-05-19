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

#include "passthru_shadow_parser.h"

shadow_message* passthru_shadow_parser_parse_reported(json_t *obj, shadow_message *message);

shadow_message* passthru_shadow_parser_parse(const char *json) {
  
  json_t *root;
  json_error_t error;
  
  shadow_message *message = malloc(sizeof(shadow_message));

  root = json_loads(json, 0, &error);
  if(!root) {
    syslog(LOG_ERR, "passthru_shadow_parser_parse: unable to parse root node. line=%i, source=%s, text=%s", error.line, error.source, error.text);
    return message;
  }

  if(!json_is_object(root)) {
    syslog(LOG_ERR, "passthru_shadow_parser_parse: Expected JSON root to be an object.");
    json_decref(root);
    return message;
  }

  json_t *state = json_object_get(root, "state");
  if(!json_is_object(state)) {
    syslog(LOG_ERR, "passthru_shadow_parser_parse: JSON 'state' element is not an object.");
    json_decref(root);
    return message;
  }

  json_t *reported = json_object_get(state, "reported");
  if(json_is_object(reported)) {
    return passthru_shadow_parser_parse_reported(reported, message);
  }

  syslog(LOG_ERR, "passthru_shadow_parser_parse: unable to parse state from json %s", json);
  return message;
}

shadow_message* passthru_shadow_parser_parse_reported(json_t *obj, shadow_message *message) {

  size_t obj_len = json_object_size(obj);
  if(obj_len > PASSTHRU_SHADOW_REPORTED_MAX_ELEMENTS) {
    syslog(LOG_ERR, "passthru_shadow_parser_parse_reported: payload too large. len=%zu, PASSTHRU_SHADOW_REPORTED_MAX_ELEMENTS=%d", obj_len, PASSTHRU_SHADOW_REPORTED_MAX_ELEMENTS);
    return NULL;
  }

  const char *key;
  json_t *element, *value;

  message->state = malloc(sizeof(shadow_state));
  message->state->reported = malloc(sizeof(shadow_report));
  message->state->reported->connected = NULL;
  message->state->reported->log = NULL;

  json_object_foreach(obj, key, value) {

    if(strncmp(key, "connected", strlen(key)) == 0) {
      const char *json_val = json_string_value(value);
      message->state->reported->connected = malloc(strlen(json_val));
      strcpy(message->state->reported->connected, json_val);
    }

    if(strncmp(key, "log", strlen(key)) == 0) {

      message->state->reported->log = malloc(sizeof(shadow_log));
      message->state->reported->log->type = NULL;
      message->state->reported->log->file = NULL;

      json_t *file = json_object_get(value, "file");
      json_t *type = json_object_get(value, "type");

      if(json_is_string(file)) {
        const char *file_val = json_string_value(file);
        message->state->reported->log->file = malloc(strlen(file_val)+1);
        strcpy(message->state->reported->log->file, file_val);
      }

      if(json_is_integer(type)) {
        message->state->reported->log->type = malloc(sizeof(int));
        message->state->reported->log->type = json_integer_value(type);
      }
    }

  }

  return message;
}

void passthru_shadow_parser_free_message(shadow_message *message) {

  if(message->state->reported->log != NULL) {

    if(message->state->reported->log->file != NULL) {
      free(message->state->reported->log->file);
      message->state->reported->log->file = NULL;
    }

    free(message->state->reported->log);
    message->state->reported->log = NULL;
  }

  if(message->state->reported->connected != NULL) {
    free(message->state->reported->connected);
    message->state->reported->connected = NULL;
  }

  if(message->state->reported != NULL) {
    free(message->state->reported);
    message->state->reported = NULL;
  }

  if(message->state != NULL) {
    free(message->state);
    message->state = NULL;
  }

  if(message != NULL) {
    free(message);
    message = NULL;
  }
}