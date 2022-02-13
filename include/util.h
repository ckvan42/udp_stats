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
 * @param numberOfPackets
 * @param min
 * @param max
 */
void count_min_max_dropped(const size_t *array, size_t numberOfPackets, size_t *min, size_t *max);

#endif //DC_UDP_UTIL_H
