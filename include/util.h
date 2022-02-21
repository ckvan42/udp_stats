//
// Created by Giwoun Bae on 2022-02-12.
//

#ifndef DC_UDP_UTIL_H
#define DC_UDP_UTIL_H

#include "udp_structures.h"
#include "dc_posix/dc_posix_env.h"
#include "dc_error/error.h"

/**
 * count the minimum and maximum dropped packets in sequence.
 *
 * @param array
 * @param numberOfPackets total number of packets
 * @param min number of dropped packets in sequence
 * @param max number of dropped packets in sequence
 */
void count_min_max_dropped(const size_t *array, size_t numberOfPackets, size_t *min, size_t *max);

/**
 * count the minimum and maximum out of order packets in sequence.
 *
 * @param array
 * @param numberOfPackets
 * @param min
 * @param max
 */
void count_min_max_out_of_order(const struct dc_posix_env* env, const size_t *array, size_t numberOfPackets, size_t *min, size_t *max);

/**
 * Calculate the average number of packets lost.
 *
 * @param numberOfPackets the number of packets received.
 * @param totalPacketsSent the original number of packets.
 * @return the average number of packets lost.
 */
double calculate_average_packets_lost(size_t numberOfPackets, size_t totalPacketsSent);


/**
 * Calculate the start_delay from the start_time.
 *
 * @param env
 * @param err
 * @param start_time
 * @return number of seconds to sleep before the program starts sending the UDP packets.
 *
 */
unsigned int calculate_start_delay(const struct dc_posix_env* env, struct dc_error *err, const char* start_time);

#endif //DC_UDP_UTIL_H
