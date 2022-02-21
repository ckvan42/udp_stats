#ifndef DC_UDP_UDP_STRUCTURES_H
#define DC_UDP_UDP_STRUCTURES_H

#include <machine/types.h> /* __uint16_t */
#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>

#define DEFAULT_UDP_PORT 4981
#define DEFAULT_UDP_DELAY 0
#define DEFAULT_UDP_NUM_PACKETS 100
#define DEFAULT_UDP_PACKET_SIZE 100

typedef __uint16_t              in_port_t;

/**
 *  Connection information for UDP and TCP connections.
 */
struct connection
{
    int                 tcp_sockfd;     //used for sending the initial data
    struct sockaddr *   tcp_ai_addr;
    int                 udp_sockfd;     //used for sending the packets
    struct sockaddr *   udp_ai_addr;
};

/**
 * Client information used for displaying statistics.
 */
struct client_info
{
    char *              ip_address;     //ip_address of the client
    in_port_t           client_port;    //port number for the client
};

/**
 *
 */
struct diagnostics
{
    struct client_info* info;
    size_t packet_sent;
    size_t packet_received;
    size_t packet_lost;
    size_t packet_out_of_order;
    size_t min_lost;
    size_t max_lost;
    size_t min_out_of_order;
    size_t max_out_of_order;
    time_t timestamp_first;
    time_t timestamp_last;
};

/**
 * This struct will be passed around on server side.
 */
struct udp_packet
{
    //array of packets received.
    char** list_packets;
    size_t *packet_number_array;
    struct diagnostics* diagnostics;
};

struct client_udp
{
    struct connection* connections;
    double start_time;
    uint16_t delay;
    uint16_t num_packets;
    uint16_t packet_size;
};

#endif // DC_UDP_UDP_STRUCTURES_H
