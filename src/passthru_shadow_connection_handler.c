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

#include "passthru_shadow_connection_handler.h"

unsigned int passthru_shadow_connection_handler_handle(passthru_thing *thing, int *connection) {

  if(connection == PASSTHRU_CONNECTTYPE_CONNECT) {
    thing->state = THING_STATE_CONNECTED;
    return 0;
  }

  if(connection == PASSTHRU_CONNECTTYPE_DISCONNECT) {
    thing->state = THING_STATE_DISCONNECTED;
    return 0;
  }

  return 1;
}
