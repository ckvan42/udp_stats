#include "tests.h"
#include "util.h"
#include <dc_util/strings.h>

static void test_min_max_dropped(size_t *array, size_t numPackets, size_t expected_min, size_t expected_max);


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

TestSuite *util_tests(void)
{
    TestSuite *suite;

    suite = create_test_suite();
    add_test_with_context(suite, util, count_min_max_dropped);

    return suite;
}
