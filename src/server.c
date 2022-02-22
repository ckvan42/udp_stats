#include "udp_structures.h"
#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/options.h>
#include <dc_network/common.h>
#include <dc_network/options.h>
#include <dc_network/server.h>
#include <dc_posix/dc_netdb.h>
#include <dc_posix/dc_signal.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/sys/dc_socket.h>
#include <dc_util/dump.h>
#include <dc_util/streams.h>
#include <dc_util/types.h>
#include <getopt.h>
#include <unistd.h>
#include "connections.h"
#include "util.h"
#include <poll.h>
#include <dc_posix/dc_unistd.h>
#include <arpa/inet.h>
#include <dc_posix/dc_fcntl.h>


struct application_settings
{
    struct dc_opt_settings opts;
    struct dc_setting_bool *verbose;
    struct dc_setting_string *hostname;
    struct dc_setting_regex *ip_version;
    struct dc_setting_uint16 *port;
    struct dc_setting_bool *reuse_address;
    struct addrinfo *address;
    int server_socket_fd;

    //udp_information

};

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err);

static int
destroy_settings(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings **psettings);

static int run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings);

static void signal_handler(int signnum);

static void do_create_settings(const struct dc_posix_env *env, struct dc_error *err, void *arg);

static void do_create_socket(const struct dc_posix_env *env, struct dc_error *err, void *arg);

static void do_set_sockopts(const struct dc_posix_env *env, struct dc_error *err, void *arg);

static void do_bind(const struct dc_posix_env *env, struct dc_error *err, void *arg);

static void do_listen(const struct dc_posix_env *env, struct dc_error *err, void *arg);

static void do_setup(const struct dc_posix_env *env, struct dc_error *err, void *arg);

static bool do_accept(const struct dc_posix_env *env, struct dc_error *err, int *client_socket_fd, void *arg);

static void do_shutdown(const struct dc_posix_env *env, struct dc_error *err, void *arg);

static void do_destroy_settings(const struct dc_posix_env *env, struct dc_error *err, void *arg);

void echo(const struct dc_posix_env *env, struct dc_error *err, int client_socket_fd, uint16_t port);

/*
static void write_displayer(const struct dc_posix_env *env, struct dc_error *err, const uint8_t *data, size_t count,
                            size_t file_position, void *arg);

static void read_displayer(const struct dc_posix_env *env, struct dc_error *err, const uint8_t *data, size_t count,
                           size_t file_position, void *arg);
*/

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static volatile sig_atomic_t exit_signal = 0;

int main(int argc, char *argv[])
{
    dc_posix_tracer tracer;
    dc_error_reporter reporter;
    struct dc_posix_env env;
    struct dc_error err;
    struct dc_application_info *info;
    int ret_val;
    struct sigaction sa;

    tracer = dc_posix_default_tracer;
    tracer = NULL;
    dc_posix_env_init(&env, tracer);
    reporter = dc_error_default_error_reporter;
    dc_error_init(&err, reporter);
    dc_memset(&env, &sa, 0, sizeof(sa));
    sa.sa_handler = &signal_handler;
    sa.sa_flags = 0;
    dc_sigaction(&env, &err, SIGINT, &sa, NULL);
    dc_sigaction(&env, &err, SIGTERM, &sa, NULL);

    info = dc_application_info_create(&env, &err, "UDP Server");
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
    static const uint16_t default_port = DEFAULT_UDP_PORT;
    static const bool default_reuse = false;
    struct application_settings *settings;

    DC_TRACE(env);
    settings = dc_malloc(env, err, sizeof(struct application_settings));

    if(settings == NULL)
    {
        return NULL;
    }

    settings->opts.parent.config_path = dc_setting_path_create(env, err);
    settings->verbose = dc_setting_bool_create(env, err);
    settings->hostname = dc_setting_string_create(env, err);
    settings->ip_version = dc_setting_regex_create(env, err, "^IPv[4|6]");
    settings->port = dc_setting_uint16_create(env, err);
    settings->reuse_address = dc_setting_bool_create(env, err);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
    struct options opts[] =
            {
                    {(struct dc_setting *)settings->opts.parent.config_path, dc_options_set_path,   "config",  required_argument, 'c', "CONFIG",        dc_string_from_string, NULL,            dc_string_from_config, NULL},
                    {(struct dc_setting *)settings->verbose,                 dc_options_set_bool,   "verbose", no_argument,       'v', "VERBOSE",       dc_flag_from_string,   "verbose",       dc_flag_from_config,   &default_verbose},
                    {(struct dc_setting *)settings->hostname,                dc_options_set_string, "host",    required_argument, 'h', "HOST",          dc_string_from_string, "host",          dc_string_from_config, default_hostname},
                    {(struct dc_setting *)settings->ip_version,              dc_options_set_regex,  "ip",      required_argument, 'i', "IP",            dc_string_from_string, "ip",            dc_string_from_config, default_ip},
                    {(struct dc_setting *)settings->port,                    dc_options_set_uint16, "port",    required_argument, 'p', "PORT",          dc_uint16_from_string, "port",          dc_uint16_from_config, &default_port},
                    {(struct dc_setting *)settings->reuse_address,           dc_options_set_bool,   "force",   no_argument,       'f', "REUSE_ADDRESS", dc_flag_from_string,   "reuse_address", dc_flag_from_config,   &default_reuse},
            };
#pragma GCC diagnostic pop

    // note the trick here - we use calloc and add 1 to ensure the last line is all 0/NULL
    settings->opts.opts = dc_calloc(env, err, (sizeof(opts) / sizeof(struct options)) + 1, sizeof(struct options));
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags = "c:vh:i:p:f";
    settings->opts.env_prefix = "DC_UDP";

    return (struct dc_application_settings *)settings;
}

static int destroy_settings(const struct dc_posix_env *env, __attribute__ ((unused)) struct dc_error *err,
                            struct dc_application_settings **psettings)
{
    struct application_settings *app_settings;

    DC_TRACE(env);
    app_settings = (struct application_settings *)*psettings;
    dc_setting_bool_destroy(env, &app_settings->verbose);
    dc_setting_string_destroy(env, &app_settings->hostname);
    dc_setting_regex_destroy(env, &app_settings->ip_version);
    dc_setting_uint16_destroy(env, &app_settings->port);
    dc_setting_bool_destroy(env, &app_settings->reuse_address);
    dc_free(env, app_settings->opts.opts, app_settings->opts.opts_size);
    dc_free(env, app_settings, sizeof(struct application_settings));

    if(env->null_free)
    {
        *psettings = NULL;
    }

    return 0;
}


static struct dc_server_lifecycle *create_server_lifecycle(const struct dc_posix_env *env, struct dc_error *err)
{
    struct dc_server_lifecycle *lifecycle;

    DC_TRACE(env);
    lifecycle = dc_server_lifecycle_create(env, err);
    dc_server_lifecycle_set_create_settings(env, lifecycle, do_create_settings);
    dc_server_lifecycle_set_create_socket(env, lifecycle, do_create_socket);
    dc_server_lifecycle_set_set_sockopts(env, lifecycle, do_set_sockopts);
    dc_server_lifecycle_set_bind(env, lifecycle, do_bind);
    dc_server_lifecycle_set_listen(env, lifecycle, do_listen);
    dc_server_lifecycle_set_setup(env, lifecycle, do_setup);
    dc_server_lifecycle_set_accept(env, lifecycle, do_accept);
    dc_server_lifecycle_set_shutdown(env, lifecycle, do_shutdown);
    dc_server_lifecycle_set_destroy_settings(env, lifecycle, do_destroy_settings);

    return lifecycle;
}

static void destroy_server_lifecycle(const struct dc_posix_env *env, struct dc_server_lifecycle **plifecycle)
{
    DC_TRACE(env);
    dc_server_lifecycle_destroy(env, plifecycle);
}


static int run(const struct dc_posix_env *env, __attribute__ ((unused)) struct dc_error *err,
               struct dc_application_settings *settings)
{
    int ret_val;
    struct dc_server_info *info;

    DC_TRACE(env);
    info = dc_server_info_create(env, err, "UDP Server", NULL, settings);

    if(dc_error_has_no_error(err))
    {
        dc_server_run(env, err, info, create_server_lifecycle, destroy_server_lifecycle);
        dc_server_info_destroy(env, &info);
    }

    if(dc_error_has_no_error(err))
    {
        ret_val = 0;
    }
    else
    {
        ret_val = -1;
    }

    return ret_val;
}


void signal_handler(__attribute__ ((unused)) int signnum)
{
    printf("caught %d\n", signnum);
    exit_signal = 1;
}

static void do_create_settings(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct application_settings *app_settings;
    const char *ip_version;
    int family;

    DC_TRACE(env);
    app_settings = arg;
    ip_version = dc_setting_regex_get(env, app_settings->ip_version);

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
            DC_ERROR_RAISE_USER(err, "Invalid ip_version", -1);
            family = 0;
        }
    }

    if(dc_error_has_no_error(err))
    {
        const char *hostname;

        hostname = dc_setting_string_get(env, app_settings->hostname);
        dc_network_get_addresses(env, err, family, SOCK_STREAM, hostname, &app_settings->address);
    }
}

static void do_create_socket(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct application_settings *app_settings;
    int socket_fd;

    DC_TRACE(env);
    app_settings = arg;
    socket_fd = dc_network_create_socket(env, err, app_settings->address);

    if(dc_error_has_no_error(err))
    {
        app_settings = arg;
        app_settings->server_socket_fd = socket_fd;
    }
    else
    {
        socket_fd = -1;
    }
}

static void do_set_sockopts(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct application_settings *app_settings;
    bool reuse_address;

    DC_TRACE(env);
    app_settings = arg;
    reuse_address = dc_setting_bool_get(env, app_settings->reuse_address);
    dc_network_opt_ip_so_reuse_addr(env, err, app_settings->server_socket_fd, reuse_address);
}

static void do_bind(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct application_settings *app_settings;
    uint16_t port;

    DC_TRACE(env);
    app_settings = arg;
    port = dc_setting_uint16_get(env, app_settings->port);

    dc_network_bind(env,
                    err,
                    app_settings->server_socket_fd,
                    app_settings->address->ai_addr,
                    port);
}

static void do_listen(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct application_settings *app_settings;
    int backlog;

    DC_TRACE(env);
    app_settings = arg;
    backlog = 5;    // TODO: make this a setting
    dc_network_listen(env, err, app_settings->server_socket_fd, backlog);
}

static void do_setup(const struct dc_posix_env *env, __attribute__ ((unused)) struct dc_error *err,
                     __attribute__ ((unused)) void *arg)
{
    DC_TRACE(env);
}

static bool do_accept(const struct dc_posix_env *env, struct dc_error *err, int *client_socket_fd, void *arg)
{
    struct application_settings *app_settings;
    bool ret_val;

    DC_TRACE(env);
    app_settings = arg;
    ret_val = false;

    in_port_t port;

    printf("accepting\n");
    *client_socket_fd = dc_network_accept(env, err, app_settings->server_socket_fd);
    printf("accepted\n");

    port = dc_setting_uint16_get(env, app_settings->port);
    if(dc_error_has_error(err))
    {
        if(exit_signal == 1 && dc_error_is_errno(err, EINTR))
        {
            ret_val = true;
            dc_error_reset(err);
        }
    }
    else
    {
        echo(env, err, *client_socket_fd, port);
    }

    return ret_val;
}

static void print_packets(struct udp_packet* packet)
{
    int i;
    i = 0;

    while ((packet->list_packets[i]) && (i < packet->diagnostics->packet_sent))
    {
        printf("EACH PACKET: %s\n", packet->list_packets[i]);
        i++;
    }
}

static void output_result(const struct dc_posix_env *env, struct dc_error *err, void* arg);
static void print_packets(struct udp_packet* packet);
static int create_log_file(const struct dc_posix_env *env, struct dc_error *err);

#define TIME_OUT_SEC 5
#define MAX_LOG_SIZE 1024
static void connect_to_udp(const struct dc_posix_env* env, struct dc_error * err, uint16_t port, void *arg, int tcp_fd)
{
    int       sockfd;
    struct    sockaddr_in servaddr;
    struct    sockaddr_in cliaddr;
    socklen_t len;
    size_t     n;
    struct pollfd poll_set[2];
    int timeout;
    int res_val;
    struct udp_packet* udpPacket;
    int log_fd;

    udpPacket = (struct udp_packet*)arg;

    char      buffer[udpPacket->diagnostics->packet_size + 1];
    timeout = TIME_OUT_SEC * 1000;

    log_fd = create_log_file(env, err);

    if (dc_error_has_no_error(err))
    {
        if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }

        memset(&servaddr, 0, sizeof(servaddr));
        memset(&cliaddr, 0, sizeof(cliaddr));

        servaddr.sin_family      = AF_INET; // IPv4
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port        = htons(port);

        if(bind(sockfd, (const struct sockaddr *)&servaddr,  sizeof(servaddr)) < 0)
        {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        poll_set[0].fd = sockfd;
        poll_set[0].events = POLLIN;

        poll_set[1].fd = tcp_fd;
        poll_set[1].events = POLLIN;
        len = sizeof(cliaddr);  //len is value/resuslt
        int i = 0;
        while ((res_val = poll(poll_set, 2, timeout)) >= 0)
        {
            if (res_val != 0)
            {
                if (poll_set[0].revents & POLLIN)
                {
                    char *ip;
                    n = (size_t) recvfrom(poll_set[0].fd, (char *) buffer, udpPacket->diagnostics->packet_size,
                                          MSG_WAITALL, (struct sockaddr *) &cliaddr,
                                          &len);
                    buffer[n] = '\0';
                    char file_content[MAX_LOG_SIZE];
                    size_t size_file_content;
                    ip = inet_ntoa(cliaddr.sin_addr);
                    sprintf(file_content, "client port: %d\nip address: %s\nPACKET: %s\n", cliaddr.sin_port, ip, buffer);
                    size_file_content = dc_strlen(env, file_content) + 1;
                    dc_write(env, err, log_fd, file_content, size_file_content);
//                    if (i ==0) //only happens for the first time.
//                    {
//                        udpPacket->diagnostics->info->client_port = cliaddr.sin_port;
//                        udpPacket->diagnostics->info->ip_address = dc_strdup(env, err, ip);
//                    }
//                    //output_to_file
//
//                    udpPacket->list_packets[i] = dc_strdup(env, err, buffer);
                    i++;
                }

                if (poll_set[1].revents & POLLIN)
                {
                    size_t size;

                    size = dc_strlen(env, END_MESSAGE) + 1;
                    char incoming[size];
                    n = (size_t) read(poll_set[1].fd, incoming, size);
                    incoming[n] = '\0';
                    if (dc_strcmp(env, END_MESSAGE, incoming) == 0)
                    {
                        break;
                    }
                }
            }
        }
        udpPacket->diagnostics->packet_received = (size_t) i;

        if (dc_error_has_no_error(err))
        {
//        process_udp(env, err, &udpPacket);
            if (dc_error_has_no_error(err))
            {
//            output_result(env, err, &(udpPacket->diagnostics));
            }
        }
        //reset_needed
//        print_packets(udpPacket);
        dc_close(env, err, log_fd);
        dc_close(env, err, sockfd);
    }
}

#define FILE_NAME_LENGTH 100

static int create_log_file(const struct dc_posix_env *env, struct dc_error *err)
{
    struct tm* ptr;
    int file_fd;

    time_t current_time;
    current_time = time(NULL);
    ptr = localtime(&current_time);



    //make file name
    char fileName[FILE_NAME_LENGTH];
    sprintf(fileName, "UDP_LOG_FILE: %s.log", asctime(ptr));
    file_fd = dc_open(env, err, fileName, O_CREAT|O_WRONLY|O_APPEND, S_IRWXU|S_IRGRP|S_IROTH);

    return file_fd;
}

static void output_result(const struct dc_posix_env *env, struct dc_error *err, void* arg)
{
    ssize_t sz;
    struct diagnostics* diagnostics;

    diagnostics = arg;

    int fd = open("foo.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        perror("r1");
        exit(1);
    }
//    char tmp[10000];
//    sprintf(tmp, "Client Info: \n ip-address: %s, \t port: %d\n, packet_sent: %zu\n packet_received: %zu\n",
//            diagnostics->info->ip_address, diagnostics->info->client_port, diagnostics->packet_sent, diagnostics->packet_received);
//    sz = dc_write(env, err, STDOUT_FILENO, tmp, strlen(tmp));
    close(fd);
}

static void initialize_udpPacket(const struct dc_posix_env *env, struct dc_error *err, void* arg)
{
    struct udp_packet* udp_packet;

    udp_packet = (struct udp_packet *)arg;

    udp_packet->diagnostics = dc_calloc(env, err, 1, sizeof (struct diagnostics));
    udp_packet->diagnostics->info = dc_calloc(env, err, 1, sizeof (struct client_info));
}
#define START true
#define MESSAGE_SEPARATOR ":"

static bool parse_initial_message(const struct dc_posix_env *env, struct dc_error *err, void* arg, char *message)
{
    bool res;
    char *temp;
    char *tempPt;
    size_t tempSize;
    char *start_str;
    char *rest;
    char *num_packets_sent_str;
    struct udp_packet* udpPacket;


    udpPacket = (struct udp_packet *)arg;

    res = false;
    temp = dc_strdup(env, err, message);
    tempPt = temp;
    tempSize = dc_strlen(env, temp);
    start_str = dc_strtok_r(env, temp, MESSAGE_SEPARATOR, &temp);
    num_packets_sent_str = temp;
    dc_strtok_r(env, temp, MESSAGE_SEPARATOR, &temp);

    if (dc_strcmp(env, start_str, START_MESSAGE) == 0)
    {
        udpPacket->diagnostics->packet_sent = (size_t) dc_strtol(env, err, num_packets_sent_str, &rest, 10);
        udpPacket->list_packets = dc_calloc(env, err, udpPacket->diagnostics->packet_sent, sizeof(char*));
        udpPacket->diagnostics->packet_size = (size_t) dc_strtol(env, err, temp, &rest, 10);
        res = START;
    }
    dc_free(env, tempPt, tempSize);
    return res;
}

static void reset_udp(const struct dc_posix_env* env, struct dc_error* err, struct udp_packet* udpPacket);

/**
 * Initial incoming information.
 *
 * @param env
 * @param err
 * @param client_socket_fd
 */
#define MAX_INIT_MSG_SIZE 50
void echo(const struct dc_posix_env *env, struct dc_error *err, int client_socket_fd, uint16_t port)
{
    char init_message[MAX_INIT_MSG_SIZE];
    ssize_t len;
    struct udp_packet udpPacket;

    initialize_udpPacket(env, err, &udpPacket);
    //create a thread to handle?
    //parse incoming messages.
        //for START message, num_packets, size_packets
    len = dc_read(env, err, client_socket_fd, init_message, MAX_INIT_MSG_SIZE);
    init_message[len] = '\0';

    if (parse_initial_message(env, err, &udpPacket, init_message)) //if true, start the message
    {
        connect_to_udp(env, err, port, &udpPacket, client_socket_fd);
    }
//    reset_udp(env, err, &udpPacket);
}

static void free_list_packets(const struct dc_posix_env* env, struct dc_error* err, struct udp_packet* udpPacket);
static void free_list_diagnostics(const struct dc_posix_env* env, struct dc_error* err, struct udp_packet* udpPacket);

static void reset_udp(const struct dc_posix_env* env, struct dc_error* err, struct udp_packet* udpPacket)
{
    if (udpPacket)
    {
        free_list_packets(env, err, udpPacket);
        free_list_diagnostics(env, err, udpPacket);
    }
}

static void free_list_packets(const struct dc_posix_env* env, struct dc_error* err, struct udp_packet* udpPacket)
{
    int i = 0;
    while (udpPacket->list_packets[i])
    {
        dc_free(env, udpPacket->list_packets[i], dc_strlen(env, udpPacket->list_packets[i]) + 1);
        i++;
    }
}
//static void free_packet_number_array(const struct dc_posix_env* env, struct dc_error* err, struct udp_packet* udpPacket)
//{
//    if (udpPacket->packet_number_array)
//    {
//        size_t size = udpPacket->diagnostics->packet_sent * sizeof(char*);
//        dc_free(env, udpPacket->packet_number_array, size);
//    }
//    udpPacket->packet_number_array = NULL;
//}
static void free_list_diagnostics(const struct dc_posix_env* env, struct dc_error* err, struct udp_packet* udpPacket)
{
    if(udpPacket->diagnostics)
    {
        if (udpPacket->diagnostics->info)
        {
            if (udpPacket->diagnostics->info->ip_address)
            {
                dc_free(env, udpPacket->diagnostics->info->ip_address, dc_strlen(env, udpPacket->diagnostics->info->ip_address));
                udpPacket->diagnostics->info->ip_address = NULL;
            }
            dc_free(env, udpPacket->diagnostics->info, sizeof(struct client_info));
            udpPacket->diagnostics->info = NULL;
        }
        dc_free(env, udpPacket->diagnostics, sizeof(struct diagnostics));
        udpPacket->diagnostics = NULL;
    }
}

static void do_shutdown(const struct dc_posix_env *env, __attribute__ ((unused)) struct dc_error *err, __attribute__ ((unused)) void *arg)
{
    DC_TRACE(env);
}

static void
do_destroy_settings(const struct dc_posix_env *env, __attribute__ ((unused)) struct dc_error *err, void *arg)
{
    struct application_settings *app_settings;

    DC_TRACE(env);
    app_settings = arg;
    dc_freeaddrinfo(env, app_settings->address);
}
