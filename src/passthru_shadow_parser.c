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

void passthru_shadow_parser_parse_reported(json_t *obj, shadow_message *message);
void passthru_shadow_parser_parse_desired(json_t *obj, shadow_message *message);

shadow_message* passthru_shadow_parser_parse_state(const char *json) {
 
  json_t *root;
  json_error_t error;

  shadow_message *message = malloc(sizeof(shadow_message));
  message->state = malloc(sizeof(shadow_state));

  message->state->reported = malloc(sizeof(shadow_report));
  message->state->reported->log = malloc(sizeof(shadow_log));;
  message->state->reported->log->type = NULL;
  message->state->reported->log->file = NULL;
  message->state->reported->connection = NULL;
  message->state->reported->j2534 = NULL;

  message->state->desired = malloc(sizeof(shadow_desired));
  message->state->desired->log = malloc(sizeof(shadow_log));;
  message->state->desired->log->type = NULL;
  message->state->desired->log->file = NULL;
  message->state->desired->connection = NULL;
  message->state->desired->j2534 = NULL;

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
    passthru_shadow_parser_parse_reported(reported, message);
  }

  json_t *desired = json_object_get(state, "desired");
  if(json_is_object(desired)) {
    passthru_shadow_parser_parse_desired(desired, message);
  }

  return message;
}

shadow_desired* passthru_shadow_parser_parse_delta(const char *json) {
 
  syslog(LOG_DEBUG, "passthru_shadow_parser_parse_delta: json=%s", json);

  json_t *root;
  json_error_t error;

  shadow_desired *desired = malloc(sizeof(shadow_desired));
  desired->log = malloc(sizeof(shadow_log));;
  desired->log->type = NULL;
  desired->log->file = NULL;
  desired->connection = NULL;
  desired->j2534 = NULL;

  root = json_loads(json, 0, &error);

  if(!root) {
    syslog(LOG_ERR, "passthru_shadow_parser_parse_delta: unable to parse root node. line=%i, source=%s, text=%s", error.line, error.source, error.text);
    return desired;
  }

  if(!json_is_object(root)) {
    syslog(LOG_ERR, "passthru_shadow_parser_parse_delta: Expected JSON root to be an object.");
    json_decref(root);
    return desired;
  }

  json_t *jslog = json_object_get(root, "log");
  if(json_is_object(jslog)) {
    json_t *type = json_object_get(jslog, "type");
    json_t *file = json_object_get(jslog, "file");
    desired->log->type = json_integer_value(type);
    desired->log->file = json_string_value(file);
  }

  return desired;
}

void passthru_shadow_parser_parse_reported(json_t *obj, shadow_message *message) {

  size_t obj_len = json_object_size(obj);
  if(obj_len > PASSTHRU_SHADOW_REPORTED_MAX_ELEMENTS) {
    syslog(LOG_ERR, "passthru_shadow_parser_parse_reported: payload too large. len=%zu, PASSTHRU_SHADOW_REPORTED_MAX_ELEMENTS=%d", obj_len, PASSTHRU_SHADOW_REPORTED_MAX_ELEMENTS);
    return NULL;
  }

  const char *key;
  json_t *element, *value;

  json_object_foreach(obj, key, value) {

    if(strncmp(key, "connection", strlen(key)) == 0) {
      message->state->reported->connection = json_integer_value(value);
    }

    if(strncmp(key, "j2534", strlen(key)) == 0) {
      message->state->reported->j2534 = json_integer_value(value);
    }

    if(strncmp(key, "log", strlen(key)) == 0) {
      json_t *type = json_object_get(value, "type");
      json_t *file = json_object_get(value, "file");
      message->state->reported->log->type = json_integer_value(type);
      message->state->reported->log->file = json_string_value(file);
    }

  }
}

void passthru_shadow_parser_parse_desired(json_t *obj, shadow_message *message) {

  size_t obj_len = json_object_size(obj);
  if(obj_len > PASSTHRU_SHADOW_REPORTED_MAX_ELEMENTS) {
    syslog(LOG_ERR, "passthru_shadow_parser_parse_desired: payload too large. len=%zu, PASSTHRU_SHADOW_REPORTED_MAX_ELEMENTS=%d", obj_len, PASSTHRU_SHADOW_REPORTED_MAX_ELEMENTS);
    return NULL;
  }

  const char *key;
  json_t *element, *value;

  json_object_foreach(obj, key, value) {

    if(strncmp(key, "connection", strlen(key)) == 0) {
      message->state->desired->connection = json_integer_value(value);
    }

    if(strncmp(key, "j2534", strlen(key)) == 0) {
      message->state->desired->j2534 = json_integer_value(value);
    }

    if(strncmp(key, "log", strlen(key)) == 0) {
      json_t *type = json_object_get(value, "type");
      json_t *file = json_object_get(value, "file");
      message->state->desired->log->type = json_integer_value(type);
      message->state->desired->log->file = json_string_value(file);
    }
  }
}

void passthru_shadow_parser_free_desired(shadow_desired *desired) {
  if(desired == NULL) return;
  if(desired->log) {
    free(desired->log);
    desired->log = NULL;
  }
  if(desired->j2534 != NULL) {
    desired->j2534 = NULL;
  }
  if(desired->connection) {
    desired->connection = NULL;
  }
  free(desired);
  desired = NULL;
}

void passthru_shadow_parser_free_message(shadow_message *message) {

  if(message->state->reported->log) {
    free(message->state->reported->log);
    message->state->reported->log = NULL;
  }
  if(message->state->reported->j2534) {
    message->state->reported->j2534 = NULL;
  }
  if(message->state->reported->connection) {
    message->state->reported->connection = NULL;
  }
  if(message->state->reported) {
    free(message->state->reported);
    message->state->reported = NULL;
  }

  passthru_shadow_parser_free_desired(message->state->desired);

  if(message->state != NULL) {
    free(message->state);
    message->state = NULL;
  }

  if(message != NULL) {
    free(message);
    message = NULL;
  }
}
