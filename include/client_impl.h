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
    SEND_TCP_START = DC_FSM_USER_START,     /**< the send initial TCP_information */             //  2
    SEND_UDP,                               /**< start sending upd packets */         //  4
    SEND_TCP_END,                           /**< send ending TCP packet */            //  5
    ERROR,                                  /**< handle errors */                 //  9
};

int run_udp_diagnostics(const struct dc_posix_env *env,
                        struct dc_error* err, const char* hostname, int family, const char* ip_version,
                        double start_time, in_port_t port, uint16_t delay, uint16_t num_packets, uint16_t packet_size);

int send_tcp_start(const struct dc_posix_env *env, struct dc_error *err, void* arg);
int send_udp(const struct dc_posix_env *env, struct dc_error *err, void* arg);
int handler_error(const struct dc_posix_env *env, struct dc_error *err, void* arg);
int send_tcp_end(const struct dc_posix_env *env, struct dc_error *err, void* arg);

#endif //DC_UDP_CLIENT_IMPL_H
