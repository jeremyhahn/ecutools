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

#ifndef CANBUSLOGGER_H
#define CANBUSLOGGER_H

typedef struct canbus_logger canbus_logger;

#include "canbus_filelogger.h"
#include "canbus_awsiotlogger.h"
#include "canbus.h"

#define CANBUS_LOGTYPE_FILE          (1 << 0)
#define CANBUS_LOGTYPE_AWSIOT        (1 << 1)
#define CANBUS_LOGTYPE_AWSIOT_REPLAY (1 << 2)

#define CANBUS_LOGTHREAD_RUNNING     (1 << 0)
#define CANBUS_LOGTHREAD_STOPPING    (1 << 1)
#define CANBUS_LOGTHREAD_STOPPED     (1 << 2)

typedef struct canbus_logger {
  char *iface;
  char *logdir;
  char *logfile;
  char *certDir;
  char *cacheDir;
  bool isrunning;
  unsigned int type;
  uint8_t canbus_flags;
  uint8_t canbus_thread_state;
  canbus_client *canbus;
  pthread_t canbus_thread;
  struct canbus_filter *filters[10];
  void (*onread)(const char *line);
} canbus_logger;

void canbus_logger_run(canbus_logger *logger);
void canbus_logger_stop(canbus_logger *logger);

#endif
