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

#ifndef PASSTHRUSHADOWPARSER_H_
#define PASSTHRUSHADOWPARSER_H_

#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <jansson.h>
#include "passthru_thing.h"
#include "passthru_shadow.h"
#include "canbus_logger.h"

#define PASSTHRU_SHADOW_REPORTED_MAX_ELEMENTS 50
#define PASSTHRU_SHADOW_DESIRED_MAX_ELEMENTS 50

/*
Shadow:
{
  "state": {
    "reported": {
      "connected":"true",
      "log": {
        "type": "LOG_AWSIOT_REPLAY",
        "file": "ecutuned_05162016_000557_GMT.log"
      }
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
/*
Delta:
{
  "state": {
    "desired": {
      "connected":"true",
      "log": {
        "type": "LOG_AWSIOT_REPLAY",
        "file": "ecutuned_05162016_000557_GMT.log"
      }
      "j2534": "PassThruOpen"
    }
  }
}
*/

shadow_message* passthru_shadow_parser_parse_state(const char *json);
void passthru_shadow_parser_free_message(shadow_message *message);

shadow_desired* passthru_shadow_parser_parse_delta(const char *json);
void passthru_shadow_parser_free_desired(shadow_desired *message);

#endif
