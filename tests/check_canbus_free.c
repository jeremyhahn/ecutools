#include <stdlib.h>
#include <string.h>
#include <check.h>

#include "canbus.h"

void canbus_free(canbus_client *canbus);

START_TEST(test_canbus_free_clears_iface)
{
  canbus_client client = {0};

  client.iface = malloc(6);
  ck_assert_ptr_nonnull(client.iface);
  strcpy(client.iface, "vcan0");

  canbus_free(&client);

  ck_assert_ptr_null(client.iface);
}
END_TEST

int main(void)
{
  int failed;
  Suite *suite = suite_create("canbus_free");
  TCase *core = tcase_create("core");
  SRunner *runner;

  tcase_add_test(core, test_canbus_free_clears_iface);
  suite_add_tcase(suite, core);

  runner = srunner_create(suite);
  srunner_run_all(runner, CK_NORMAL);
  failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
