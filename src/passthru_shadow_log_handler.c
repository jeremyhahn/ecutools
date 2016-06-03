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

void passthru_shadow_log_handler_init(passthru_thing *thing) {

  if(logger != NULL) {
    syslog(LOG_DEBUG, "passthru_shadow_log_handler_init: logger not null! aborting!");
    return;
  }

  logger = malloc(sizeof(canbus_logger));
  logger->logfile = NULL;
  logger->iface = thing->params->iface;
  logger->logdir = thing->params->logdir;
  logger->certDir = thing->params->certDir;

  logger->canbus = malloc(sizeof(canbus_client));
  logger->canbus->iface = thing->params->iface;

  if(logger->iface != NULL) {
    logger->canbus->iface = logger->iface;
  }

  if(logger->logdir == NULL) {
    logger->logdir = malloc(2);
    strcpy(logger->logdir, ".");
  }

  canbus_init(logger->canbus);
}

void passthru_shadow_log_handler_send_report(shadow_log *slog) {
  unsigned int json_len = 255;
  char json[json_len];
  if(slog->file) {
    snprintf(json, json_len, "{\"log\":{\"type\":%i, \"file\": \"%s\" }}", slog->type, slog->file);
  }
  else {
   snprintf(json, json_len, "{\"log\":{\"type\":%i }}", slog->type);
  }
  passthru_thing_send_report(json);
}

void passthru_shadow_log_handler_handle(passthru_thing *thing, shadow_log *slog) {

   syslog(LOG_DEBUG, "passthru_shadow_log_handler_handle: iface=%s, logidr=%s, log->type=%d, log->file=%s",
    thing->params->iface, thing->params->logdir, slog->type, slog->file);

  if(logger != NULL && slog->type != PASSTHRU_LOGTYPE_NONE) {
    syslog(LOG_ERR, "passthru_shadow_log_handler_handle: logger already running! aborting");
    return;
  }

  if(slog->type == PASSTHRU_LOGTYPE_NONE) {
    syslog(LOG_DEBUG, "passthru_shadow_log_handler_handle: LOG_NONE");
    if(logger == NULL) {
      syslog(LOG_ERR, "passthru_shadow_log_handler_handle: logger not running! aborting.");
      return;
    }
    if(logger->isrunning) {
      syslog(LOG_DEBUG, "passthru_shadow_log_handler_handle: stopping logger thread");
      canbus_logger_stop(logger);
    }
    passthru_shadow_log_handler_free();
    passthru_shadow_log_handler_send_report(slog);
    return;
  }

  passthru_shadow_log_handler_init(thing);

  if(slog->type == PASSTHRU_LOGTYPE_FILE) {
    logger->type = CANBUS_LOGTYPE_FILE;
    canbus_logger_run(logger);
    passthru_shadow_log_handler_send_report(slog);
    return;
  }

  if(slog->type == PASSTHRU_LOGTYPE_AWSIOT) {
    logger->type = CANBUS_LOGTYPE_AWSIOT;
    canbus_awsiotlogger_init(logger);
    canbus_logger_run(logger);
    passthru_shadow_log_handler_send_report(slog);
    return;
  }

  if(slog->type == PASSTHRU_LOGTYPE_AWSIOT_REPLAY) {
    logger->type = CANBUS_LOGTYPE_AWSIOT_REPLAY;
    if(slog->file == NULL) {
      syslog(LOG_ERR, "passthru_shadow_log_handler_handle: LOG_AWSIOT_REPLAY passed NULL log->file");
    }
    else {
      canbus_awsiotlogger_init(logger);
      logger->logfile = slog->file;
      canbus_awsiotlogger_replay(logger);
      passthru_shadow_log_handler_send_report(slog);
      return;
    }
  }

  return 0;
}

void passthru_shadow_log_handler_free() {
  canbus_free(logger->canbus);
  logger->canbus = NULL;
  if(logger != NULL) {
    free(logger);
    logger = NULL;
  }
}