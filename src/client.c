#include <assert.h>
#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/options.h>
#include <dc_posix/dc_netdb.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_unistd.h>
#include <dc_posix/sys/dc_socket.h>
#include <getopt.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>
#include "client_impl.h"



struct application_settings
{
    struct dc_opt_settings opts;
    struct dc_setting_bool *verbose;
    struct dc_setting_string *hostname;
    struct dc_setting_regex *ip_version;

    struct dc_setting_string *start_time;
    struct dc_setting_uint16 *server;
    struct dc_setting_uint16 *delay;
    struct dc_setting_uint16 *num_packets;
    struct dc_setting_uint16 *packet_size;
};


static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err);

static int
destroy_settings(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings **psettings);

static int run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings);

int main(int argc, char *argv[])
{
    dc_posix_tracer tracer;
    dc_error_reporter reporter;
    struct dc_posix_env env;
    struct dc_error err;
    struct dc_application_info *info;
    int ret_val;

    tracer = dc_posix_default_tracer;
    tracer = NULL;
    dc_posix_env_init(&env,  tracer);
    reporter = dc_error_default_error_reporter;
    dc_error_init(&err, reporter);
    info = dc_application_info_create(&env, &err, "Test Application");
    ret_val = dc_application_run(&env, &err, info, create_settings, destroy_settings, run, dc_default_create_lifecycle, dc_default_destroy_lifecycle,
                                 "~/.dcecho.conf",
                                 argc, argv);
    dc_application_info_destroy(&env, &info);
    dc_error_reset(&err);

    return ret_val;
}

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err)
{
    static const bool default_verbose = false;
    static const char *default_hostname = "localhost";
    static const char *default_ip = "IPv4";

    static const char *default_start_time = NULL; //now??
    static const uint16_t default_port = DEFAULT_UDP_PORT;
    static const uint16_t default_delay = DEFAULT_UDP_DELAY;
    static const uint16_t default_num_packets = DEFAULT_UDP_NUM_PACKETS;
    static const uint16_t default_packet_size = DEFAULT_UDP_PACKET_SIZE;

    struct application_settings *settings;

    settings = dc_malloc(env, err, sizeof(struct application_settings));

    if(settings == NULL)
    {
        return NULL;
    }

    settings->opts.parent.config_path = dc_setting_path_create(env, err);
    settings->verbose = dc_setting_bool_create(env, err);
    settings->hostname = dc_setting_string_create(env, err);
    settings->ip_version = dc_setting_regex_create(env, err, "^IPv[4|6]");

    settings->start_time = dc_setting_string_create(env, err);
    settings->server = dc_setting_uint16_create(env, err);
    settings->delay = dc_setting_uint16_create(env, err);
    settings->num_packets = dc_setting_uint16_create(env, err);
    settings->packet_size = dc_setting_uint16_create(env, err);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
    struct options opts[] =
            {
                    {(struct dc_setting *)settings->opts.parent.config_path, dc_options_set_path,   "config",       required_argument, 'c', "CONFIG",  dc_string_from_string, NULL,      dc_string_from_config, NULL},
                    {(struct dc_setting *)settings->verbose,                 dc_options_set_bool,   "verbose",      no_argument,       'v', "VERBOSE", dc_flag_from_string,   "verbose", dc_flag_from_config,   &default_verbose},
                    {(struct dc_setting *)settings->hostname,                dc_options_set_string, "host",         required_argument, 'h', "HOST",    dc_string_from_string, "host",    dc_string_from_config, default_hostname},
                    {(struct dc_setting *)settings->ip_version,              dc_options_set_regex,  "ip",           required_argument, 'i', "IP",      dc_string_from_string, "ip",      dc_string_from_config, default_ip},

                    {(struct dc_setting *)settings->start_time,              dc_options_set_string, "start_time",   required_argument, 't', "START_TIME",   dc_string_from_string, "start",     dc_string_from_config, default_start_time},
                    {(struct dc_setting *)settings->server,                  dc_options_set_uint16, "server",       required_argument, 's', "SERVER",       dc_uint16_from_string, "server",    dc_uint16_from_config, &default_port},
                    {(struct dc_setting *)settings->delay,                   dc_options_set_uint16, "delay",        required_argument, 'd', "DELAY",        dc_uint16_from_string, "delay",     dc_uint16_from_config, &default_delay},
                    {(struct dc_setting *)settings->num_packets,             dc_options_set_uint16, "packets",      required_argument, 'p', "PACKETS",      dc_uint16_from_string, "packets",   dc_uint16_from_config, &default_num_packets},
                    {(struct dc_setting *)settings->packet_size,             dc_options_set_uint16, "size",         required_argument, 'z', "SIZE",         dc_uint16_from_string, "size",      dc_uint16_from_config, &default_packet_size}
            };
#pragma GCC diagnostic pop

    // note the trick here - we use calloc and add 1 to ensure the last line is all 0/NULL
    settings->opts.opts = dc_calloc(env, err, (sizeof(opts) / sizeof(struct options)) + 1, sizeof(struct options));
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags = "c:vh:i:t:s:d:p:z";
    settings->opts.env_prefix = "DC_UDP_";

    return (struct dc_application_settings *)settings;
}

static int destroy_settings(const struct dc_posix_env *env, __attribute__ ((unused)) struct dc_error *err,
                            struct dc_application_settings **psettings)
{
    struct application_settings *app_settings;

    app_settings = (struct application_settings *)*psettings;
    dc_setting_bool_destroy(env, &app_settings->verbose);
    dc_setting_string_destroy(env, &app_settings->hostname);
    dc_setting_regex_destroy(env, &app_settings->ip_version);

    dc_setting_string_destroy(env, &app_settings->start_time);
    dc_setting_uint16_destroy(env, &app_settings->server);
    dc_setting_uint16_destroy(env, &app_settings->delay);
    dc_setting_uint16_destroy(env, &app_settings->num_packets);
    dc_setting_uint16_destroy(env, &app_settings->packet_size);

    dc_free(env, app_settings->opts.opts, app_settings->opts.opts_size);
    dc_free(env, app_settings, sizeof(struct application_settings));

    if(env->null_free)
    {
        *psettings = NULL;
    }

    return 0;
}

static int run(const struct dc_posix_env *env, __attribute__ ((unused)) struct dc_error *err,
               struct dc_application_settings *settings)
{
    struct application_settings *app_settings;
    bool verbose;
    const char *hostname;
    const char *ip_version;
    const char *start_time;
    in_port_t port;
    uint16_t delay;
    uint16_t num_packets;
    uint16_t packet_size;
    int ret_val;

    int family;

    app_settings = (struct application_settings *)settings;
    verbose = dc_setting_bool_get(env, app_settings->verbose);
    hostname = dc_setting_string_get(env, app_settings->hostname);
    ip_version = dc_setting_regex_get(env, app_settings->ip_version);

    start_time = dc_setting_string_get(env, app_settings->start_time);
    port = dc_setting_uint16_get(env, app_settings->server);
    delay = dc_setting_uint16_get(env, app_settings->delay);
    num_packets = dc_setting_uint16_get(env, app_settings->num_packets);
    packet_size = dc_setting_uint16_get(env, app_settings->packet_size);


    ret_val = 0;

    if(verbose)
    {
        fprintf(stderr, "Connecting to %s @ %" PRIu16 " via %s\n", hostname, port, ip_version);
    }

    if(dc_strcmp(env, ip_version, "IPv4") == 0)
    {
        family = PF_INET;
    }
    else
    {
        if(dc_strcmp(env, ip_version, "IPv6") == 0)
        {
            family = PF_INET6;
        }
        else
        {
            assert("Can't get here" != NULL);
            family = 0;
        }
    }

    ret_val = run_udp_diagnostics(env, err, hostname, family, ip_version, start_time, port, delay, num_packets, packet_size);

    return ret_val;
}
