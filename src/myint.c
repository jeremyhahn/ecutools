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

#include "myint.h"
#include <syslog.h>

int MYINT_LEN(int *num) {
  if(*num >= 100000000000000000000000) {
    syslog(LOG_ERR, "MYINT_LEN: number greater than 25 digits! aborting!");
    return 0;
  }
  char sDeviceId[25];
  return snprintf(sDeviceId, 25, "%d", num);
}

int MYINT_DUP(int const *src) {
  size_t len = MYINT_LEN(src);
  char ssrc[len+1];
  snprintf(ssrc, len+1, "%d", src);
  return atoi(ssrc);
/*
  unsigned long * p = malloc(len * sizeof(unsigned long));
  memcpy(p, src, len * sizeof(unsigned long));
  return p;
*/
}

