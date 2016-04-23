#ifndef WCBRIDGE_H_
#define WCBRIDGE_H_

#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include "cwebsocket.h"
#include "canbus.h"

#define WCBRIDGE_WEBSOCKET_ENDPOINT ((const char *)"ws://ecutools.io:8080/ecutune")

//#define WCBRIDGE_FLAG_MODE_LOGGING (1 << 0);

struct wcbridge_filter;

typedef struct _wcbridge {
  struct wcbridge_filter *filters[10];
  pthread_t websocket_thread;
  pthread_attr_t websocket_thread_attr;
  pthread_t canbus_thread;
  cwebsocket_client *websocket;
  canbus_client *canbus;
  uint8_t canbus_flags;
  void (*onmessage)(struct _wcbridge *, struct can_frame *frame);
} wcbridge;

typedef struct {
  void (*onframe)(wcbridge *bridge, struct can_frame *frame);
} wcbridge_filter;

wcbridge *bridge;

wcbridge *wcbridge_new();
int wcbridge_run(wcbridge *wcbridge);
void wcbridge_close(wcbridge *wcbridge, const char *message);
void wcbridge_destroy(wcbridge *wcbridge);
void wcbridge_process_filter(wcbridge *bridge, struct can_frame *frame);

void *wcbridge_canbus_logger_thread(void *ptr);
void *wcbridge_websocket_thread(void *ptr);

#endif
