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

#ifndef CANBUSLOG_H
#define CANBUSLOG_H

#include <stdio.h>
#include <syslog.h>
#include <time.h>
#include "canbus_logger.h"

unsigned int canbus_log_open(canbus_logger *logger, const char *mode);
unsigned int canbus_log_write(char *data);
unsigned int canbus_log_read(canbus_logger *logger);
void canbus_log_close();

 #endif
