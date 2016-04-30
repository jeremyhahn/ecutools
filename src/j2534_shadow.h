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

#ifndef J2534SHADOW_H_
#define J2534SHADOW_H_

#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include "aws_iot_src/utils/aws_iot_error.h"
#include "aws_iot_src/utils/aws_iot_log.h"
#include "aws_iot_src/utils/aws_iot_version.h"
#include "aws_iot_src/protocol/mqtt/aws_iot_mqtt_interface.h"
#include "aws_iot_config.h"

typedef struct _j2534_shadow {
  MQTTSubscribeParams *subscribeParams;
  MQTTPublishParams *publishParams;
  IoT_Error_t rc;
  pthread_t update_accepted_thread;
  pthread_t update
  pthread_t canbus_subscribe_thread;
  pthread_mutex_t publish_lock;
  pthread_mutex_t subscribe_lock;
  void (*onopen)(struct _awsiot_client *);
  void (*onmessage)(MQTTCallbackParams params);
  void (*ondisconnect)(void);
  void (*onclose)(struct _awsiot_client *, const char *message);
  void (*onerror)(struct _awsiot_client *, const char *message);
} j2534_shadow;


void j2534_shadow_init();

void j2534_shadow_update(j2534_shadow *shadow);
void j2534_shadow_update_accepted(j2534_shadow *shadow);
void j2534_shadow_update_rejected(j2534_shadow *shadow);
void j2534_shadow_update_delta(j2534_shadow *shadow);

void j2534_shadow_get(j2534_shadow *shadow);
void j2534_shadow_get_accepted(j2534_shadow *shadow);
void j2534_shadow_get_rejected(j2534_shadow *shadow);

void j2534_shadow_delete(j2534_shadow *shadow);

#endif