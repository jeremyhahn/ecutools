#ifndef CANBUS_H_
#define CANBUS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#ifdef THREADED
	#include <pthread.h>
#endif

#ifndef CAN_IFNAME
	#define CAN_IFACE "can0"
#endif

#define CANBUS_STATE_CONNECTING (1 << 0)
#define CANBUS_STATE_CONNECTED  (1 << 1)
#define CANBUS_STATE_CLOSING    (1 << 2)
#define CANBUS_STATE_CLOSED     (1 << 3)

#define CANBUS_FLAG_RECV_OWN_MSGS 0     // 0 = disable, 1 = enable

typedef struct {
	int socket;
	uint8_t state;
	uint8_t flags;
#if THREADED
	pthread_t thread;
	pthread_mutex_t lock;
	pthread_mutex_t rwlock;
#endif
} canbus_client;

int canbus_connect(canbus_client *canbus);
ssize_t canbus_read(canbus_client *canbus, struct can_frame *frame);
int canbus_write(canbus_client *canbus, struct can_frame *frame);
void canbus_close(canbus_client *canbus);
void canbus_framecpy(struct can_frame * frame, char *buf);
int canbus_framecmp(struct can_frame *frame1, struct can_frame *frame2);
void canbus_print_frame(struct can_frame * frame);

#endif
