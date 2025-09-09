#include "main.h"
#include "invoke.h"



int get_memory_usage(struct ubus_context *ctx, struct ubus_object *obj,
                             struct ubus_request_data *req, const char *method,
                             struct blob_attr *msg) {
    struct blob_buf b;
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
        blobmsg_add_u32(&b, "free_memory", latest->value);
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
    struct blob_buf b;
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
    struct blob_buf b;
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
            blobmsg_add_u32(&b, "value", latest->value);
            blobmsg_add_u32(&b, "threshold", latest->threshold);
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
     struct blob_buf b;
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
            blobmsg_add_u32(&b, "threshold", data->threshold);
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
     struct blob_buf b;
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