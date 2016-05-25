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

#ifndef PASSTHRUTHING_H_
#define PASSTHRUTHING_H_

#include <stdint.h>
#include <stddef.h>
#include <signal.h>
#include <pthread.h>
#include "passthru_thing_common.h"
#include "passthru_shadow_parser.h"
#include "passthru_shadow_router.h"

#define THING_STATE_INITIALIZING  (1 << 0)
#define THING_STATE_CONNECTING    (1 << 1)
#define THING_STATE_CONNECTED     (1 << 2)
#define THING_STATE_CLOSING       (1 << 3)
#define THING_STATE_CLOSED        (1 << 4)
#define THING_STATE_DISCONNECTING (1 << 5) 
#define THING_STATE_DISCONNECTED  (1 << 6)

void passthru_thing_init(thing_init_params *params);
int passthru_thing_run();
void passthru_thing_close();
void passthru_thing_disconnect();
void passthru_thing_destroy();
int passthru_thing_send_connect_report();
int passthru_thing_send_disconnect_report();

void passthru_thing_shadow_onopen(passthru_shadow *shadow);
void passthru_thing_shadow_ondelta(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t);
void passthru_thing_shadow_onupdate(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
    const char *pReceivedJsonDocument, void *pContextData);
void passthru_thing_shadow_onget(const char *pJsonValueBuffer, uint32_t valueLength, jsonStruct_t *pJsonStruct_t);
void passthru_thing_shadow_ondisconnect();
void passthru_thing_shadow_onerror(passthru_shadow *shadow, const char *message);
void *passthru_thing_shadow_yield_thread(void *ptr);

#endif
