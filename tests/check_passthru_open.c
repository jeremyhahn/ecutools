#include <stdlib.h>
#include <check.h>

#include "j2534.h"

/* PassThruOpen dependencies that are not exercised by NULL validation. */
void vector_init(vector *items) { (void)items; }
void vector_add(vector *items, void *item) { (void)items; (void)item; }
void *vector_get(vector *items, int index) { (void)items; (void)index; return NULL; }

unsigned int awsiot_client_connect(awsiot_client *client)
{
  (void)client;
  return 0;
}

unsigned int awsiot_client_subscribe(awsiot_client *client,
                                     const char *topic,
                                     void *handler,
                                     void *data)
{
  (void)client;
  (void)topic;
  (void)handler;
  (void)data;
  return 0;
}

unsigned int awsiot_client_publish(awsiot_client *client,
                                   const char *topic,
                                   char *payload)
{
  (void)client;
  (void)topic;
  (void)payload;
  return 0;
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

shadow_message *passthru_shadow_parser_parse_state(const char *json)
{
  (void)json;
  return NULL;
}

void passthru_shadow_parser_free_message(shadow_message *message)
{
  (void)message;
}

START_TEST(test_passthru_open_rejects_null_name)
{
  unsigned long device_id = 1;

  ck_assert_int_eq(PassThruOpen(NULL, &device_id), ERR_NULL_PARAMETER);
}
END_TEST

START_TEST(test_passthru_open_rejects_null_device_id)
{
  ck_assert_int_eq(
    PassThruOpen("J2534-1:ecutools", NULL),
    ERR_NULL_PARAMETER
  );
}
END_TEST

int main(void)
{
  int failed;
  Suite *suite = suite_create("PassThruOpen");
  TCase *null_parameters = tcase_create("null_parameters");
  SRunner *runner;

  tcase_add_test(null_parameters, test_passthru_open_rejects_null_name);
  tcase_add_test(null_parameters, test_passthru_open_rejects_null_device_id);
  suite_add_tcase(suite, null_parameters);

  runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
