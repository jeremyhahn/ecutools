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

void passthru_shadow_router_print_desired(shadow_desired *desired) {
  syslog(LOG_DEBUG, "passthru_shadow_router_print_desired: j2534->deviceId=%d, j2534->state=%d, j2534->error=%x, j2534->filters->count=%i", 
    desired->j2534->deviceId, desired->j2534->state, desired->j2534->error, desired->j2534->filters->count);
  syslog(LOG_DEBUG, "passthru_shadow_router_print_desired: log->type=%d, log->file=%s",  desired->log->type, desired->log->file);
}

void passthru_shadow_router_print_reported(shadow_report *reported) {
  syslog(LOG_DEBUG, "passthru_shadow_router_print_reported: j2534->deviceId=%d, j2534->state=%d, j2534->error=%x", reported->j2534->deviceId, reported->j2534->state, reported->j2534->error);
  syslog(LOG_DEBUG, "passthru_shadow_router_print_reported: log->type=%d, log->file=%s",  reported->log->type, reported->log->file);
}

void passthru_shadow_router_print_message(shadow_message *message) {
  syslog(LOG_DEBUG, "passthru_shadow_router_print_message: reported->connection=%i", message->state->reported->connection);
  passthru_shadow_router_print_desired(message->state->desired);
  passthru_shadow_router_print_reported(message->state->reported);
}

void passthru_shadow_router_route_message(passthru_thing *thing, shadow_message *message) {

  if(message == NULL || message->state == NULL) return;

  passthru_shadow_router_print_message(message);

  if(message->state->reported && message->state->reported->connection) {
    return passthru_shadow_connection_handler_handle(thing, message->state->reported->connection);
  }

  if(message->state->desired->log->type) {
    return passthru_shadow_log_handler_handle(thing, message->state->desired->log);
  }

  if(message->state->desired->j2534->state) {
    return passthru_shadow_j2534_handler_handle_state(thing, message->state);
  }

  syslog(LOG_DEBUG, "passthru_shadow_router_route: unable to locate state hander");
}

void passthru_shadow_router_route_delta(passthru_thing *thing, shadow_desired *desired) {

  syslog(LOG_DEBUG, "passthru_shadow_router_route_delta");

  passthru_shadow_router_print_desired(desired);

  if(desired->log->type) {
    return passthru_shadow_log_handler_handle(thing, desired->log);
  }

  if(desired->j2534->state) {
    return passthru_shadow_j2534_handler_handle_delta(thing, desired->j2534);
  }

  syslog(LOG_DEBUG, "passthru_shadow_router_route_delta: unable to locate delta handler");
}
