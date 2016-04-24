#ifndef AWSIOTCLIENT_H_
#define AWSIOTCLIENT_H_

#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include "aws_iot_src/utils/aws_iot_error.h"
#include "aws_iot_src/utils/aws_iot_log.h"
#include "aws_iot_src/utils/aws_iot_version.h"
#include "aws_iot_src/protocol/mqtt/aws_iot_mqtt_interface.h"
#include "aws_iot_config.h"

typedef struct _awsiot_client {
  MQTTSubscribeParams *subscribeParams;
  MQTTPublishParams *publishParams;
  IoT_Error_t rc;
  pthread_t canbus_publish_thread;
  pthread_t canbus_subscribe_thread;
  pthread_mutex_t publish_lock;
  pthread_mutex_t subscribe_lock;
  void (*onopen)(struct _awsiot_client *);
  void (*onmessage)(MQTTCallbackParams params);
  void (*ondisconnect)(void);
  void (*onclose)(struct _awsiot_client *, const char *message);
  void (*onerror)(struct _awsiot_client *, const char *message);
} awsiot_client;

void awsiot_client_connect(awsiot_client *awsiot);
bool awsiot_client_isconnected();
void awsiot_client_canbus_subscribe(awsiot_client *awsiot);
void awsiot_client_canbus_publish(awsiot_client *awsiot, const char *payload);
void awsiot_client_close(awsiot_client *awsiot, const char *payload);

#endif
