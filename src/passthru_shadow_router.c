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

#include "passthru_shadow_router.h"

void passthru_shadow_router_route_log(passthru_thing *thing, shadow_state *state) {
  if(state->reported->log) {
    syslog(LOG_DEBUG, "passthru_shadow_router_route_log: type=%i, file=%s", state->reported->log->type, state->reported->log->file);
    passthru_shadow_log_handler_handle(thing->params->iface, thing->params->logdir, state->reported->log);
  }
}

void passthru_shadow_router_route(passthru_thing *thing, shadow_message *message) {

  if(message == NULL || message->state == NULL) return;

  passthru_shadow_router_print_message(message);

  if(message->state->reported && message->state->reported->connection) {
    return  passthru_shadow_connection_handler_handle(message->state->reported->connection);
  }

  if(message->state->desired->log || message->state->reported->log) {
    return passthru_shadow_router_route_log(thing, message->state);
  }

  if(message->state->desired->j2534 || message->state->reported->j2534) {
    return passthru_shadow_j2534_handler_handle(message->state);
  }
}

void passthru_shadow_router_print_message(shadow_message *message) {

  char strmessage[1000] = "";

  sprintf(strmessage, "reported->connection=%i, desired->j2534=%i, reported->j2534=%i ", 
    message->state->reported->connection, 
    message->state->desired->j2534, message->state->reported->j2534
  );

  syslog(LOG_DEBUG, "passthru_shadow_router_print_message: %s", strmessage);
}
