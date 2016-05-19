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

void passthru_shadow_router_route(passthru_thing *thing, shadow_message *message) {

  // report: connection
  if(message && message->state && message->state->reported && message->state->reported->connection) {
    syslog(LOG_DEBUG, "passthru_shadow_router_route: connection=%i", message->state->reported->connection);
    passthru_shadow_connection_handler_handle(message->state->reported->connection);
    return;
  }

  // report: log
  if(message && message->state && message->state->reported && message->state->reported->log != NULL) {
    syslog(LOG_DEBUG, "passthru_shadow_router_route: log->type=%i, log->file=%s", message->state->reported->log->type, message->state->reported->log->file);
    passthru_shadow_log_handler_handle(thing->params->iface, thing->params->logdir, message->state->reported->log);
    return;
  }

}