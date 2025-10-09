#include "main.h"
#include "ubus.h"
#include "get_data.h"

static void general_callback(struct uloop_timeout *t) {
    cpu_temperature_callback(t);
    memory_usage_callback(t);
    internet_status_callback(t);
    listen_cpu_alert(t);
    listen_mem_alert(t);
    listen_net_alert(t);
    uloop_timeout_set(t, 2000); 
}

void start_timer() {
    timer.cb = general_callback;
    uloop_timeout_set(&timer, 2000);
}

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
    start_timer();
    uloop_run();
    uloop_timeout_cancel(&timer);
    ubus_free(ctx);
    uloop_done();
    return 0;
}
