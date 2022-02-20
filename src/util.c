//
// Created by Giwoun Bae on 2022-02-12.
//

#include <libc.h>
#include <dc_posix/dc_string.h>
#include "util.h"

static void array_swap(size_t *first, size_t *second);


/**
 * Process the packets for dropped, out-of-order packets in sequence.
 *
 * @param env the posix env object.
 * @param err the error object.
 * @param diagnostics struct to access the diagnostics
 */
void process_packets_in_sequence(const struct dc_posix_env *env, struct dc_error *err, struct udp_packet* udpPacket)
{
    size_t min_dropped;
    size_t max_dropped;
    size_t min_out_of_order;
    size_t max_out_of_order;

    size_t temp_dropped;
    size_t temp_out_of_order;
    //in one looping, find the min dropped, max dropped etc

    //lets find the min dropped in sequence
    temp_dropped = 0;
    max_dropped = 0;
    min_dropped = udpPacket->diagnostics->packet_received;
    for (size_t i = 0; i < udpPacket->diagnostics->packet_received; i++)
    {
        if (i != udpPacket->packet_number_array[i])
        {
            temp_dropped++;
        }
        else
        {
            if ((temp_dropped < min_dropped) && (temp_dropped > 0))
            {
                min_dropped = temp_dropped;
            }
            else if (temp_dropped > max_dropped)
            {
                max_dropped = temp_dropped;
            }
        }
    }
}

void count_min_max_dropped(const size_t *array, size_t numberOfPackets, size_t *min, size_t *max)
{
    // loop through the array
    size_t temp_max;
    size_t temp_min;

    size_t difference;

    temp_max = 0;
    temp_min = numberOfPackets;
    for (size_t i = 0; i < numberOfPackets - 1; ++i)
    {
        difference = array[i + 1] - array[i];
        if (difference > 1) //then there is a dropped packet.
        {
            if (difference < temp_min)
            {
                temp_min = difference;
            } else if (difference > temp_max)
            {
                temp_max = difference;
            }
        }
    }
    // find the difference between adjacent elements.
    // if the difference is smaller than the minimum,
    //Update the temp_minimum.
            //else if the difference is bigger than the maximum,
            //Update the temp_maximum.

    // min is temp_min - 1
    // max is temp_max - 1
    *min = temp_min - 1;
    *max = temp_max - 1;
}

double calculate_average_packets_lost(size_t numberOfPackets, size_t totalPacketsSent)
{
    double result;

    if (totalPacketsSent < 0)
    {
        result  = 0;
    }
    else
    {
        result = (double) (numberOfPackets)/ (double) (totalPacketsSent);
    }
    return result;
}



void count_min_max_out_of_order(const struct dc_posix_env* env, const size_t *array, size_t numberOfPackets, size_t *min, size_t *max)
{
    size_t count;
    size_t temp_min;
    size_t temp_max;
    size_t temp_array[numberOfPackets];

    count = 0;
    temp_min = numberOfPackets;
    temp_max = 0;
    dc_memcpy(env, temp_array, array, sizeof(temp_array));
    //for each element,
        // do bubble sort?
            //count number of swaps.
    for (size_t i = 0; i <numberOfPackets - 1; ++i)
    {
        if (temp_array[i] < temp_array[i + 1])
        {
            count = 0;
        } else {
            while (temp_array[i] > temp_array[i + 1])
            {
                count++;
                i++;
            }

            if (count > 0)
            {
                if (count < temp_min)
                {
                    temp_min = count;
                } else if (count > temp_max)
                {
                    temp_max = count; //for
                }
            }
        }
    }

    *min = temp_min;
    *max = temp_max;
}
