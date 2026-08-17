#include <stdlib.h>
#include <check.h>

#include "canbus.h"

static int shutdown_fd;
static int shutdown_how;

int __wrap_shutdown(int fd, int how)
{
  shutdown_fd = fd;
  shutdown_how = how;
  return 0;
}

START_TEST(test_canbus_shutdown_forwards_how)
{
  canbus_client client = {
    .socket = 42
  };

  shutdown_fd = -1;
  shutdown_how = -1;

  canbus_shutdown(&client, SHUT_WR);

  ck_assert_int_eq(shutdown_fd, 42);
  ck_assert_int_eq(shutdown_how, SHUT_WR);
}
END_TEST

int main(void)
{
  int failed;
  Suite *suite = suite_create("canbus_shutdown");
  TCase *core = tcase_create("core");
  SRunner *runner;

  tcase_add_test(core, test_canbus_shutdown_forwards_how);
  suite_add_tcase(suite, core);

  runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
