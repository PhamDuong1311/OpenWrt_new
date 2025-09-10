#include "get_data.h"

void add_monitor_data(monitor_data_t new_data) {
    monitor_node_t *current = monitor_head;

    while (current != NULL) {
        if (current->node.type == new_data.type) {
            current->node = new_data; 
            return;
        }
        current = current->next;
    }

    monitor_node_t *new_node = malloc(sizeof(monitor_node_t));
    if (!new_node) {
        fprintf(stderr, "malloc failed\n");
        return;
    }
    new_node->node = new_data;
    new_node->next = monitor_head;
    monitor_head = new_node;
}


void cpu_temperature_callback(struct uloop_timeout *timeout) {
    char buffer[1024];
    FILE *f = popen("sensors | grep Core", "r");

    if (f != NULL) {        
        while (fgets(buffer, sizeof(buffer), f) != NULL) {
            if (strlen(buffer) > 10) {
                monitor_data_t data;
                float value;
                int parsed = sscanf(buffer, "Core %*d: +%f", &value);
                if (parsed == 1) {
                    data.value = value;
                    data.type = CPU_TEMPERATURE;
                    data.threshold = 80;
                    data.timestamp = time(NULL);
                    memcpy(data.unit, "C", 2);
                    

                    if (data.value > data.threshold) {
                        memcpy(data.severity, "WARNING", 8);
                    } else {
                        memcpy(data.severity, "NORMAL", 7);
                    }

                    add_monitor_data(data);
                }
            }
        }
        pclose(f);
    }
    uloop_timeout_set(timeout, 5000);
}

void memory_usage_callback(struct uloop_timeout *timeout) {
    char buffer[1024];
    FILE *f = popen("cat /proc/meminfo | grep -E 'MemTotal|MemFree'", "r");
    if (f != NULL) {
        while (fgets(buffer, sizeof(buffer), f) != NULL) {
            if (strlen(buffer) > 10) {

                monitor_data_t data;
                float memtotal;
                float memfree;
                if (strstr(buffer, "MemTotal:")) {
                    sscanf(buffer, "MemTotal: %f kB", &memtotal);
                } else if (strstr(buffer, "MemFree:")) {
                    sscanf(buffer, "MemFree: %f kB", &memfree);
                }

                data.value = (float) memfree / memtotal;
                data.type = MEMORY_USAGE;
                data.threshold = 0.2;
                data.timestamp = time(NULL);
                memcpy(data.unit, "%", 1);
                

                if (data.value < data.threshold) {
                    memcpy(data.severity, "WARNING", 8);
                } else {
                    memcpy(data.severity, "NORMAL", 7);
                }

                add_monitor_data(data);
            }
        }
        pclose(f);
    }
    uloop_timeout_set(timeout, 5000);
}

void internet_status_callback(struct uloop_timeout *timeout) {
    char buffer[1024];
    FILE *f = popen("ping -c 1 -W 1 8.8.8.8", "r"); 

    monitor_data_t data;
    data.type = INTERNET_INFO;
    data.timestamp = time(NULL);
    data.threshold = 1;  
    strcpy(data.unit, "");


    if (f != NULL) {
        while (fgets(buffer, sizeof(buffer), f) != NULL) {
            if (strstr(buffer, "1 received") || strstr(buffer, "1 packets received")) {
                data.value = 1;
                break;
            }
        }
        pclose(f);
    }

    if (data.value < data.threshold) {
        memcpy(data.severity, "INTERNET DOWN", 14);
    } else {
        memcpy(data.severity, "INTERNET UP", 12);
    }

    add_monitor_data(data);

    uloop_timeout_set(timeout, 5000);
}
