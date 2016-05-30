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

#include "passthru_shadow_j2534_handler.h"

void passthru_shadow_j2534_handler_send_error(passthru_thing *thing, unsigned int state, unsigned int error) {
  syslog(LOG_ERR, "passthru_shadow_j2534_handler_send_error: error=%x", error);
  char json[34];
  snprintf(json, 34, "{\"j2534\":{\"state\":%i,\"error\":\"%x\"}}", state, error);
  passthru_thing_send_report(json);
}

void passthru_shadow_j2534_handler_send_report(int state) {
  char json[36];
  snprintf(json, 36, "{\"j2534\":{\"state\":%i,\"error\":null}}", state);
  passthru_thing_send_report(json);
}

void passthru_shadow_j2534_handler_desired_disconnect(passthru_thing *thing) {
  syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_handle: REPORT");
  canbus_close(thing->j2534->canbus);
  passthru_shadow_j2534_handler_send_report(J2534_PassThruDisconnect);
}

void passthru_shadow_j2534_handler_desired_connect(passthru_thing *thing) {

  if(vector_get(thing->j2534->clients, thing->name) || thing->j2534->canbus != NULL) {
    syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_desired_connect: already connected. aborting.");
    return passthru_shadow_j2534_handler_send_error(thing, J2534_PassThruConnect, ERR_DEVICE_IN_USE);
  }

  canbus_init(thing->j2534->canbus);
  if(!canbus_connect(thing->j2534->canbus)) {
    syslog(LOG_ERR, "passthru_shadow_j2534_handler_desired_connect: Failed to establish CAN connection");
    return;
  }

  passthru_shadow_j2534_handler_send_report(J2534_PassThruConnect);
  syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_handle_desired: CAN connection successful");  
}

void passthru_shadow_j2534_handler_handle_desired(passthru_thing *thing, shadow_desired *desired) {

  int *state = desired->j2534->state;

  if(state == J2534_PassThruConnect) {
    passthru_shadow_j2534_handler_desired_connect(thing);
  }

  if(state == J2534_PassThruDisconnect) {
    passthru_shadow_j2534_handler_desired_disconnect(thing);
  }

}

void passthru_shadow_j2534_handler_handle(passthru_thing *thing, shadow_state *state) {

  if(state->reported->j2534->state) {
    syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_handle: REPORT");
  }

  if(state->desired->j2534->state) {
    passthru_shadow_j2534_handler_handle_desired(thing, state->desired);
    syslog(LOG_DEBUG, "passthru_shadow_j2534_handler_handle: DESIRED");
  }

}
