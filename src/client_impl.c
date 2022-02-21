#include <dc_posix/dc_string.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/sys/dc_socket.h>
#include "connections.h"
#include "client_impl.h"


int run_udp_diagnostics(const struct dc_posix_env *env,
                        struct dc_error* err, const char* hostname, int family, const char* ip_version,
                        const char* start_time, in_port_t port, uint16_t delay, uint16_t num_packets, uint16_t packet_size)
{
    int ret_val;

    struct dc_fsm_info *fsm_info;

    static struct dc_fsm_transition transitions[] = {
            {DC_FSM_INIT,       INIT_STATE,         init_states},
//            {INIT_STATE,        SEND_TCP_START,     send_tcp_start},
//            {INIT_STATE,        ERROR,              handler_error},
//            {SEND_TCP_START,    SEND_UDP,           send_udp},
//            {SEND_TCP_START,    ERROR,              handler_error},
//            {SEND_UDP,          ERROR,              handler_error},
//            {SEND_UDP,          SEND_TCP_END,       send_tcp_end},
//            {SEND_TCP_END,      DESTROY_STATE,      destroy_state},
//            {ERROR,             DESTROY_STATE,      destroy_state}
            {DESTROY_STATE,     DC_FSM_EXIT,        NULL}
    };

    ret_val = EXIT_SUCCESS;
    fsm_info = dc_fsm_info_create(env, err, "UDP_DIAGNOSTICS");

    if (dc_error_has_no_error(err))
    {
        struct client_udp states;

        int from_state;
        int to_state;

        dc_memset(env, &states, '\0', sizeof (struct client_udp));

        states.connections = dc_calloc(env, err, 1, sizeof(struct connection));

        create_tcp_connection(env, err, states.connections, hostname, family, ip_version, port);
        create_udp_connection(env, err, states.connections, hostname, family, ip_version, port);
        //save start time
        states.start_time = dc_strdup(env, err, start_time);
        states.delay = delay;
        states.num_packets = num_packets;
        states.packet_size = packet_size;

        ret_val = dc_fsm_run(env, err, fsm_info, &from_state, &to_state, &states, transitions);
        dc_fsm_info_destroy(env, &fsm_info);

//        destroy_tcp_connection();
//        destroy_udp_connection();
    }

    return ret_val;
}

/**
 * Initialize udp_packet
 *
 * @param env
 * @param err
 * @param arg
 * @return
 */
int init_states(const struct dc_posix_env *env, struct dc_error *err, void* arg)
{
    struct client_udp *clientUdp;

    clientUdp = (struct client_udp*)arg;

    printf("TESTING\n");

    return SEND_TCP_START;
}



