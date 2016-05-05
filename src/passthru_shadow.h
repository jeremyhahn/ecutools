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

#ifndef PASSTHRUSHADOW_H_
#define PASSTHRUSHADOW_H_

#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include "aws_iot_src/utils/aws_iot_log.h"
#include "aws_iot_src/utils/aws_iot_version.h"
#include "aws_iot_src/protocol/mqtt/aws_iot_mqtt_interface.h"
#include "aws_iot_src/shadow/aws_iot_shadow_interface.h"
#include "aws_iot_config.h"

char DELTA_REPORT[SHADOW_MAX_SIZE_OF_RX_BUFFER];
bool messageArrivedOnDelta;

typedef struct _passthru_shadow {
  char *clientId;
  IoT_Error_t rc;
  MQTTClient_t *mqttClient;
  pthread_t yield_thread;
  void (*onopen)(struct _passthru_shadow *);
  void (*ondelta)(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t);
  void (*onupdate)(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status, const char *pReceivedJsonDocument, void *pContextData);
  void (*onget)(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t);
  void (*ondisconnect)(void);
  void (*onerror)(struct _passthru_shadow *, const char *message);
} passthru_shadow;

void passthru_shadow_connect(passthru_shadow *shadow);
bool passthru_shadow_build_report_json(char *pJsonDocument, size_t maxSizeOfJsonDocument, const char *pReceivedDeltaData, uint32_t lengthDelta);
void passthru_shadow_report_delta(passthru_shadow *shadow);
void passthru_shadow_get(passthru_shadow *shadow);
void passthru_shadow_update(passthru_shadow *shadow, char *message);
void passthru_shadow_disconnect(passthru_shadow *shadow);

#endif