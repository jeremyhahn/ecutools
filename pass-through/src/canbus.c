#include "canbus.h"

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

int canbus_framecmp(struct can_frame *frame1, struct can_frame *frame2) {
  if(frame1->can_id != frame2->can_id) return -1;
  return strcmp((const char *)frame1->data, (const char *)frame2->data) == 0;
}

int canbus_connect(canbus_client *canbus) {
  if(canbus->socket > 0) {
    syslog(LOG_CRIT, "canbus_connect: already connected to CAN\n");
	return -1;
  }
#ifdef THREADED
  if(pthread_mutex_init(&canbus->lock, NULL) != 0) {
    syslog(LOG_ERR, "canbus_connect: unable to initialize canbus state mutex: %s\n", strerror(errno));
    return -1;
  }
  if(pthread_mutex_init(&canbus->rwlock, NULL) != 0) {
    syslog(LOG_ERR, "canbus_connect: unable to initialize canbus read/write mutex: %s\n", strerror(errno));
    return -1;
  }
  pthread_mutex_lock(&canbus->lock);
  canbus->state = CANBUS_STATE_CONNECTING;
  pthread_mutex_unlock(&canbus->lock);
#else
  canbus->state = CANBUS_STATE_CONNECTING;
#endif

  int recv_own_msgs = CANBUS_FLAG_RECV_OWN_MSGS;
  struct sockaddr_can addr = {0};
  struct ifreq ifr;
  can_err_mask_t err_mask = CAN_ERR_MASK;

  if((canbus->socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) == -1) {
	syslog(LOG_CRIT, "canbus_connect: error opening socket\n");
	return -1;
  }

  strcpy(ifr.ifr_name, CAN_IFACE);
  addr.can_family  = AF_CAN;

  if(ioctl(canbus->socket, SIOCGIFINDEX, &ifr) < 0) {
	syslog(LOG_CRIT, "canbus_connect: unable to find CAN interface %s\n", CAN_IFACE);
	exit(1);
  }

  addr.can_ifindex = ifr.ifr_ifindex;
  if(bind(canbus->socket, (struct sockaddr *)&addr, sizeof(addr)) < -1) {
    printf("canbus_connect: error in socket bind\n");
    return -1;
  }

  if(setsockopt(canbus->socket, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask, sizeof(err_mask)) == -1) {
    syslog(LOG_ERR, "canbus_connect: unable to set CAN_RAW_ERR_FILTER socket option: %s\n", strerror(errno));
    return -1;
  }

  if(setsockopt(canbus->socket, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs)) == -1) {
    syslog(LOG_ERR, "canbus_connect: unable to set CAN_RAW_RECV_OWN_MSGS socket option: %s\n", strerror(errno));
    return -1;
  }

  syslog(LOG_DEBUG, "canbus_connect: %s socket descriptor: %i\n", ifr.ifr_name, canbus->socket);

#ifdef THREADED
  pthread_mutex_lock(&canbus->lock);
  canbus->state = CANBUS_STATE_CONNECTED;
  pthread_mutex_unlock(&canbus->lock);
#else
  canbus->state = CANBUS_STATE_CONNECTED;
#endif

  return 0;
}

ssize_t canbus_read(canbus_client *canbus, struct can_frame *frame) {
  if((canbus->state & CANBUS_STATE_CONNECTED) == 0) {
    syslog(LOG_ERR, "canbus_read: CAN socket not connected\n");
    return -1;
  }

#ifdef THREADED
  pthread_mutex_lock(&canbus->rwlock);
#endif

  int nbytes = read(canbus->socket, frame, sizeof(struct can_frame));
  if(nbytes < 0) {
    syslog(LOG_CRIT, "canbus_read: %s\n", strerror(errno));
    return -1;
  }

#ifdef THREADED
  pthread_mutex_unlock(&canbus->rwlock);
#endif

  if(nbytes < sizeof(struct can_frame)) {
    syslog(LOG_CRIT, "canbus_read: incomplete CAN frame\n");
    return -1;
  }

  syslog(LOG_DEBUG, "canbus_read: read %i bytes\n", nbytes);
  return nbytes;
}

int canbus_write(canbus_client *canbus, struct can_frame *frame) {
  if((canbus->state & CANBUS_STATE_CONNECTED) == 0) {
    syslog(LOG_ERR, "canbus_write: CAN socket not connected\n");
    return -1;
  }

#ifdef THREADED
  pthread_mutex_lock(&canbus->rwlock);
#endif

  int bytes = write(canbus->socket, frame, sizeof(struct can_frame));
  if(bytes == -1) {
    syslog(LOG_ERR, "canbus_write: %s", strerror(errno));
  }

#ifdef THREADED
  pthread_mutex_unlock(&canbus->rwlock);
#endif

  syslog(LOG_DEBUG, "canbus_write: wrote %d bytes\n", bytes);
  return bytes;
}

void canbus_close(canbus_client *canbus) {
#ifdef THREADED
  pthread_mutex_lock(&canbus->lock);
  canbus->state = CANBUS_STATE_CLOSING;
  pthread_mutex_unlock(&canbus->lock);
#else
  canbus->state = CANBUS_STATE_CLOSING;
#endif
  if(canbus->socket > 0) {
    if(close(canbus->socket) == -1) {
      syslog(LOG_ERR, "canbus_close: error closing socket: %s\n", strerror(errno));
    }
    canbus->socket = 0;
  }
  syslog(LOG_DEBUG, "canbus_close: connection closed\n");
#ifdef THREADED
  pthread_mutex_lock(&canbus->lock);
  canbus->state = CANBUS_STATE_CLOSED;
  pthread_mutex_unlock(&canbus->lock);
#else
  canbus->state = CANBUS_STATE_CLOSED;
#endif
}
