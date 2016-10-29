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

#include "canbus.h"

bool reading = false;

void canbus_print_frame(struct can_frame *frame) {
  int i;
  printf("%04x: ", frame->can_id);
  if(frame->can_id & CAN_RTR_FLAG) {
    printf("remote request");
  }
  else {
    printf("[%d]", frame->can_dlc);
    for(i = 0; i < frame->can_dlc; i++)
      printf(" %02x", frame->data[i]);
  }
  printf("\n");
}

void canbus_framecpy(struct can_frame * frame, char *buf) {
  int i;
  sprintf(buf, "%04x: ", frame->can_id);
  if(frame->can_id & CAN_RTR_FLAG) {
    printf("remote request");
  }
  else {
    sprintf(buf + strlen(buf), "[%d]", frame->can_dlc);
    for(i = 0; i < frame->can_dlc; i++)
      sprintf(buf + strlen(buf), " %02x", frame->data[i]);
  }
}

unsigned int canbus_framecmp(struct can_frame *frame1, struct can_frame *frame2) {
  if(frame1->can_id != frame2->can_id) return 1;
  return strcmp((const char *)frame1->data, (const char *)frame2->data) == 0;
}

void canbus_init(canbus_client *canbus) {
  syslog(LOG_DEBUG, "canbus_init: initializing canbus client. iface=%s", canbus->iface);
  canbus->socket = 0;
  canbus->state = CANBUS_STATE_CLOSED;
  canbus->flags = 0;
  canbus->reading = false;
  if(canbus->iface == NULL) {
    canbus->iface = malloc(6);
    strcpy(canbus->iface, "vcan0");
  }
}

unsigned int canbus_connect(canbus_client *canbus) {

  syslog(LOG_DEBUG, "canbus_connect: socket=%i, iface=%s", canbus->socket, canbus->iface);

  if(canbus->socket > 0) {
    syslog(LOG_CRIT, "canbus_connect: CAN socket already connected. socket=%i", canbus->socket);
	  return 1;
  }

  if(pthread_mutex_init(&canbus->lock, NULL) != 0) {
    return 2;
  }

  if(pthread_mutex_init(&canbus->wlock, NULL) != 0) {
    syslog(LOG_ERR, "canbus_connect: unable to initialize canbus read/write mutex: %s", strerror(errno));
    return 3;
  }

  pthread_mutex_lock(&canbus->lock);
  canbus->state = CANBUS_STATE_CONNECTING;
  pthread_mutex_unlock(&canbus->lock);

  int recv_own_msgs = CANBUS_FLAG_RECV_OWN_MSGS;
  struct sockaddr_can addr;
  struct ifreq ifr;
  can_err_mask_t err_mask = CAN_ERR_MASK;

  if((canbus->socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) == -1) {
	  syslog(LOG_ERR, "canbus_connect: error opening socket");
	  return 4;
  }

  strcpy(ifr.ifr_name, canbus->iface);
  if(ioctl(canbus->socket, SIOCGIFINDEX, &ifr) < 0) {
	  syslog(LOG_ERR, "canbus_connect: unable to find CAN interface %s", canbus->iface);
	  return 5;
  }

  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if(bind(canbus->socket, (struct sockaddr *)&addr, sizeof(addr)) < -1) {
    syslog(LOG_ERR, "canbus_connect: error in socket bind");
    return 6;
  }

  if(setsockopt(canbus->socket, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask, sizeof(err_mask)) == -1) {
    syslog(LOG_ERR, "canbus_connect: unable to set CAN_RAW_ERR_FILTER socket option: %s", strerror(errno));
    return 7;
  }

  if(setsockopt(canbus->socket, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs)) == -1) {
    syslog(LOG_ERR, "canbus_connect: unable to set CAN_RAW_RECV_OWN_MSGS socket option: %s", strerror(errno));
    return 8;
  }

  syslog(LOG_DEBUG, "canbus_connect: %s socket=%i", ifr.ifr_name, canbus->socket);

  pthread_mutex_lock(&canbus->lock);
  canbus->state = CANBUS_STATE_CONNECTED;
  pthread_mutex_unlock(&canbus->lock);

  return 0;
}

bool canbus_isconnected(canbus_client *canbus) {
  return canbus->socket > 0 && (canbus->state & CANBUS_STATE_CONNECTED);
}

ssize_t canbus_read(canbus_client *canbus, struct can_frame *frame) {
  if((canbus->state & CANBUS_STATE_CONNECTED) == 0) {
    syslog(LOG_ERR, "canbus_read: CAN socket not connected");
    return 1;
  }

  int nbytes = read(canbus->socket, frame, sizeof(struct can_frame));

  if(nbytes < 0) {
    syslog(LOG_CRIT, "canbus_read: %s", strerror(errno));
    return 2;
  }

  if(nbytes < sizeof(struct can_frame)) {
    syslog(LOG_CRIT, "canbus_read: received incomplete CAN frame");
    return 3;
  }

  syslog(LOG_DEBUG, "canbus_read: read %i byte CAN frame", nbytes);

  return nbytes;
}

unsigned int canbus_write(canbus_client *canbus, struct can_frame *frame) {
  if((canbus->state & CANBUS_STATE_CONNECTED) == 0) {
    syslog(LOG_ERR, "canbus_write: CAN socket not connected");
    return 1;
  }

  pthread_mutex_lock(&canbus->wlock);

  int bytes = write(canbus->socket, frame, sizeof(struct can_frame));
  if(bytes == -1) {
    syslog(LOG_ERR, "canbus_write: %s", strerror(errno));
  }

  pthread_mutex_unlock(&canbus->wlock);

  syslog(LOG_DEBUG, "canbus_write: wrote %d byte CAN frame", bytes);

  return bytes;
}

int canbus_filter(canbus_client *canbus, struct can_filter *filters, unsigned int filter_len) {
  if((canbus->state & CANBUS_STATE_CONNECTED) == 0) {
    syslog(LOG_ERR, "canbus_write: CAN socket not connected");
    return 1;
  }
  int i;
  for(i=0; i<filter_len; i++) {
    syslog(LOG_DEBUG, "canbus_filter: can_id=%x, can_mask=%x", filters[i].can_id, filters[i].can_mask);
  }
  return setsockopt(canbus->socket, SOL_CAN_RAW, CAN_RAW_FILTER, filters, sizeof(struct can_filter) * filter_len);
}

void canbus_shutdown(canbus_client *canbus, int how) {
  if(canbus->socket != NULL) {
    if(shutdown(canbus->socket, SHUT_RD) != 0) {
      syslog(LOG_ERR, "canbus_shutdown: unable to shutdown socket");
    }
  }
}

void canbus_close(canbus_client *canbus) {

  pthread_mutex_lock(&canbus->lock);
  canbus->state = CANBUS_STATE_CLOSING;
  pthread_mutex_unlock(&canbus->lock);

  if(canbus->socket > 0) {
    canbus_shutdown(canbus, SHUT_RD);
    if(close(canbus->socket) == -1) {
      syslog(LOG_ERR, "canbus_close: error closing socket: %s", strerror(errno));
    }
    canbus->socket = 0;
  }

  syslog(LOG_DEBUG, "canbus_close: connection closed");

  pthread_mutex_lock(&canbus->lock);
  canbus->state = CANBUS_STATE_CLOSED;
  pthread_mutex_unlock(&canbus->lock);
}

void canbus_free(canbus_client *canbus) {
  if(canbus->iface != NULL) {
    free(canbus->iface);
    canbus->iface == NULL;
  }
  syslog(LOG_DEBUG, "canbus_free: freed");
}
