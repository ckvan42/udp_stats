
#ifndef DC_UDP_CONNECTIONS_H
#define DC_UDP_CONNECTIONS_H

#include "udp_structures.h"

#define START_MESSAGE "START"
#define END_MESSAGE "END"

void create_tcp_connection(  const struct dc_posix_env *env, struct dc_error *err, struct connection* connection,
                                    const char* hostname, int family, const char* ip_version, in_port_t port);
void create_udp_connection(  const struct dc_posix_env *env, struct dc_error *err, struct connection* connection,
                                    const char* hostname, int family, const char* ip_version, in_port_t port);

void destroy_tcp_connection(const struct dc_posix_env *env, struct dc_error *err, struct connection* connection);
void destroy_udp_connection(const struct dc_posix_env *env, struct dc_error *err, struct connection* connection);
#endif //DC_UDP_CONNECTIONS_H
