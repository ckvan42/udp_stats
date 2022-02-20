#include "tests.h"
#include "util.h"
#include <dc_util/strings.h>

static void test_min_max_dropped(size_t *array, size_t numPackets, size_t expected_min, size_t expected_max);
static void test_min_max_out_of_order(const struct dc_posix_env *env, size_t *array, size_t numPackets, size_t expected_min, size_t expected_max);


Describe(util);

static struct dc_posix_env environ;
static struct dc_error error;

BeforeEach(util)
{
    dc_posix_env_init(&environ, NULL);
    dc_error_init(&error, NULL);
}

AfterEach(util)
{
    dc_error_reset(&error);
}

Ensure(util, count_min_max_dropped)
{
    size_t dropped_array[] = {1, 2, 4, 5, 8};
    size_t * arrayPt = malloc(sizeof dropped_array);
    memcpy(arrayPt, dropped_array, (sizeof dropped_array));
    test_min_max_dropped(arrayPt, 5, 1, 2);
}

static void test_min_max_dropped(size_t *array, size_t numPackets, size_t expected_min, size_t expected_max)
{
    size_t min;
    size_t max;
    count_min_max_dropped(array, numPackets, &min, &max);
    assert_equal(min, expected_min);
    assert_equal(max, expected_max);
}

Ensure(util, count_min_max_out_of_order)
{
    size_t dropped_array[] = {1, 3, 2, 4, 7, 6, 5};
    size_t * arrayPt = malloc(sizeof dropped_array);
    memcpy(arrayPt, dropped_array, (sizeof dropped_array));
    test_min_max_out_of_order(&environ, arrayPt, 7, 1, 2);
}

static void test_min_max_out_of_order(const struct dc_posix_env *env, size_t *array, size_t numPackets, size_t expected_min, size_t expected_max)
{
    size_t min;
    size_t max;
    count_min_max_out_of_order(env, array, numPackets, &min, &max);
    assert_equal(min, expected_min);
    assert_equal(max, expected_max);
}

Ensure(util, calculate_average_packets_lost)
{
    size_t numOfPackets;
    size_t totalNumberOfPacketsSent;
    double exp_average;
    double actual_average;

    numOfPackets = 10;
    totalNumberOfPacketsSent = 100;
    exp_average = (double)(numOfPackets)/(double)(totalNumberOfPacketsSent);
    actual_average = calculate_average_packets_lost(numOfPackets, totalNumberOfPacketsSent);
    assert_double_equal(actual_average, exp_average);
}

TestSuite *util_tests(void)
{
    TestSuite *suite;

    suite = create_test_suite();
    add_test_with_context(suite, util, count_min_max_dropped);
    add_test_with_context(suite, util, count_min_max_out_of_order);
    add_test_with_context(suite, util, calculate_average_packets_lost);

    return suite;
}
