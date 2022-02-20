//
// Created by Giwoun Bae on 2022-02-19.
//

#ifndef DC_UDP_CLIENT_IMPL_H
#define DC_UDP_CLIENT_IMPL_H

#include <dc_posix/dc_posix_env.h>
#include <dc_fsm/fsm.h>
#include "udp_structures.h"

enum client_udp_states
{
    INIT_STATE = DC_FSM_USER_START, /**< the initial state */             //  2
    SEND_TCP_START,                  /**< accept user input */             //  3
    SEND_UDP,              /**< separate the commands */         //  4
    SEND_TCP_END,                 /**< parse the commands */            //  5
    ERROR,                          /**< handle errors */                 //  9
    DESTROY_STATE,                  /**< destroy the state */             // 10
};

int run_udp_diagnostics(const struct dc_posix_env *env,
        struct dc_error* err, const char* hostname, int family, const char* ip_version,
                const char* start_time, in_port_t port, uint16_t delay, uint16_t num_packets, uint16_t packet_size);

int init_states(const struct dc_posix_env *env, struct dc_error *err, void* arg);







#endif //DC_UDP_CLIENT_IMPL_H
