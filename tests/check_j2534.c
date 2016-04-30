#include <stdlib.h>
#include <math.h>
#include <check.h>
#include "j2534.h"

START_TEST(test_j2534_PassThruScanForDevices)
{
  unsigned long *pDeviceCount = 0;
  unsigned long rc = PassThruScanForDevices(&pDeviceCount);

  if(rc != STATUS_NOERROR) {
    char errmsg[80] = "\0";
    PassThruGetLastError(&errmsg);
    printf("PassThruGetLastError: %s", errmsg);
  }

  ck_assert_int_eq(pDeviceCount, 1);
}
END_TEST

Suite * create_suite(void) {

    Suite *suite = suite_create("Core");

    TCase *tc_core = tcase_create("j2534");
    tcase_add_test(tc_core, test_j2534_PassThruScanForDevices);
    suite_add_tcase(suite, tc_core);

    return suite;
}

int main( void ) {
    int num_fail;
    Suite *suite = create_suite();
    SRunner *sr = srunner_create(suite);
    srunner_run_all(sr, CK_NORMAL);
    num_fail = srunner_ntests_failed(sr);
    srunner_free (sr);
    return (num_fail == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
