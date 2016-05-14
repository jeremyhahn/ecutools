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

#include "canbus_log.h"

char* canbus_log_datestamp() {
  char buf[1000];
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);
  return buf;
}

void canbus_log_open() {
   char filename[1050];
   snprintf(filename, 1000, canbus_log_datestamp());
   strcat(filename, ".log");
   canbus_log = fopen(filename, "w");
   if(canbus_log == NULL) {
     syslog(LOG_ERR, "canbus_log_open: Unable to open log file: %s", filename);
     return;
   }
}

int canbus_log_write(char *data) {
  if(strlen(data) > 255) {
    syslog(LOG_ERR, "canbus_log_write: data is larger than 255 bytes");
    return 1;
  }
  char d[257];
  strncpy(d, data, strlen(data)+1);
  strcat(d, "\n");
  return fprintf(canbus_log, d);
}

void canbus_log_close() {
  fclose(canbus_log);
}
