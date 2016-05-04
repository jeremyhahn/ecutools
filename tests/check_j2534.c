#include <stdlib.h>
#include <math.h>
#include <check.h>
#include "j2534.h"

extern SDEVICE j2534_device_list[25];
extern SDEVICE *j2534_device_selected;

START_TEST(test_j2534_PassThruScanForDevices)
{
  unsigned long *pDeviceCount = 0;
  long rc = PassThruScanForDevices(&pDeviceCount);

  if(rc != STATUS_NOERROR) {
    char errmsg[80] = "\0";
    PassThruGetLastError(&errmsg);
    printf("PassThruGetLastError: %s", errmsg);
  }

syslog(LOG_DEBUG, "j2534_device_selected: %s", j2534_device_selected);

  ck_assert_int_eq(pDeviceCount, 1);
}
END_TEST

START_TEST(test_j2534_PassThruOpen)
{
  unsigned long *pDeviceCount = 0;
  unsigned long *pDeviceId = 1;
  long rc;

  rc = PassThruScanForDevices(&pDeviceCount);
  if(rc != STATUS_NOERROR) {
    char errmsg[80] = "\0";
    PassThruGetLastError(&errmsg);
    printf("PassThruGetLastError: %s", errmsg);
  }
 
  rc = PassThruOpen("J2534-1:ecutools", pDeviceId);
  if(rc != STATUS_NOERROR) {
    char errmsg[80] = "\0";
    PassThruGetLastError(&errmsg);
    printf("PassThruGetLastError: %s", errmsg);
  }

  ck_assert_int_eq(pDeviceId, 1);
}
END_TEST

Suite * create_suite(void) {
    Suite *suite = suite_create("ecutools");

    TCase *tc_core = tcase_create("j2534");
    tcase_add_test(tc_core, test_j2534_PassThruScanForDevices);
    tcase_add_test(tc_core, test_j2534_PassThruOpen);
    suite_add_tcase(suite, tc_core);

    return suite;
}

int main( void ) {
    openlog("ecutools-testsuite", LOG_CONS | LOG_PERROR, LOG_USER);
    syslog(LOG_DEBUG, "starting ecutools-j2534-testsuite");
    int num_fail;
    Suite *suite = create_suite();
    SRunner *sr = srunner_create(suite);
    srunner_run_all(sr, CK_NORMAL);
    num_fail = srunner_ntests_failed(sr);
    srunner_free (sr);
    closelog();
    return (num_fail == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
