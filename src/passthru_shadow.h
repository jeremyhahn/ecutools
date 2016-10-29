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

#ifndef PASSTHRUSHADOW_H_
#define PASSTHRUSHADOW_H_

#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include <errno.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include "vector.h"
#include "aws_iot_src/include/aws_iot_log.h"
#include "aws_iot_src/include/aws_iot_version.h"
#include "aws_iot_src/include/aws_iot_mqtt_client_interface.h"
#include "aws_iot_src/include/aws_iot_shadow_interface.h"
#include "aws_iot_config.h"

#define PASSTHRU_SHADOW_UPDATE_TOPIC          "$aws/things/%s/shadow/update"
#define PASSTHRU_SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/%s/shadow/update/accepted"
#define PASSTHRU_SHADOW_GET_TOPIC             "$aws/things/%s/shadow/get"
#define PASSTHRU_SHADOW_GET_ACCEPTED_TOPIC    "$aws/things/%s/shadow/get/accepted"

char DELTA_REPORT[SHADOW_MAX_SIZE_OF_RX_BUFFER];

typedef struct {
  int *type;
  char *file;
} shadow_log;

typedef struct {
  canid_t can_id;
  canid_t can_mask;
} shadow_j2534_filter;

typedef struct {
  int *deviceId;
  int *state;
  int *error;
  char *data;
  vector *filters;
} shadow_j2534;

typedef struct {
  char *connection;
  shadow_j2534 *j2534;
  shadow_log *log;
} shadow_report;

typedef struct {
  shadow_report *reported;
} shadow_metadata;

typedef struct {
  int *connection;
  shadow_j2534 *j2534;
  shadow_log *log;
} shadow_desired;

typedef struct {
  shadow_report *reported;
  shadow_desired *desired;
} shadow_state;

typedef struct {
  shadow_state *state;
  shadow_metadata *metadata;
  uint64_t version;
  uint64_t timestamp;
  char *clientToken;
} shadow_message;

typedef struct _passthru_shadow {
  char *thingName;
  char *certDir;
  IoT_Error_t rc;
  AWS_IoT_Client *mqttClient;
  pthread_t yield_thread;
  pthread_mutex_t shadow_update_lock;
  char *get_topic;
  char *get_accepted_topic;
  char *update_topic;
  char *update_accepted_topic;
  void (*onopen)(struct _passthru_shadow *);
  void (*ondelta)(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t);
  void (*onupdate)(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status, const char *pReceivedJsonDocument, void *pContextData);
  void (*onget)(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t);
  void (*ondisconnect)(void);
  void (*onerror)(struct _passthru_shadow *, const char *message);
} passthru_shadow;

int passthru_shadow_connect(passthru_shadow *shadow);
bool passthru_shadow_build_report_json(char *pJsonDocument, size_t maxSizeOfJsonDocument, const char *pReceivedDeltaData, uint32_t lengthDelta);
int passthru_shadow_report_delta(passthru_shadow *shadow);
void passthru_shadow_get(passthru_shadow *shadow);
int passthru_shadow_update(passthru_shadow *shadow, char *message, void *pContextData);
int passthru_shadow_disconnect(passthru_shadow *shadow);

#endif
