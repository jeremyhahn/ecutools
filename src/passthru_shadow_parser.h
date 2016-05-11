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

#ifndef PASSTHRUSHADOWPARSER_H_
#define PASSTHRUSHADOWPARSER_H_

#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <jansson.h>

#define PASSTHRU_SHADOW_REPORTED_MAX_ELEMENTS 50
#define PASSTHRU_SHADOW_DESIRED_MAX_ELEMENTS 50

/*
{
  "state": {
    "reported": {
      "connected":"true"
    }
  },
  "metadata": {
    "reported":{
      "connected":{
        "timestamp":1462924337
      }
    }
  },
  "version":185,
  "timestamp": 1462924337,
  "clientToken": "VirtualDataLogger-0"
}
*/

typedef struct _shadow_reported {
  char *connected;
} shadow_report;

typedef struct _shadow_metadata {
  shadow_report *reported;
} shadow_metadata;

typedef struct _shadow_desired {
  char *connected;
} shadow_desired;

typedef struct _shadow_state {
  shadow_report *reported;
  shadow_desired *desired;
} shadow_state;

typedef struct _shadow_message {
  shadow_state *state;
  shadow_metadata *metadata;
  uint64_t version;
  uint64_t timestamp;
  char *clientToken;
} shadow_message;

shadow_message* passthru_shadow_parser_parse(const char *json);

#endif