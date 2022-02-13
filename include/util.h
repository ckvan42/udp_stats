//
// Created by Giwoun Bae on 2022-02-12.
//

#ifndef DC_UDP_UTIL_H
#define DC_UDP_UTIL_H

#include "udp_structures.h"



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
void count_min_max_out_of_order(const size_t *array, size_t numberOfPackets, size_t *min, size_t *max);


double calculate_average_packets_lost(size_t numberOfPackets, size_t totalPacketsSent);

#endif //DC_UDP_UTIL_H
