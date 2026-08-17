#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>

#include "j2534.h"

#define MAX_FREED_POINTERS 64

j2534_client *j2534_client_by_device_id(unsigned long device_id);

void *__real_malloc(size_t size);
void __real_free(void *ptr);

static unsigned int connect_result;
static unsigned int subscribe_result;
static unsigned int unsubscribe_result;
static int published_state;
static int close_calls;
static int record_frees;
static int record_allocations;
static int zero_allocations;
static size_t allocation_sizes[16];
static size_t allocation_count;
static void *preserved_client;
static void *freed_pointers[MAX_FREED_POINTERS];
static size_t freed_pointer_count;
static void (*message_handler)(AWS_IoT_Client *, char *, uint16_t,
                               IoT_Publish_Message_Params *, void *);
static void *message_data;

void *__wrap_malloc(size_t size)
{
  /* Isolate lifecycle behavior from the existing topic-length calculations. */
  if(record_allocations && size == 0)
    zero_allocations++;
  if(record_allocations && allocation_count < 16)
    allocation_sizes[allocation_count++] = size;
  if(size == 0)
    size = 16;
  if(size > 4096)
    size = 4096;
  return __real_malloc(size);
}

void __wrap_free(void *ptr)
{
  if(record_frees && ptr != NULL && freed_pointer_count < MAX_FREED_POINTERS)
    freed_pointers[freed_pointer_count++] = ptr;

  /* Keep the client readable so a stale vector entry is deterministic. */
  if(ptr != preserved_client)
    __real_free(ptr);
}

static int was_freed(void *ptr)
{
  size_t i;

  for(i = 0; i < freed_pointer_count; i++) {
    if(freed_pointers[i] == ptr)
      return 1;
  }
  return 0;
}

unsigned int awsiot_client_connect(awsiot_client *client)
{
  (void)client;
  return connect_result;
}

unsigned int awsiot_client_subscribe(awsiot_client *client,
                                     const char *topic,
                                     void *handler,
                                     void *data)
{
  (void)client;
  (void)topic;
  message_handler = handler;
  message_data = data;
  preserved_client = data;
  return subscribe_result;
}

unsigned int awsiot_client_unsubscribe(awsiot_client *client,
                                       const char *topic)
{
  (void)client;
  (void)topic;
  return unsubscribe_result;
}

shadow_message *passthru_shadow_parser_parse_state(const char *json)
{
  static shadow_j2534 j2534;
  static shadow_report reported;
  static shadow_state state;
  static shadow_message message;

  (void)json;
  memset(&j2534, 0, sizeof(j2534));
  memset(&reported, 0, sizeof(reported));
  memset(&state, 0, sizeof(state));
  memset(&message, 0, sizeof(message));

  j2534.state = (int *)(intptr_t)published_state;
  reported.j2534 = &j2534;
  state.reported = &reported;
  message.state = &state;
  return &message;
}

void passthru_shadow_parser_free_message(shadow_message *message)
{
  (void)message;
}

unsigned int awsiot_client_publish(awsiot_client *client,
                                   const char *topic,
                                   char *payload)
{
  IoT_Publish_Message_Params params = {
    .payload = payload,
    .payloadLen = strlen(payload)
  };

  (void)topic;
  message_handler(client->client, "test", 4, &params, message_data);
  return 0;
}

void awsiot_client_close(awsiot_client *client)
{
  (void)client;
  close_calls++;
}

IoT_Error_t aws_iot_mqtt_yield(AWS_IoT_Client *client, uint32_t timeout_ms)
{
  (void)client;
  (void)timeout_ms;
  return SUCCESS;
}

int MYINT_LEN(int *num)
{
  (void)num;
  return 1;
}

START_TEST(test_open_failure_does_not_register_client)
{
  unsigned long device_id = 100;

  connect_result = 1;

  ck_assert_int_eq(
    PassThruOpen("J2534-1:test", &device_id),
    ERR_DEVICE_NOT_CONNECTED
  );
  ck_assert_ptr_null(j2534_client_by_device_id(device_id));
}
END_TEST

START_TEST(test_open_subscription_failure_rolls_back_client)
{
  unsigned long device_id = 150;

  connect_result = 0;
  subscribe_result = 1;

  ck_assert_int_eq(
    PassThruOpen("J2534-1:test", &device_id),
    ERR_DEVICE_NOT_CONNECTED
  );
  ck_assert_ptr_null(j2534_client_by_device_id(device_id));
  ck_assert_int_eq(close_calls, 1);
}
END_TEST

START_TEST(test_close_unregisters_and_releases_client)
{
  unsigned long device_id = 200;
  j2534_client *client;
  void *shadow_update_topic;
  void *mqtt_client;
  void *filter;
  void *filter_storage;
  void *rx_storage;
  void *tx_storage;

  connect_result = 0;
  published_state = J2534_PassThruOpen;
  record_allocations = 1;
  ck_assert_int_eq(PassThruOpen("J2534-1:test", &device_id), STATUS_NOERROR);
  record_allocations = 0;

  ck_assert_int_eq(zero_allocations, 0);
  ck_assert_uint_ge(allocation_count, 6);
  ck_assert_uint_eq(
    allocation_sizes[1],
    snprintf(NULL, 0, PASSTHRU_SHADOW_UPDATE_TOPIC, "J2534-1:test") + 1
  );
  ck_assert_uint_eq(
    allocation_sizes[2],
    snprintf(NULL, 0, PASSTHRU_SHADOW_UPDATE_ACCEPTED_TOPIC, "J2534-1:test") + 1
  );
  ck_assert_uint_eq(
    allocation_sizes[3],
    snprintf(NULL, 0, J2534_ERROR_TOPIC, "J2534-1:test") + 1
  );
  ck_assert_uint_eq(
    allocation_sizes[4],
    snprintf(NULL, 0, J2534_MSG_TX_TOPIC, "J2534-1:test") + 1
  );
  ck_assert_uint_eq(
    allocation_sizes[5],
    snprintf(NULL, 0, J2534_MSG_RX_TOPIC, "J2534-1:test") + 1
  );

  client = j2534_client_by_device_id(device_id);
  ck_assert_ptr_nonnull(client);
  filter = __real_malloc(sizeof(j2534_canfilter));
  vector_add(client->filters, filter);
  vector_add(client->rxQueue, client);
  vector_add(client->txQueue, client);

  shadow_update_topic = client->shadow_update_topic;
  mqtt_client = client->awsiot->client;
  filter_storage = client->filters->data;
  rx_storage = client->rxQueue->data;
  tx_storage = client->txQueue->data;

  record_frees = 1;
  published_state = J2534_PassThruClose;
  ck_assert_int_eq(PassThruClose(device_id), STATUS_NOERROR);

  ck_assert_ptr_null(j2534_client_by_device_id(device_id));
  ck_assert_int_eq(close_calls, 1);
  ck_assert_int_eq(was_freed(shadow_update_topic), 1);
  ck_assert_int_eq(was_freed(mqtt_client), 1);
  ck_assert_int_eq(was_freed(filter), 1);
  ck_assert_int_eq(was_freed(filter_storage), 1);
  ck_assert_int_eq(was_freed(rx_storage), 1);
  ck_assert_int_eq(was_freed(tx_storage), 1);
}
END_TEST

START_TEST(test_close_failure_still_releases_closed_client)
{
  unsigned long device_id = 300;

  connect_result = 0;
  published_state = J2534_PassThruOpen;
  ck_assert_int_eq(PassThruOpen("J2534-1:test", &device_id), STATUS_NOERROR);
  ck_assert_ptr_nonnull(j2534_client_by_device_id(device_id));

  unsubscribe_result = 1;
  published_state = J2534_PassThruClose;
  ck_assert_int_eq(PassThruClose(device_id), ERR_DEVICE_NOT_CONNECTED);

  ck_assert_ptr_null(j2534_client_by_device_id(device_id));
  ck_assert_int_eq(close_calls, 1);
}
END_TEST

int main(void)
{
  int failed;
  Suite *suite = suite_create("j2534_lifecycle");
  TCase *core = tcase_create("core");
  SRunner *runner;

  tcase_add_test(core, test_open_failure_does_not_register_client);
  tcase_add_test(core, test_open_subscription_failure_rolls_back_client);
  tcase_add_test(core, test_close_unregisters_and_releases_client);
  tcase_add_test(core, test_close_failure_still_releases_closed_client);
  suite_add_tcase(suite, core);

  runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
