#ifndef __CALLBACK_H
#define __CALLBACK_H

#include "main.h"
void add_monitor_data(monitor_data_t new_data);
void cpu_temperature_callback(struct uloop_timeout *timeout);
void memory_usage_callback(struct uloop_timeout *timeout);
void internet_status_callback(struct uloop_timeout *timeout);

#endif