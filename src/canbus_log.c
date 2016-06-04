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

#include "canbus_log.h"

static FILE *canbus_log;

unsigned int canbus_log_open(canbus_logger *logger, const char *mode) {

  char datestamp[100];
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  strftime(datestamp, sizeof datestamp, "ecutuned_%m%d%Y_%H%M%S_%Z", &tm);

  char filename[360];
  if(logger->logdir != NULL) {
    strcpy(filename, logger->logdir);
    if(filename[strlen(filename)] != '/') {
      strcat(filename, "/");
    }
  }

  if(logger->logfile == NULL) {
    strcat(filename, datestamp);
    strcat(filename, ".log");
  }
  else {
    strcat(filename, logger->logfile);
  }

  syslog(LOG_DEBUG, "canbus_log_open: filename=%s", filename);
  canbus_log = fopen(filename, mode);
  if(canbus_log == NULL) {
    syslog(LOG_ERR, "canbus_log_open: Unable to open %s. error=%s", filename, strerror(errno));
    return errno;
  }
  return 0;
}

unsigned int canbus_log_read(canbus_logger *logger) {
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, canbus_log)) != -1) {
    syslog(LOG_DEBUG, "canbus_log_read: len=%zu, line=%s", read, line);
    logger->onread(line);
  }
  return 0;
}

unsigned canbus_log_write(char *data) {
  if(strlen(data) > 255) {
    syslog(LOG_ERR, "canbus_log_write: data must not be larger than 255 chars");
    return 1;
  }
  char d[257];
  strncpy(d, data, strlen(data)+1);
  syslog(LOG_DEBUG, "canbus_log_write: %s", d);
  strcat(d, "\n");
  return fprintf(canbus_log, d);
}

void canbus_log_close() {
  fclose(canbus_log);
}
