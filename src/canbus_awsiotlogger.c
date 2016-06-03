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

#include "canbus_awsiotlogger.h"

static const char *awsiotlogger_topic = "ecutools/datalogger";
static awsiot_client *iotlogger;
static pthread_t replay_thread;

void canbus_awsiotlogger_onopen(awsiot_client *awsiot) {
  syslog(LOG_DEBUG, "canbus_awsiotlogger_onopen");
}

void canbus_awsiotlogger_onclose(awsiot_client *awsiot) {
  syslog(LOG_DEBUG, "canbus_awsiotlogger_onclose: connection closed");
}

void canbus_awsiotlogger_onerror(awsiot_client *awsiot, const char *message) {
  syslog(LOG_DEBUG, "canbus_awsiotlogger_onerror: code:%i, message=%s", awsiot->rc, message);
}

void canbus_awsiotlogger_onmessage(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen, 
  IoT_Publish_Message_Params *params, void *pData) {

 syslog(LOG_DEBUG, "canbus_awsiotlogger_onmessage: code:%i, message=%s", 1, (char *)pData);
}

void *canbus_awsiotlogger_thread(void *ptr) {

  syslog(LOG_DEBUG, "canbus_awsiotlogger_thread: running");

  canbus_logger *pLogger = (canbus_logger *)ptr;

  int can_frame_len = sizeof(struct can_frame);
  struct can_frame frame;
  memset(&frame, 0, can_frame_len);

  int data_len = can_frame_len + 25;
  char data[data_len];
  memset(data, 0, data_len);

  while(canbus_isconnected(pLogger->canbus) && canbus_read(pLogger->canbus, &frame) > 0 && pLogger->isrunning) {

    memset(data, 0, data_len);
    canbus_framecpy(&frame, data);

    if(frame.can_id & CAN_ERR_FLAG) {
      syslog(LOG_ERR, "canbus_awsiotlogger_thread: CAN ERROR: %s", data);
      continue;
    }

    awsiot_client_publish(iotlogger, awsiotlogger_topic, data);
  }

  syslog(LOG_DEBUG, "canbus_awsiotlogger_thread: stopping");
  pLogger->canbus_thread_state = CANBUS_LOGTHREAD_STOPPED;
  canbus_iotlogger_close();

  return NULL;
}

void *canbus_awsiotlogger_replay_thread(void *ptr) {

  syslog(LOG_DEBUG, "canbus_awsiotlogger_replay_thread: running");

  canbus_logger *pLogger = (canbus_logger *)ptr;

  canbus_log_open(pLogger, "r");
  unsigned int rc;
  do {
    rc = canbus_log_read(pLogger);
  } while(rc != 0 && pLogger->isrunning);
  canbus_log_close();

  syslog(LOG_DEBUG, "canbus_awsiotlogger_replay_thread: stopping");
  pLogger->canbus_thread_state = CANBUS_LOGTHREAD_STOPPED;
  canbus_iotlogger_close();
}

unsigned int canbus_awsiotlogger_init(canbus_logger *logger) {
  if(iotlogger != NULL) return;
  iotlogger = malloc(sizeof(awsiot_client));
  iotlogger->client = malloc(sizeof(AWS_IoT_Client));
  iotlogger->onopen = &canbus_awsiotlogger_onopen;
  iotlogger->onmessage = &canbus_awsiotlogger_onmessage;
  iotlogger->onclose = &canbus_awsiotlogger_onclose;
  iotlogger->onerror = &canbus_awsiotlogger_onerror;
  iotlogger->ondisconnect = NULL;
  iotlogger->certDir = logger->certDir;
  awsiot_client_connect(iotlogger);
  logger->isrunning = true;
}

unsigned int canbus_awsiotlogger_run(canbus_logger *logger) {
  canbus_awsiotlogger_init(logger);
  pthread_create(&logger->canbus_thread, NULL, canbus_awsiotlogger_thread, (void *)logger);
  return 0;
}

void canbus_awsiotlogger_onread(const char *line) {
  awsiot_client_publish(iotlogger, awsiotlogger_topic, line);
}

unsigned int canbus_awsiotlogger_replay(canbus_logger *logger) {
  canbus_awsiotlogger_init(logger);
  logger->onread = &canbus_awsiotlogger_onread;
  pthread_create(&replay_thread, NULL, canbus_awsiotlogger_replay_thread, (void *)logger);
  return 0;
}

void canbus_iotlogger_close() {
  awsiot_client_close(iotlogger);
  free(iotlogger->client);
  free(iotlogger);
  iotlogger = NULL;
}
