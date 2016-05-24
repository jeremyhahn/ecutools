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

#include "canbus_filelogger.h"

void *canbus_filelogger_thread(void *ptr) {

  canbus_logger *pLogger = (canbus_logger *)ptr;
  if(canbus_log_open(pLogger, "w") != 0) return NULL;

  syslog(LOG_DEBUG, "canbus_filelogger_thread: running");

  int can_frame_len = sizeof(struct can_frame);
  struct can_frame frame;
  memset(&frame, 0, can_frame_len);

  int data_len = can_frame_len + 25;
  char data[data_len];
  memset(data, 0, data_len);

  while(canbus_isconnected(pLogger->canbus) && canbus_read(pLogger->canbus, &frame) > 0 && pLogger->isrunning) {

    memset(data, 0, data_len);
    canbus_framecpy(&frame, data);

    if(frame.can_id & CAN_ERR_FLAG) {
      syslog(LOG_ERR, "canbus_filelogger_thread: CAN ERROR: %s", data);
      continue;
    }

    canbus_log_write(data);
  }

  canbus_log_close();
  syslog(LOG_DEBUG, "canbus_filelogger_thread: stopping");
  pLogger->canbus_thread_state = CANBUS_LOGTHREAD_STOPPED;
  return NULL;
}

unsigned int canbus_filelogger_run(canbus_logger *logger) {
  logger->isrunning = true;
  pthread_create(&logger->canbus_thread, NULL, canbus_filelogger_thread, (void *)logger);
  return 0;
}

unsigned int canbus_filelogger_stop(canbus_logger *logger) {
  if(logger->canbus_thread != NULL) {
    logger->isrunning = false;
    while(canbus_isconnected(logger->canbus)) {
      syslog(LOG_DEBUG, "canbus_filelogger_stop: waiting for canbus connection to close");
      sleep(1);
    }
    logger->canbus_thread = NULL;
  }
  return 0;
}
