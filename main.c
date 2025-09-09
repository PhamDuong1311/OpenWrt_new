#include "main.h"
#include "event.h"
#include "invoke.h"
#include "callback.h"

monitor_node_t* monitor_head = NULL;
struct ubus_context *ctx = NULL;
struct uloop_timeout timer = {};
struct blob_buf b;

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
        printf("log2\n");

        blobmsg_add_u64(&b, "timestamp", (uint64_t)latest->timestamp);
        blobmsg_add_u32(&b, "cpu_temperature", latest->value);
        blobmsg_add_string(&b, "unit", latest->unit);
    } else {
        blobmsg_add_string(&b, "error", "No CPU temperature data found");
    }
    printf("log3\n");
    ubus_send_reply(ctx, req, b.head);

    return 0;
}


                                
static const struct ubus_method monitor_methods[] = {
    { .name = "cpu_temperature", .handler = get_cpu_temperature} 
    // { .name = "memory_usage", .handler = get_memory_usage },
    // { .name = "internet_info", .handler = get_internet_info },
};

static struct ubus_object_type monitor_type =
    UBUS_OBJECT_TYPE("monitor", monitor_methods);

static struct ubus_object get_status = {
    .name = "server.monitor",
    .type = &monitor_type,
    .methods = monitor_methods,
    .n_methods = ARRAY_SIZE(monitor_methods),
};





// static void general_callback(struct uloop_timeout *t) {
//     cpu_temperature_callback(t);
//     memory_usage_callback(t);
//     internet_status_callback(t);
//     listen_cpu_alert(t);
//     listen_mem_alert(t);
//     listen_net_alert(t);
//     uloop_timeout_set(t, 2000); 
// }

// void start_timer() {
//     timer.cb = general_callback;
//     uloop_timeout_set(&timer, 2000);
// }

int main() {    
    uloop_init();

    ctx = ubus_connect(NULL);
    if (!ctx) {
        fprintf(stderr, "Failed to connect to ubusd\n");
        uloop_done();
        return -1;
    }

    ubus_add_uloop(ctx);
    int ret = ubus_add_object(ctx, &get_status);
    if (ret) {
        fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));
        ubus_free(ctx);
        uloop_done();
        return ret;
    }
    //start_timer();
    printf("run uloop\n");
    uloop_run();
    printf("uloop done");
    // uloop_timeout_cancel(&timer);
    ubus_free(ctx);
    uloop_done();
    return 0;
}
