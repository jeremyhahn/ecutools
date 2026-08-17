#include <stdarg.h>
#include <stdlib.h>
#include <check.h>

#include "canbus.h"

static int setsockopt_calls;

int __wrap_socket(int domain, int type, int protocol)
{
  (void)domain;
  (void)type;
  (void)protocol;
  return 42;
}

int __wrap_ioctl(int fd, unsigned long request, ...)
{
  (void)fd;
  (void)request;
  return 0;
}

int __wrap_bind(int fd, const struct sockaddr *address, socklen_t length)
{
  (void)fd;
  (void)address;
  (void)length;
  return -1;
}

int __wrap_setsockopt(int fd, int level, int option,
                      const void *value, socklen_t length)
{
  (void)fd;
  (void)level;
  (void)option;
  (void)value;
  (void)length;
  setsockopt_calls++;
  return 0;
}

START_TEST(test_canbus_connect_reports_bind_failure)
{
  canbus_client client = {
    .iface = "vcan0"
  };

  setsockopt_calls = 0;

  ck_assert_uint_eq(canbus_connect(&client), 6);
  ck_assert_int_eq(setsockopt_calls, 0);

  pthread_mutex_destroy(&client.lock);
  pthread_mutex_destroy(&client.wlock);
}
END_TEST

int main(void)
{
  int failed;
  Suite *suite = suite_create("canbus_connect");
  TCase *bind_failure = tcase_create("bind_failure");
  SRunner *runner;

  tcase_add_test(bind_failure, test_canbus_connect_reports_bind_failure);
  suite_add_tcase(suite, bind_failure);

  runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
