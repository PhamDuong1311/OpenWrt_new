#include "ubus.h"

static struct blob_buf b;


monitor_node_t* monitor_head = NULL;
struct ubus_context *ctx = NULL;
struct uloop_timeout timer = {};
                                
static const struct ubus_method monitor_methods[] = {
    { .name = "cpu_temperature", .handler = get_cpu_temperature},
    { .name = "memory_usage", .handler = get_memory_usage },
    { .name = "internet_info", .handler = get_internet_info },
};

static struct ubus_object_type monitor_type =
    UBUS_OBJECT_TYPE("monitor", monitor_methods);

struct ubus_object get_status = {
    .name = "server.monitor",
    .type = &monitor_type,
    .methods = monitor_methods,
    .n_methods = ARRAY_SIZE(monitor_methods),
};

int get_cpu_temperature(struct ubus_context *ctx, struct ubus_object *obj,
                               struct ubus_request_data *req, const char *method,
                               struct blob_attr *msg) {

    blob_buf_init(&b, 0);
    monitor_node_t *current = monitor_head;
    monitor_data_t *latest = NULL;

    while (current != NULL) {
        if (current->node.type == CPU_TEMPERATURE) {
            latest = &current->node;
            break;  
        }
        current = current->next;
    }

    if (latest) {

        blobmsg_add_u64(&b, "timestamp", (uint64_t)latest->timestamp);
        blobmsg_add_double(&b, "cpu_temperature", latest->value);
        blobmsg_add_string(&b, "unit", latest->unit);
    } else {
        blobmsg_add_string(&b, "error", "No CPU temperature data found");
    }
    ubus_send_reply(ctx, req, b.head);

    return 0;
}


int get_memory_usage(struct ubus_context *ctx, struct ubus_object *obj,
                             struct ubus_request_data *req, const char *method,
                             struct blob_attr *msg) {
    blob_buf_init(&b, 0);

    monitor_node_t *current = monitor_head;
    monitor_data_t *latest = NULL;

    while (current != NULL) {
        if (current->node.type == MEMORY_USAGE) {
            latest = &current->node;
            break;  
        }
        current = current->next;
    }

    if (latest) {
        blobmsg_add_u64(&b, "timestamp", (uint64_t)latest->timestamp);
        blobmsg_add_double(&b, "free_memory", latest->value);
        blobmsg_add_string(&b, "unit", latest->unit);
    } else {
        blobmsg_add_string(&b, "error", "No memory usage data found");
    }

    ubus_send_reply(ctx, req, b.head);
    
    return 0;
}

int get_internet_info(struct ubus_context *ctx, struct ubus_object *obj,
                             struct ubus_request_data *req, const char *method,
                             struct blob_attr *msg) {
    blob_buf_init(&b, 0);

    monitor_node_t *current = monitor_head;
    monitor_data_t *latest = NULL;

    while (current != NULL) {
        if (current->node.type == INTERNET_INFO) {
            latest = &current->node;
            break;  
        }
        current = current->next;
    }

    if (latest) {
        blobmsg_add_u64(&b, "timestamp", (uint64_t)latest->timestamp);
        blobmsg_add_u32(&b, "sent_packet", latest->threshold);
        blobmsg_add_u32(&b, "received_packet", latest->value);
        blobmsg_add_string(&b, "unit", latest->unit);
    } else {
        blobmsg_add_string(&b, "error", "No Internet Info data found");
    }

    ubus_send_reply(ctx, req, b.head);
    
    return 0;
}

int listen_cpu_alert(struct uloop_timeout *timeout) {
    blob_buf_init(&b, 0);

    monitor_node_t *current = monitor_head;
    monitor_data_t *latest = NULL;

    while (current != NULL) {
        if (current->node.type == CPU_TEMPERATURE) {
            latest = &current->node;
            break;  
        }
        current = current->next;
    }

    if (latest) {
        blobmsg_add_string(&b, "type", "cpu.temperature");
        void *array = blobmsg_open_table(&b, "data: ");
            blobmsg_add_double(&b, "value", latest->value);
            blobmsg_add_double(&b, "threshold", latest->threshold);
            blobmsg_add_u64(&b, "timestamp", (uint64_t)latest->timestamp);
            blobmsg_add_string(&b, "unit", latest->unit);
            blobmsg_add_string(&b, "severity", latest->severity);
        blobmsg_close_table(&b, array);

    } else {
        blobmsg_add_string(&b, "error", "No CPU temperature data found");
    }

    ubus_send_event(ctx, "server.temperature", b.head);    
    blob_buf_free(&b);
    return 0;
}

int listen_mem_alert(struct uloop_timeout *timeout) {
    blob_buf_init(&b, 0);

    monitor_node_t *current = monitor_head;
    monitor_data_t *data;

    while (current) {
        if (current->node.type == MEMORY_USAGE) {
            data = &current->node;
            break;
        }
        current = current->next;
    }

    if (data) {
        blobmsg_add_string(&b, "type", "memory.usage");
        void *array = blobmsg_open_table(&b, "data: ");
            blobmsg_add_double(&b, "value", data->value);
            blobmsg_add_double(&b, "threshold", data->threshold);
            blobmsg_add_u64(&b, "timestamp", (uint64_t)data->timestamp);
            blobmsg_add_string(&b, "unit", data->unit);
            blobmsg_add_string(&b, "severity", data->severity);
        blobmsg_close_table(&b, array);
            monitor_node_t *current = monitor_head;

    } else {
        blobmsg_add_string(&b, "error", "No memory usage data found");
    }

    ubus_send_event(ctx, "server.memory", b.head);    
    blob_buf_free(&b);
    return 0;
}

int listen_net_alert(struct uloop_timeout *timeout) {
    blob_buf_init(&b, 0);

    monitor_node_t *current = monitor_head;
    monitor_data_t *data;

    while (current) {
        if (current->node.type == INTERNET_INFO) {
            data = &current->node;
            break;
        }
        current = current->next;
    }

    if (data) {
        blobmsg_add_string(&b, "type", "internet.info");
        void *array = blobmsg_open_table(&b, "data: ");
            blobmsg_add_u32(&b, "value", data->value);
            blobmsg_add_u32(&b, "threshold", data->threshold);
            blobmsg_add_u64(&b, "timestamp", (uint64_t)data->timestamp);
            blobmsg_add_string(&b, "unit", data->unit);
            blobmsg_add_string(&b, "severity", data->severity);
        blobmsg_close_table(&b, array);
        
    } else {
        blobmsg_add_string(&b, "error", "No internet info data found");
    }

    ubus_send_event(ctx, "server.internet", b.head);    
    blob_buf_free(&b);
    return 0;
}