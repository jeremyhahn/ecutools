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

#ifndef CANBUS_H_
#define CANBUS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#define CANBUS_STATE_CONNECTING (1 << 0)
#define CANBUS_STATE_CONNECTED  (1 << 1)
#define CANBUS_STATE_CLOSING    (1 << 2)
#define CANBUS_STATE_CLOSED     (1 << 3)

#define CANBUS_FLAG_RECV_OWN_MSGS 0     // 0 = disable, 1 = enable

typedef struct {
  char *iface;
  unsigned int socket;
  uint8_t state;
  uint8_t flags;
  pthread_t thread;
  bool reading;
  pthread_mutex_t lock;
  pthread_mutex_t wlock;
} canbus_client;

void canbus_init(canbus_client *canbus);
unsigned int canbus_connect(canbus_client *canbus);
bool canbus_isconnected(canbus_client *canbus);
ssize_t canbus_read(canbus_client *canbus, struct can_frame *frame);
unsigned int canbus_write(canbus_client *canbus, struct can_frame *frame);
int canbus_filter(canbus_client *canbus, struct can_filter *filters, unsigned int filter_len);
void canbus_shutdown(canbus_client *canbus, int how);
void canbus_close(canbus_client *canbus);
void canbus_framecpy(struct can_frame * frame, char *buf);
unsigned int canbus_framecmp(struct can_frame *frame1, struct can_frame *frame2);
void canbus_print_frame(struct can_frame * frame);

#endif
