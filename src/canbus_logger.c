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

#include "canbus_logger.h"

void canbus_logger_run(canbus_logger *logger) {

  syslog(LOG_DEBUG, "canbus_logger_run: running");

  canbus_connect(logger->canbus);

  if(!canbus_isconnected(logger->canbus)) {
    syslog(LOG_CRIT, "canbus_logger_run: unable to connect to CAN");
    return 1;
  }

  if(logger->type & CANBUS_LOGTYPE_FILE) {
    canbus_filelogger_run(logger);
  }
  else if(logger->type & CANBUS_LOGTYPE_AWSIOT_REPLAY) {
    canbus_awsiotlogger_replay(logger); 
  }
  else if(logger->type & CANBUS_LOGTYPE_AWSIOT) {
    canbus_awsiotlogger_run(logger);
  }
  else {
    syslog(LOG_ERR, "canbus_logger_run: invalid logger->type");
    return;
  }

  logger->canbus_thread_state = CANBUS_LOGTHREAD_RUNNING;
}

void canbus_logger_stop(canbus_logger *logger) {
  syslog(LOG_DEBUG, "canbus_logger_stop: stopping");
  logger->isrunning = false;
  logger->canbus_thread_state = CANBUS_LOGTHREAD_STOPPING;
  while(!(logger->canbus_thread_state & CANBUS_LOGTHREAD_STOPPED)) {
    syslog(LOG_DEBUG, "canbus_logger_stop: waiting for canbus connection to close");
    sleep(1);
  }
  canbus_close(logger->canbus);
  logger->canbus_thread = NULL;
}
