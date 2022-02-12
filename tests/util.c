#include "tests.h"
#include "util.h"
#include <dc_util/strings.h>

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

Ensure(util, process_packets_in_sequence)
{
    struct udp_packet udpPacket;

    unsetenv("PS1");
    prompt = get_prompt(&environ, &error);
    assert_that(prompt, is_equal_to_string("$ "));
    free(prompt);

    setenv("PS1", "ABC", true);
    prompt = get_prompt(&environ, &error);
    assert_that(prompt, is_equal_to_string("ABC"));
    free(prompt);
}


TestSuite *util_tests(void)
{
    TestSuite *suite;

    suite = create_test_suite();
    add_test_with_context(suite, util, get_prompt);

    return suite;
}