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

#include "canbus_filelogger.h"

void *canbus_filelogger_thread(void *ptr) {

  syslog(LOG_DEBUG, "canbus_filelogger_thread: running");

  canbus_log_open();

  canbus_logger *pLogger = (canbus_logger *)ptr;

  int can_frame_len = sizeof(struct can_frame);
  struct can_frame frame;
  memset(&frame, 0, can_frame_len);

  int data_len = can_frame_len + 25;
  char data[data_len];
  memset(data, 0, data_len);

  while((pLogger->canbus->state & CANBUS_STATE_CONNECTED) &&
    canbus_read(pLogger->canbus, &frame) > 0) {

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
  return NULL;
}

unsigned int canbus_filelogger_run(canbus_logger *logger) {
  canbus_connect(logger->canbus);
  if(!canbus_isconnected(logger->canbus)) {
    syslog(LOG_CRIT, "canbus_filelogger_run: unable to connect to CAN");
    return 1;
  }
  pthread_create(&logger->canbus_thread, NULL, canbus_filelogger_thread, (void *)logger);
  pthread_join(logger->canbus_thread, NULL);
  syslog(LOG_DEBUG, "canbus_filelogger_run: logger closed");
  return 0;
}

unsigned int canbus_filelogger_cancel(canbus_logger *logger) {
  return pthread_cancel(logger->canbus_thread);
}