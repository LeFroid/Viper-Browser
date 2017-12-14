#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <sys/time.h>
typedef unsigned long long timestamp_t;

static timestamp_t get_timestamp()
{
    struct timeval now;
    gettimeofday (&now, 0);
    return  now.tv_usec + (timestamp_t)now.tv_sec * 1000000;
}


#endif // TIMESTAMP_H
