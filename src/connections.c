#include <dc_posix/dc_netdb.h>
#include <assert.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/sys/dc_socket.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_unistd.h>
#include <netinet/tcp.h>
#include "connections.h"

void create_tcp_connection(  const struct dc_posix_env *env, struct dc_error *err, struct connection* connection,
                                    const char* hostname, int family, const char* ip_version, in_port_t port)
{
    struct addrinfo hints;
    struct addrinfo *result;
    uint16_t converted_port;

    dc_memset(env, &hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    dc_getaddrinfo(env, err, hostname, NULL, &hints, &result);

    if(dc_error_has_error(err))
    {
        return;
    }

    connection->tcp_sockfd = dc_socket(env, err, result->ai_family, result->ai_socktype, result->ai_protocol);

    if(dc_error_has_error(err))
    {
        return;
    }

    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    converted_port = htons(port);

    if(dc_strcmp(env, ip_version, "IPv4") == 0)
    {
        struct sockaddr_in *sockaddr;

        sockaddr = (struct sockaddr_in *)result->ai_addr;
        sockaddr->sin_port = converted_port;
    }
    else
    {
        if(dc_strcmp(env, ip_version, "IPv6") == 0)
        {
            struct sockaddr_in6 *sockaddr;

            sockaddr = (struct sockaddr_in6 *)result->ai_addr;
            sockaddr->sin6_port = converted_port;
        }
        else
        {
            assert("Can't get here" != NULL);
        }
    }

    connection->tcp_ai_addr = dc_calloc(env, err, 1, sizeof(struct sockaddr));
    dc_memcpy(env, connection->tcp_ai_addr, result->ai_addr, sizeof(struct sockaddr));
    dc_connect(env, err, connection->tcp_sockfd, connection->tcp_ai_addr, sizeof(struct sockaddr));
}

void create_udp_connection(  const struct dc_posix_env *env, struct dc_error *err, struct connection* connection,
                                    const char* hostname, int family, const char* ip_version, in_port_t port)
{
    struct    sockaddr_in  servaddr;

    connection->udp_sockfd = dc_socket(env, err, AF_INET, SOCK_DGRAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    connection->udp_ai_addr = dc_calloc(env, err, 1, sizeof(struct sockaddr));
    dc_memcpy(env, connection->udp_ai_addr, &servaddr, sizeof(struct sockaddr));
}


void destroy_tcp_connection(const struct dc_posix_env *env, struct dc_error *err, struct connection* connection)
{
    dc_close(env, err, connection->tcp_sockfd);
}

void destroy_udp_connection(const struct dc_posix_env *env, struct dc_error *err, struct connection* connection)
{
    dc_close(env, err, connection->udp_sockfd);
}
