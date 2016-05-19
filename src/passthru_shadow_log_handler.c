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

#include "passthru_shadow_log_handler.h"

canbus_logger *logger = NULL;

void passthru_shadow_log_handler_init() {

  if(logger != NULL) return;

  logger = malloc(sizeof(canbus_logger));
  logger->logfile = NULL;

  logger->canbus = malloc(sizeof(canbus_client));
  logger->canbus->iface = NULL;

  if(logger->iface != NULL) {
    unsigned int iface_len = strnlen(logger->iface, 8)+1;
    logger->canbus->iface = malloc(sizeof(char) * iface_len);
    strncpy(logger->canbus->iface, logger->iface, iface_len);
  }

  if(logger->logdir == NULL) {
    logger->logdir = malloc(2);
    strcpy(logger->logdir, ".");
  }

  canbus_init(logger->canbus);
}

void passthru_shadow_log_handler_handle(const char *iface, const char *logdir, shadow_log *log) {

   syslog(LOG_DEBUG, "passthru_shadow_log_handler_handle: iface=%s, logidr=%s, log->type=%i, log->file=%s",
    iface, logdir, log->type, log->file);

  if(logger != NULL && log->type != PASSTHRU_LOGTYPE_NONE) {
    syslog(LOG_ERR, "passthru_shadow_log_handler_handle: logger already running! aborting");
    return;
  }

  // LOG_NONE
  if(log->type == PASSTHRU_LOGTYPE_NONE) {
    syslog(LOG_DEBUG, "passthru_shadow_log_handler_handle: LOG_NONE");
    if(logger == NULL) {
      syslog(LOG_ERR, "passthru_shadow_log_handler_handle: logger not running! aborting.");
      return;
    }
    if(logger->isrunning) {
      syslog(LOG_DEBUG, "passthru_shadow_log_handler_handle: stopping logger thread");
      canbus_logger_stop(logger);
      passthru_shadow_log_handler_free();
    }
    return;
  }

  passthru_shadow_log_handler_init();
  logger->iface = iface;
  logger->logdir = logdir;

  if(log->type == PASSTHRU_LOGTYPE_FILE) {
    logger->type = CANBUS_LOGTYPE_FILE;
    canbus_logger_run(logger);
    return;
  }

  if(log->type == PASSTHRU_LOGTYPE_AWSIOT) {
    logger->type = CANBUS_LOGTYPE_AWSIOT;
    canbus_logger_run(logger);
    return;
  }

  if(log->type == PASSTHRU_LOGTYPE_AWSIOT_REPLAY) {
    logger->type = CANBUS_LOGTYPE_AWSIOT_REPLAY;
    if(log->file == NULL) {
      syslog(LOG_ERR, "passthru_shadow_log_handler_handle: LOG_AWSIOT_REPLAY passed NULL log->file");
    }
    else {
      logger->logfile = malloc(sizeof(char)*strlen(log->file)+1);
      strcpy(logger->logfile, log->file);
      canbus_awsiotlogger_replay(logger);
    }
  }

}

void passthru_shadow_log_handler_free() {

  canbus_free(logger->canbus);

  if(logger != NULL) {
    free(logger);
    logger = NULL;
  }
}