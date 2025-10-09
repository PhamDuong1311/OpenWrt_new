#ifndef __INVOKE_H
#define __INVOKE_H

#include "main.h"

extern struct ubus_object get_status;

int get_cpu_temperature(struct ubus_context *ctx, struct ubus_object *obj,
                             struct ubus_request_data *req, const char *method,
                             struct blob_attr *msg);
int get_memory_usage(struct ubus_context *ctx, struct ubus_object *obj,
                             struct ubus_request_data *req, const char *method,
                             struct blob_attr *msg);
int get_internet_info(struct ubus_context *ctx, struct ubus_object *obj,
                             struct ubus_request_data *req, const char *method,
                             struct blob_attr *msg);

int listen_cpu_alert(struct uloop_timeout *timeout);
int listen_mem_alert(struct uloop_timeout *timeout);
int listen_net_alert(struct uloop_timeout *timeout);

#endif