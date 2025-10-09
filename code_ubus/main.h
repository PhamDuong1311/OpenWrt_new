#ifndef __MAIN_H
#define __MAIN_H

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <libubus.h>
#include <libubox/uloop.h>
#include <libubox/blobmsg_json.h>

extern struct ubus_context *ctx;
extern struct uloop_timeout timer;

typedef enum {
    CPU_TEMPERATURE,
    MEMORY_USAGE,
    INTERNET_INFO,
} alarm_type_t;

typedef struct {
    alarm_type_t type;
    float value;
    float threshold;
    time_t timestamp;
    char unit[5];
    char severity[20];
} monitor_data_t;

typedef struct monitor_node {
    struct monitor_node *next;
    monitor_data_t node;
} monitor_node_t;

extern monitor_node_t* monitor_head;


#endif 