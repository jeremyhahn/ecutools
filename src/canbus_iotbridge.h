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

#ifndef IOTBRIDGE_H_
#define IOTBRIDGE_H_

#include <stddef.h>
#include <signal.h>
#include <pthread.h>
#include "canbus.h"
#include "awsiot_client.h"

typedef struct _iotbridge {
  struct iotbridge_filter *filters[10];
  pthread_t canbus_thread;
  pthread_t publish_thread;
  awsiot_client *awsiot;
  canbus_client *canbus;
  uint8_t canbus_flags;
} iotbridge;

typedef struct {
  awsiot_client *awsiot;
  char *payload;
} iotbridge_publish_thread_args;

iotbridge *bridge;
iotbridge *wcbridge_new();
int iotbridge_run(iotbridge *iotbridge);
void iotbridge_close(iotbridge *iotbridge, const char *message);
void iotridge_destroy(iotbridge *iotbridge);
void iotbridge_process_filter(iotbridge *bridge, struct can_frame *frame);

void *iotbridge_canbus_logger_thread(void *ptr);
void *iotbridge_awsiot_canbus_subscribe_thread(void *ptr);
void *iotbridge_awsiot_canbus_publish_thread(void *ptr);

#endif
