#include <stdlib.h>
#include <check.h>

#include "canbus.h"

START_TEST(test_canbus_framecmp_identical_frames)
{
    struct can_frame lhs = {
        .can_id = 0x123,
        .can_dlc = 4,
        .data = { 0x01, 0x00, 0xAA, 0x55 }
    };

    struct can_frame rhs = {
        .can_id = 0x123,
        .can_dlc = 4,
        .data = { 0x01, 0x00, 0xAA, 0x55 }
    };

    ck_assert_uint_eq(canbus_framecmp(&lhs, &rhs), 0);
}
END_TEST

START_TEST(test_canbus_framecmp_detects_different_can_id)
{
    struct can_frame lhs = {
        .can_id = 0x123,
        .can_dlc = 2,
        .data = { 0xAA, 0x55 }
    };

    struct can_frame rhs = {
        .can_id = 0x124,
        .can_dlc = 2,
        .data = { 0xAA, 0x55 }
    };

    ck_assert_uint_ne(canbus_framecmp(&lhs, &rhs), 0);
}
END_TEST

START_TEST(test_canbus_framecmp_detects_different_dlc)
{
    struct can_frame lhs = {
        .can_id = 0x123,
        .can_dlc = 2,
        .data = { 0xAA, 0x55 }
    };

    struct can_frame rhs = {
        .can_id = 0x123,
        .can_dlc = 3,
        .data = { 0xAA, 0x55, 0x00 }
    };

    ck_assert_uint_ne(canbus_framecmp(&lhs, &rhs), 0);
}
END_TEST

START_TEST(test_canbus_framecmp_detects_binary_difference_after_null_byte)
{
    struct can_frame lhs = {
        .can_id = 0x123,
        .can_dlc = 4,
        .data = { 0x01, 0x00, 0xAA, 0x55 }
    };

    struct can_frame rhs = {
        .can_id = 0x123,
        .can_dlc = 4,
        .data = { 0x01, 0x00, 0xBB, 0x55 }
    };

    ck_assert_uint_ne(canbus_framecmp(&lhs, &rhs), 0);
}
END_TEST

START_TEST(test_canbus_framecmp_detects_different_payload)
{
    struct can_frame lhs = {
        .can_id = 0x123,
        .can_dlc = 2,
        .data = { 0xAA, 0x55 }
    };

    struct can_frame rhs = {
        .can_id = 0x123,
        .can_dlc = 2,
        .data = { 0xAB, 0x55 }
    };

    ck_assert_uint_ne(canbus_framecmp(&lhs, &rhs), 0);
}
END_TEST

Suite *create_canbus_suite(void)
{
    Suite *suite = suite_create("canbus");
    TCase *tc_core = tcase_create("framecmp");

    tcase_add_test(tc_core, test_canbus_framecmp_identical_frames);
    tcase_add_test(tc_core, test_canbus_framecmp_detects_different_can_id);
    tcase_add_test(tc_core, test_canbus_framecmp_detects_different_dlc);
    tcase_add_test(tc_core, test_canbus_framecmp_detects_different_payload);
    tcase_add_test(
        tc_core,
        test_canbus_framecmp_detects_binary_difference_after_null_byte
    );

    suite_add_tcase(suite, tc_core);

    return suite;
}

int main(void)
{
    int num_fail;
    Suite *suite = create_canbus_suite();
    SRunner *sr = srunner_create(suite);

    srunner_run_all(sr, CK_NORMAL);

    num_fail = srunner_ntests_failed(sr);

    srunner_free(sr);

    return (num_fail == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
