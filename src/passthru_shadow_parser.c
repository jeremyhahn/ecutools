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
  message->state->reported->log = malloc(sizeof(shadow_log));
  message->state->reported->log->type = 0;
  message->state->reported->log->file = NULL;
  message->state->reported->j2534 = malloc(sizeof(shadow_j2534));
  message->state->reported->j2534->state = 0;
  message->state->reported->j2534->error = 0;
  message->state->reported->j2534->data = NULL;
  message->state->reported->j2534->deviceId = NULL;
  message->state->reported->connection = NULL;

  message->state->desired = malloc(sizeof(shadow_desired));
  message->state->desired->log = malloc(sizeof(shadow_log));
  message->state->desired->log->type = 0;
  message->state->desired->log->file = NULL;
  message->state->desired->j2534 = malloc(sizeof(shadow_j2534));
  message->state->desired->j2534->state = 0;
  message->state->desired->j2534->error = 0;
  message->state->desired->j2534->data = NULL;
  message->state->desired->j2534->deviceId = NULL;
  message->state->desired->j2534->filters = malloc(sizeof(vector));
  vector_init(message->state->desired->j2534->filters);
  message->state->desired->connection = NULL;

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
  desired->j2534 = malloc(sizeof(shadow_j2534));
  desired->j2534->deviceId = NULL;
  desired->j2534->state = NULL;
  desired->j2534->error = NULL;
  desired->j2534->data = NULL;
  desired->j2534->filters = malloc(sizeof(vector));
  vector_init(desired->j2534->filters);
  desired->connection = NULL;

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

  json_t *j2534 = json_object_get(root, "j2534");
  if(json_is_object(j2534)) {

    json_t *state = json_object_get(j2534, "state");
    json_t *error = json_object_get(j2534, "error");
    json_t *data = json_object_get(j2534, "data");
    json_t *deviceId = json_object_get(j2534, "deviceId");
    json_t *filters = json_object_get(j2534, "filters");
    desired->j2534->state = json_integer_value(state);
    desired->j2534->error = json_string_value(error);
    desired->j2534->data = json_string_value(data);
    desired->j2534->deviceId = json_integer_value(deviceId);

    if(!json_is_array(filters)) {
      syslog(LOG_ERR, "passthru_shadow_parser_parse_delta: J2534 filters is not an array");
      return desired;
    }

    long j2534_filter_count = (unsigned long) json_array_size(filters);

    int i;
    for(i=0; i<j2534_filter_count; i++) {

      json_t *filter, *filterId, *filterMask;
      filter = json_array_get(filters, i);

      if(!json_is_object(filter)) {
        syslog(LOG_ERR, "passthru_shadow_parser_parse_delta: J2534 filter element is not an object");
        return desired;
      }

      filterId = json_object_get(filter, "id");
      if(!json_is_string(filterId)) {
        syslog(LOG_ERR, "passthru_shadow_parser_parse_delta: filter id is not a string");
        return desired;
      }

      filterMask = json_object_get(filter, "mask");
      if(!json_is_string(filterMask)) {
        syslog(LOG_ERR, "passthru_shadow_parser_parse_delta: filter mask is not a string");
        return desired;
      }

      shadow_j2534_filter *j2534_filter = malloc(sizeof(shadow_j2534_filter));
      j2534_filter->can_id = strtoul(json_string_value(filterId), NULL, 16);
      j2534_filter->can_mask = strtoul(json_string_value(filterMask), NULL, 16);
      vector_add(desired->j2534->filters, j2534_filter);
    }
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

    if(strncmp(key, "log", strlen(key)) == 0) {
      json_t *type = json_object_get(value, "type");
      json_t *file = json_object_get(value, "file");
      message->state->reported->log->type = json_integer_value(type);
      message->state->reported->log->file = json_integer_value(file);
    }

    if(strncmp(key, "j2534", strlen(key)) == 0) {
      json_t *state = json_object_get(value, "state");
      json_t *error = json_object_get(value, "error");
      json_t *data = json_object_get(value, "data");
      json_t *deviceId = json_object_get(value, "deviceId");
      message->state->reported->j2534->state = json_integer_value(state);
      message->state->reported->j2534->error = json_integer_value(error);
      message->state->reported->j2534->data = json_string_value(data);
      message->state->reported->j2534->deviceId = json_integer_value(deviceId);
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

    if(strncmp(key, "log", strlen(key)) == 0) {
      json_t *type = json_object_get(value, "type");
      json_t *file = json_object_get(value, "file");
      message->state->desired->log->type = json_integer_value(type);
      message->state->desired->log->file = json_integer_value(file);
    }

    if(strncmp(key, "j2534", strlen(key)) == 0) {
      json_t *state = json_object_get(value, "state");
      json_t *error = json_object_get(value, "error");
      json_t *data = json_object_get(value, "data");
      json_t *deviceId = json_object_get(value, "deviceId");
      message->state->desired->j2534->state = json_integer_value(state);
      message->state->desired->j2534->error = json_integer_value(error);
      message->state->desired->j2534->data = json_string_value(data);
      message->state->desired->j2534->deviceId = json_integer_value(deviceId);
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
    if(desired->j2534->filters != NULL) {
      int i;
      for(i=0; i<desired->j2534->filters->count; i++) {
        free(vector_get(desired->j2534->filters, i));
      }
      vector_free(desired->j2534->filters);
      desired->j2534->filters = NULL;
    }
    free(desired->j2534);
    desired->j2534 = NULL;
  }
  if(desired->connection) {
    desired->connection = NULL;
  }
  free(desired);
  desired = NULL;
}

void passthru_shadow_parser_free_reported(shadow_report *reported) {
  if(reported == NULL) return;
  if(reported->log) {
    free(reported->log);
    reported->log = NULL;
  }
  if(reported->j2534 != NULL) {
    free(reported->j2534);
    reported->j2534 = NULL;
  }
  if(reported->connection) {
    reported->connection = NULL;
  }
  free(reported);
  reported = NULL;
}

void passthru_shadow_parser_free_message(shadow_message *message) {

  passthru_shadow_parser_free_reported(message->state->reported);
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
