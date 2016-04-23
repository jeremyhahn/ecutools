#ifndef IOTBRIDGE_H_
#define IOTBRIDGE_H_

#include <stddef.h>
#include <signal.h>
#include <pthread.h>
#include "canbus.h"
#include "awsiot_client.h"

typedef struct _iotbridge {
  struct iotbridge_filter *filters[10];
  pthread_t canbus_thread;
  pthread_t awsiot_thread;
  awsiot_client *awsiot;
  canbus_client *canbus;
  uint8_t canbus_flags;
} iotbridge;

iotbridge *bridge;
iotbridge *wcbridge_new();
int iotbridge_run(iotbridge *iotbridge);
void iotbridge_close(iotbridge *iotbridge, const char *message);
void iotridge_destroy(iotbridge *iotbridge);
void iotbridge_process_filter(iotbridge *bridge, struct can_frame *frame);

void *iotbridge_canbus_connect_thread(void *ptr);
void *iotbridge_canbus_logger_thread(void *ptr);
void *iotbridge_awsiot_subscribe_thread(void *ptr);

#endif
