/*
MIT License

Copyright (c) 2019 dreamz-group

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "include/select_cb.h"

#define UNUSED_VALUE_ID -1
#define MAX_CB_TIMERS   20

typedef struct
{
    int fd;
    cb_func_p fp;
    void *user_data;
    bool in_use;
} cb_type;

typedef struct
{
    cb_to_func_p      fp;
    void*             ud;
    time_t            to;
    bool              repeat;
    time_t            sec;    // Only used if repeat is true, stores the seconds to sleep.
} cb_timer_t;

static int               cb_max_used = 0;
static cb_type           internal_list[20];

static int               cb_timer_max_used = 0;
static cb_timer_t        internal_timers[MAX_CB_TIMERS];

static bool              looping = true;

int _cb_wakeup_loop()
{
    // Send a wakeup to fall out of the select
    return 0;
}

int cb_init()
{
    unsigned int x;
    cb_timer_max_used = 0;
    for( x=0; x < MAX_CB_TIMERS; ++x )
    {
        internal_timers[x].fp     = NULL;
        internal_timers[x].ud     = NULL;
        internal_timers[x].to     = UNUSED_VALUE_ID;
        internal_timers[x].repeat = false;
        internal_timers[x].sec    = 0;
    }
    
    cb_max_used = 0;
    memset(internal_list, 0, sizeof(internal_list));
    for (x = 0; x < sizeof(internal_list) / sizeof(cb_type); x++) 
    {
        internal_list[x].fd = -1;
    }
    return 0;
}

static int _cb_to_epoch_add(time_t wake_uptime_in_epoch_sec, cb_to_func_p fp, void *user_data, time_t sec)
{
    unsigned int x;
    for(x=0; x < MAX_CB_TIMERS; ++x )
    {
        if( internal_timers[x].to == UNUSED_VALUE_ID )
        {
            internal_timers[x].to     = wake_uptime_in_epoch_sec;
            internal_timers[x].fp     = fp;
            internal_timers[x].ud     = user_data;
            if( sec != 0 )
            {
                internal_timers[x].repeat = true;
                internal_timers[x].sec    = sec;
            }
            
            if( cb_timer_max_used <= x )
            {
                cb_timer_max_used = x + 1;
            }
            return 0;
        }
    }
    // Note: here we should make sure select wakesup.. On the otherhand if one threaded it is woken up...
    return -1;
}

int cb_to_epoch_add(time_t wake_uptime_in_epoch_sec, cb_to_func_p fp, void *user_data)
{
    return _cb_to_epoch_add(wake_uptime_in_epoch_sec, fp, user_data, 0);
}

int cb_to_repeat_add(time_t in_sec, cb_to_func_p fp, void *user_data)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return _cb_to_epoch_add(tv.tv_sec + in_sec, fp, user_data, in_sec);
}

// Timeout version...
int cb_to_add(time_t wake_uptime_from_now_in_sec, cb_to_func_p fp, void *user_data)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return _cb_to_epoch_add(tv.tv_sec + wake_uptime_from_now_in_sec, fp, user_data, 0);
}

void call_timeout_callback()
{
    unsigned int x;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    for(x=0; x < cb_timer_max_used; ++x )
    {
        if( internal_timers[x].to == UNUSED_VALUE_ID )
        {
            continue;
        }
        if( internal_timers[x].to <= tv.tv_sec )
        {            
            cb_to_func_p  fp = internal_timers[x].fp;
            void*         ud = internal_timers[x].ud;
            if( internal_timers[x].repeat )
            {
                internal_timers[x].to = tv.tv_sec + internal_timers[x].sec;
            }
            else
            {
                internal_timers[x].to = UNUSED_VALUE_ID;
                internal_timers[x].fp = NULL;
                internal_timers[x].ud = NULL;
            }
            fp(tv.tv_sec, ud);
        }
    }
}

void cb_to_fire( cb_to_func_p fp, time_t call )
{
    unsigned int x;
    if( call != 0 )
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        call = tv.tv_sec + call;
    }

    for(x=0; x < cb_timer_max_used; ++x )
    {
        if( internal_timers[x].to == UNUSED_VALUE_ID )
        {
            continue;
        }
        if( internal_timers[x].fp == fp )
        {
            internal_timers[x].to = call;
        }
    }
}

// Timeout version...
int cb_to_del(cb_to_func_p fp)
{
    unsigned int x;
    for(x=0; x < cb_timer_max_used; ++x )
    {
        if( internal_timers[x].fp == fp )
        {
            internal_timers[x].to     = UNUSED_VALUE_ID;
            internal_timers[x].fp     = NULL;
            internal_timers[x].ud     = NULL;
            internal_timers[x].repeat = false;
            return 0;
        }
    }
    return -1;
}

int cb_add(int fd, cb_func_p fp, void *user_data)
{
    unsigned int x;

    for (x = 0; x < sizeof(internal_list) / sizeof(cb_type); x++) 
    {
        if (!internal_list[x].in_use) 
        {
            // use this instance...
            internal_list[x].in_use    = true;
            internal_list[x].fd        = fd;
            internal_list[x].fp        = fp;
            internal_list[x].user_data = user_data;

            // Wake up the loop to make sure our new cb is added.
            _cb_wakeup_loop();
            if( cb_max_used <= x )
            {
                cb_max_used = x + 1; 
            }
            return 0;
        }
    }
    fprintf(stderr, "ERROR - fd could not be added as the list is full current MAX is %zd.\n", sizeof(internal_list) / sizeof(cb_type));
    return -1;
}

int cb_del(int fd)
{
    unsigned int x;
    for (x = 0; x < cb_max_used; ++x) 
    {
        if (internal_list[x].fd == fd && internal_list[x].in_use)
        {
            // use this instance...
            internal_list[x].in_use    = false;
            internal_list[x].fd        = -1;
            internal_list[x].fp        = NULL;
            internal_list[x].user_data = NULL;

            // Wake up the loop to make sure our new cb is deleted.
            _cb_wakeup_loop();

            return 0;
        }
    }
    fprintf(stderr, "WARNING - fd is not found in the list of used.\n");
    return -1;
}

int cb_stop()
{
    looping = false;
    _cb_wakeup_loop();
    return 0;
}

static int calc_timeout(struct timeval* out)
{
    unsigned int x;
    out->tv_sec  = 0;
    out->tv_usec = 10000;

    time_t to_to = UNUSED_VALUE_ID;
    for(x=0; x < cb_timer_max_used; ++x )
    {
        if( internal_timers[x].to != UNUSED_VALUE_ID )
        {
            if( to_to == UNUSED_VALUE_ID || to_to > internal_timers[x].to )
            {
                to_to = internal_timers[x].to;
            }
        }
    }
    if( to_to == UNUSED_VALUE_ID )
    {
        return false;
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if( to_to > tv.tv_sec )
    {
        out->tv_sec  = to_to - tv.tv_sec;
        out->tv_usec = 0;
    }
    return true;
}

int _cb_poll()
{
    fd_set rfds;
    FD_ZERO(&rfds);
    int max_fd = -1;
    int retval;
    int x;
    for (x = 0; x < cb_max_used; x++)
    {
        if (internal_list[x].in_use)
        {
            max_fd = internal_list[x].fd > max_fd ? internal_list[x].fd : max_fd;
            FD_SET(internal_list[x].fd, &rfds);
        }
    }

    if (max_fd == -1)
    {
        // Check what callback that should be invoked
        call_timeout_callback();
        sleep(1);
        printf("Error, no fd in select...\n");
        return -1;
    }

    // Wait for action...
    struct timeval tv;
    if( looping == false ) // We are in poll mode or ending loop
    {
        tv.tv_sec  = 0;
        tv.tv_usec = 10000;
        retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
    }
    else if ( calc_timeout(&tv) )
    {        
        retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
    }
    else
    {
        retval = select(max_fd + 1, &rfds, NULL, NULL, NULL);
    }

    // Check the outcome of the callback now...
    if (retval < 0)
    {
        if (retval == -1)
        {
            perror("select()");
            // sleep(1);
        }
        
        // Call the read function so that it can get the bad fd return from read.
        for (x = 0; x < cb_max_used; x++)
        {
            if (internal_list[x].in_use && FD_ISSET(internal_list[x].fd, &rfds))
            {
                internal_list[x].fp(internal_list[x].fd, internal_list[x].user_data);
            }
        }

        call_timeout_callback();
        return retval;
    }
    else if (retval == 0)
    {
        // Call timeout callback...
        call_timeout_callback();
        return 0;
    }

    // Check what callback that should be invoked
    for (x = 0; x < cb_max_used; x++)
    {
        if (internal_list[x].in_use && FD_ISSET(internal_list[x].fd, &rfds))
        {
            internal_list[x].fp(internal_list[x].fd, internal_list[x].user_data);
        }
    }
    call_timeout_callback();
    return 1;
}

int cb_poll()
{
    looping = false;
    return _cb_poll();
}

// -1 => no timeout...
int cb_loop()
{    
    looping = true;

    // setup wakeup fd...

    // Looping...
    while (looping)
    {
        int ret = _cb_poll();
        if (ret < 0)
        {
            // No cb set...
            printf("No error-callback is set, (sleep 200us)...\n");
            usleep(200000);
        }
    }
    exit(EXIT_SUCCESS);
}
