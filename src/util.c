//
// Created by Giwoun Bae on 2022-02-12.
//

#include <libc.h>
#include <dc_posix/dc_string.h>
#include "util.h"
#include <time.h>
#include <dc_posix/dc_stdlib.h>

#define HOUR_SEPARATOR ":"
/**
 *
 * @param first
 * @param second
 */
static void sort_array(size_t *arr, size_t length);

/**
 * Calculate the start_delay from the start_time.
 *
 * @param env
 * @param err
 * @param start_time
 * @return number of seconds to sleep before the program starts sending the UDP packets.
 *
 */
static double parse_time(const struct dc_posix_env* env, struct dc_error *err, const char* start_time);

static size_t * createArray(const struct dc_posix_env* env, struct dc_error *err, void * arg, size_t *array);

void process_udp(const struct dc_posix_env* env, struct dc_error *err, void *arg)
{
    struct udp_packet* udpPackets;
    size_t num_packets_received;


    udpPackets = (struct udp_packet*) arg;
    num_packets_received = udpPackets->diagnostics->packet_received;
    size_t array[num_packets_received];
    size_t sorted[num_packets_received];

    if (num_packets_received > 0)
    {
        //create size_t array with id's only.
        createArray(env, err, udpPackets, &array[0]);
        dc_memcpy(env, sorted, array, sizeof(array));
        sort_array(sorted, num_packets_received);
        count_min_max_dropped(sorted, num_packets_received, &udpPackets->diagnostics->min_lost, &udpPackets->diagnostics->max_lost);
        count_min_max_out_of_order(env, array, num_packets_received, &udpPackets->diagnostics->min_out_of_order, &udpPackets->diagnostics->max_out_of_order);
        udpPackets->diagnostics->average_packet_lost = calculate_average_packets_lost(num_packets_received, udpPackets->diagnostics->packet_sent);
    }
  }


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

void count_min_max_dropped(size_t *array, size_t numberOfPackets, size_t *min, size_t *max)
{
    // loop through the array
    size_t temp_max;
    size_t temp_min;

    size_t difference;
    //sort the array
    sort_array(array, numberOfPackets);
    temp_max = 0;
    temp_min = numberOfPackets;
    for (size_t i = 0; i < numberOfPackets - 1; ++i)
    {
        difference = array[i + 1] - array[i];
        if (difference > 1) //then there is a dropped packet.
        {
            if (difference < temp_min)
            {
                temp_min = difference - 1;
            } else if (difference > temp_max)
            {
                temp_max = difference - 1;
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
    *min = temp_min;
    *max = temp_max;
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

unsigned int calculate_start_delay(const struct dc_posix_env* env, struct dc_error *err, const char* start_time)
{
    //if start-time is NULL, then delay is zero.
    unsigned int  delay_in_seconds;

    if (!start_time)
    {
        delay_in_seconds = 0;
    } else
    {
        delay_in_seconds =(unsigned int) parse_time(env, err, start_time);
    }

    return delay_in_seconds;
}

static double parse_time(const struct dc_posix_env* env, struct dc_error *err, const char* start_time)
{
    //the start_time will be in hh:mm
    char *temp;
    char *tempPt;
    size_t tempSize;
    char *hour_str;
    char *minutes_str;

    temp = dc_strdup(env, err, start_time);
    tempPt = temp;
    tempSize = dc_strlen(env, temp);
    //getHour
    hour_str = dc_strtok_r(env, temp, HOUR_SEPARATOR, &temp);
    //getMinute
    minutes_str = temp;

    struct tm *start_time_info;

    //getCurrentTime
    time_t rawTime;
    struct tm *info;
    time(&rawTime);
    //this is the current time
    info = localtime( &rawTime );

    //this is the start time.
    start_time_info = localtime(&rawTime);

    start_time_info->tm_hour = atoi(hour_str);
    start_time_info->tm_min = atoi(minutes_str);
    start_time_info->tm_sec = 0;

    //compare with current time and return the result.
    time_t startTime;
    startTime = mktime(start_time_info);

    double difference;
    difference = difftime(startTime, rawTime);

    if (difference < 0)
    {
        difference = 0;
    }
    printf("Time: %f\n", difference);
    dc_free(env, tempPt, tempSize);
    return difference;
}


static size_t * createArray(const struct dc_posix_env* env, struct dc_error *err, void * arg, size_t *array)
{
    struct udp_packet* udpPackets;
    size_t num_packets_received;
    char * temp;
    udpPackets = (struct udp_packet*) arg;
    num_packets_received = udpPackets->diagnostics->packet_received;

    for (size_t i = 0; i < num_packets_received; i++)
    {
        temp = dc_strdup(env, err, udpPackets->list_packets[i]);
        char *pt = temp;
        array[i] = (size_t) dc_strtol(env, err, temp, NULL, 10);
        free(pt);
    }
    return array;
}


static void sort_array(size_t *arr, size_t length)
{
    size_t temp;

    for (size_t i = 0; i < length; i++) {
        for (size_t j = i+1; j < length; j++) {
            if(arr[i] > arr[j]) {
                temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
    }
}
