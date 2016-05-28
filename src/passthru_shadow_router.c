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

void passthru_shadow_router_route_desired_log(passthru_thing *thing, shadow_log *slog) {
  syslog(LOG_DEBUG, "passthru_shadow_router_route_desired_log: type=%i, file=%s", slog->type, slog->file);
  passthru_shadow_log_handler_handle(thing->params->iface, thing->params->logdir, slog);
}

void passthru_shadow_router_route(passthru_thing *thing, shadow_message *message) {

  if(message == NULL || message->state == NULL) return;

  passthru_shadow_router_print_message(message);

  if(message->state->reported && message->state->reported->connection) {
    return passthru_shadow_connection_handler_handle(thing, message->state->reported->connection);
  }

  if(message->state->desired->log) {
    return passthru_shadow_router_route_desired_log(thing, message->state);
  }

  if(message->state->desired->j2534 || message->state->reported->j2534) {
    return passthru_shadow_j2534_handler_handle(thing, message->state);
  }

  syslog(LOG_DEBUG, "passthru_shadow_router_route: unable to locate state hander");
}

void passthru_shadow_router_route_delta(passthru_thing *thing, shadow_desired *desired) {
  if(desired->log) {
    passthru_shadow_router_route_desired_log(thing, desired->log);
  }

  syslog(LOG_DEBUG, "passthru_shadow_router_route_delta: unable to locate delata handler");
}

void passthru_shadow_router_print_message(shadow_message *message) {
  syslog(LOG_DEBUG, "passthru_shadow_router_print_message: reported->connection=%i", message->state->reported->connection);
  syslog(LOG_DEBUG, "passthru_shadow_router_print_message: desired->j2534=%i, reported->j2534=%i",  message->state->desired->j2534,  message->state->reported->j2534);
  syslog(LOG_DEBUG, "passthru_shadow_router_print_message: desired->log->type=%d, desired->log->file=%s",  message->state->desired->log->type,  message->state->desired->log->file);
  syslog(LOG_DEBUG, "passthru_shadow_router_print_message: reported->log->type=%d, reported->log->file=%s",  message->state->reported->log->type,  message->state->reported->log->file);
}
