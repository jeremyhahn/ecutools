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

#ifndef PASSTHRUSHADOWSTATE_H
#define PASSTHRUSHADOWSTATE_H

#include <stdio.h>
#include <syslog.h>
#include <stdbool.h>
#include "mystring.h"
#include "passthru_thing.h"
#include "awsiot_client.h"
#include "passthru_shadow.h"
#include "passthru_shadow_parser.h"

void passthru_shadow_state_sync(passthru_thing *thing);

unsigned int passthru_shadow_state_open(const char *cacheDir, const char *mode);
unsigned int passthru_shadow_state_write(char *data);
unsigned int passthru_shadow_state_read();
void passthru_shadow_state_close();

 #endif
