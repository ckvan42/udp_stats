#include <dc_posix/dc_string.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/sys/dc_socket.h>
#include <dc_posix/dc_unistd.h>
#include "connections.h"
#include "client_impl.h"

static struct packet* createEachPacket(const struct dc_posix_env *env, struct dc_error *err, uint16_t id, uint16_t packet_size);
static void destroy_packet(const struct dc_posix_env *env, struct dc_error *err, struct packet** packet, uint16_t packet_size);

int run_udp_diagnostics(const struct dc_posix_env *env,
                        struct dc_error* err, const char* hostname, int family, const char* ip_version,
                        double start_time, in_port_t port, uint16_t delay, uint16_t num_packets, uint16_t packet_size)
{
    int ret_val;

    struct dc_fsm_info *fsm_info;

    static struct dc_fsm_transition transitions[] = {
            {DC_FSM_INIT,       PREP_UDP,           prep_udp_packets},
            {PREP_UDP,          SEND_TCP_START,     send_tcp_start},
            {SEND_TCP_START,    SEND_UDP,           send_udp},
            {SEND_TCP_START,    ERROR,              handler_error},
            {SEND_UDP,          ERROR,              handler_error},
            {SEND_UDP,          SEND_TCP_END,       send_tcp_end},
            {SEND_TCP_END,      DESTROY_STATE,      destroy_state},
            {ERROR,             DESTROY_STATE,      destroy_state},
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
        states.start_time_delay = start_time;
        states.delay = delay;
        states.num_packets = num_packets;
        states.packet_size = packet_size;

        ret_val = dc_fsm_run(env, err, fsm_info, &from_state, &to_state, &states, transitions);
        dc_fsm_info_destroy(env, &fsm_info);

        destroy_tcp_connection(env, err, states.connections);
        destroy_udp_connection(env, err, states.connections);
        dc_free(env, states.connections, sizeof (struct connection));
    }

    return ret_val;
}

int prep_udp_packets(const struct dc_posix_env *env, struct dc_error *err, void* arg)
{

    struct client_udp* clientUdp;
    uint16_t id;

    clientUdp = (struct client_udp*) arg;

    clientUdp->packets = dc_calloc(env, err, clientUdp->num_packets, sizeof(struct packet*));
    //create each packet and save to packets
    for (id = 0; id < clientUdp->num_packets; id++)
    {
        clientUdp->packets[id] = createEachPacket(env, err, id, clientUdp->packet_size);
        if (dc_error_has_error(err))
        {
            return ERROR;
        }
    }
    return SEND_TCP_START;
}

static struct packet* createEachPacket(const struct dc_posix_env *env, struct dc_error *err, uint16_t id, uint16_t packet_size) {
    struct packet *each;

    each = dc_calloc(env, err, 1, sizeof(struct packet));
    if (dc_error_has_no_error(err))
    {
        each->id = id;
        each->size = packet_size;
        each->data = dc_calloc(env, err, packet_size + 1, sizeof(uint8_t));
        if(dc_error_has_no_error(err))
        {
            srandom(time(0));
            for( int i = 0; i < packet_size; ++i){
                each->data[i] = (uint8_t) ('0' + rand() % 72); // starting on '0', ending on '}'
            }
        }
    }
    return each;
}

#define TCP_MESSAGE_BUFFER 100

int send_tcp_start(const struct dc_posix_env *env, struct dc_error *err, void* arg)
{
    struct client_udp* clientUdp;
    struct connection* connection;
    char tcp_start_message[TCP_MESSAGE_BUFFER];

    clientUdp = (struct client_udp*) arg;
    connection = clientUdp->connections;
    sprintf(tcp_start_message, "%s:%d:%d\n", START_MESSAGE, clientUdp->num_packets, clientUdp->packet_size);
    dc_write(env, err, connection->tcp_sockfd, tcp_start_message, dc_strlen(env, tcp_start_message) + 1);
    if(dc_error_has_error(err))
    {
        return ERROR;
    }
    return SEND_UDP;
}

int send_udp(const struct dc_posix_env *env, struct dc_error *err, void* arg)
{
    struct client_udp* clientUdp;
    struct connection* connection;
    size_t each_packet_size_total;
    struct packet* packet;

    clientUdp = (struct client_udp*) arg;
    connection = clientUdp->connections;

    each_packet_size_total = clientUdp->packet_size + sizeof (uint16_t) * 2 + 1; // for id and size and + 1 for null.

    char each_packet[each_packet_size_total];

    dc_sleep(env, (unsigned int) clientUdp->start_time_delay);

    for (int i = 0; i < clientUdp->num_packets; i++)
    {
        packet = clientUdp->packets[i];
        sprintf(each_packet, "%d:%s", packet->id, packet->data);
        dc_sendto(env, err, connection->udp_sockfd, each_packet, dc_strlen(env, each_packet) + 1, 0, connection->udp_ai_addr, sizeof (*(connection->udp_ai_addr)));
        dc_sleep(env, clientUdp->delay);
    }

    if(dc_error_has_error(err))
    {
        return ERROR;
    }
    return SEND_TCP_END;
}

int handler_error(const struct dc_posix_env *env, struct dc_error *err, void* arg)
{
    return DESTROY_STATE;
}

int send_tcp_end(const struct dc_posix_env *env, struct dc_error *err, void* arg)
{
    struct client_udp* clientUdp;
    struct connection* connection;
    const char* tcp_end_message;

    clientUdp = (struct client_udp*) arg;
    connection = clientUdp->connections;
    tcp_end_message = END_MESSAGE;

    dc_sleep(env, 5);

    dc_write(env, err, connection->tcp_sockfd, tcp_end_message, dc_strlen(env, tcp_end_message) + 1);
    if(dc_error_has_error(err))
    {
        return ERROR;
    }
    return DESTROY_STATE;
}


int destroy_state(const struct dc_posix_env *env, struct dc_error *err, void* arg)
{
    struct client_udp* clientUdp;

    clientUdp = (struct client_udp*) arg;

    if (clientUdp)
    {
        destroy_packet(env, err, clientUdp->packets, clientUdp->packet_size);
    }
    return DC_FSM_EXIT;
}

static void destroy_packet(const struct dc_posix_env *env, struct dc_error *err, struct packet** packet, uint16_t packet_size)
{
    int i;

    i=0;
    while (!(packet[i]))
    {
        dc_free(env, packet[i], packet_size);
        i++;
    }
}





