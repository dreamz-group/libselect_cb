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

#ifndef __SELECTED_CB_H__
#define __SELECTED_CB_H__
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*cb_func_p)(int fd, void *user_data);
typedef int (*cb_to_func_p)(time_t epoc_wake_uptime, void *user_data);

/// Poll the for data.
int cb_poll(); 

/// This will start a loop waiting for data to arive and trigger the callbacks.
int cb_loop(); 

/// Add a callback assosiated to the fd to be called when the fd is activated
/// or when an error accure.
///
/// @param fd        - file descriptor to monitor.
/// @param fp        - function pointert to be called when the fd is triggerd.
/// @param user_data - is passed to the fd.
/// @return -1 on error.
int cb_add(int fd, cb_func_p fp, void *user_data);

/// Remove a file descriptior from the list of fds to monitor.
int cb_del(int fd);

/// Init must be called one time during startup and only one time!
int cb_init();

/// Add a reacurring timer 
/// @param sec       - intervall to be used to wake up the callback.
/// @param fp        - callback function to call when timer is release.
/// @param user_data - user data to be passed to the callback function.
//  @return -1 if the list of callbacks is full.
int cb_to_repeat_add(time_t sec, cb_to_func_p fp, void *user_data);

/// Add a timer, this will only trigger once.
/// @param wake_uptime_from_now_in_sec - call the callback in n seconds from now.
/// @param fp                          - callback to call.
/// @param user_data                   - passed to the callback on trigger.
/// @return -1 if the list of callbacks if full.
int cb_to_add(time_t wake_uptime_from_now_in_sec, cb_to_func_p fp, void *user_data);

/// Change the timer to run in n sec and then if its a repeat timer continue on
/// its normal cyckel or go away if its a one time.
void cb_to_fire( cb_to_func_p fp, time_t call );

/// Removes a timer if found.
/// @param callback function to look for.
int cb_to_del(cb_to_func_p fp);

/// Add a timer using epoch time stamp.
/// @param wake_uptime_in_epoch - call the callback at this exact point in time.
/// @param fp                   - callback function to be called.
/// @param user_data            - passed to the callback on trigger.
/// @return -1 if the list of callbacks if full.
int cb_to_epoch_add(time_t wake_uptime_in_epoch, cb_to_func_p fp, void *user_data);

/// Stop the callback loop, this can be used in a callback to trigger exit from the cb_loop.
int cb_stop();

#ifdef __cplusplus
} //extern "C" 
#endif
#endif
