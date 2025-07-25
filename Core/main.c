#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <wiringSerial.h>
#include <wiringPi.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <libgen.h>   // For dirname
#include <sys/stat.h> // For mkdir

#include "Modules/btn_handler.h"
#include "Modules/force_ntp_sync.h"
#include "Modules/cJSON.h"

#define SERIAL_PORT "/dev/serial0"
#define BAUD_RATE 9600
#define BTN_PIN 0
#define PATH_MAX 4096

char JSON_BASE_DIR[PATH_MAX];

typedef enum {
    IDLE_TYPE,
    ALARM_STOP_TYPE,
    NTP_TYPE,
    SENSOR_TYPE,
} json_post_type_t;

extern volatile bool button_press;
json_post_type_t json_post_type = IDLE_TYPE;

//UTILS

static void write_json_to_file(const char *json_string, const char *filename_base) {
    if (json_string == NULL || filename_base == NULL || JSON_BASE_DIR[0] == '\0') {
        fprintf(stderr, "Error: Invalid arguments or JSON_BASE_DIR not set for write_json_to_file.\n");
        return;
    }

    char filepath[PATH_MAX];
    snprintf(filepath, sizeof(filepath) - 1, "%s/%s.json", JSON_BASE_DIR, filename_base);
    filepath[PATH_MAX - 1] = '\0';

    if (access(JSON_BASE_DIR, F_OK) == -1) {
        printf("Directory %s does not exist, attempting to create...\n", JSON_BASE_DIR);
        if (mkdir(JSON_BASE_DIR, 0755) == -1) {
            fprintf(stderr, "Error creating directory %s: %s\n", JSON_BASE_DIR, strerror(errno));
            return;
        }
    }

    FILE *fp = fopen(filepath, "w");
    if (fp == NULL) {
        fprintf(stderr, "Error opening file %s for writing: %s\n", filepath, strerror(errno));
        return;
    }

    fprintf(fp, "%s", json_string);
    fclose(fp);
    printf("Successfully wrote JSON to %s\n", filepath);
}

void *ntp_timer_thread(void *arg) {
    while (1) {
        printf("[NTP Timer Thread] Waiting 5 minutes for the next NTP JSON...\n");
        sleep(300);
        json_post_type = NTP_TYPE;
        printf("[NTP Timer Thread] 1 minute has passed! Marking NTP_TYPE for JSON dispatch.\n");
    }
    return NULL;
}


int main(void) {
    
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        perror("readlink");
        return 1;
    }
    exe_path[len] = '\0';

    char *dup_exe_path = strdup(exe_path);
    if (dup_exe_path == NULL) {
        perror("strdup");
        return 1;
    }
    char *dup_dir1 = strdup(dirname(dup_exe_path));
    if (dup_dir1 == NULL) {
        perror("strdup");
        free(dup_exe_path);
        return 1;
    }
    char *dup_dir2 = strdup(dirname(dup_dir1));
    if (dup_dir2 == NULL) {
        perror("strdup");
        free(dup_dir1);
        free(dup_exe_path);
        return 1;
    }
    char *dup_dir3 = strdup(dirname(dup_dir2));
    if (dup_dir3 == NULL) {
        perror("strdup");
        free(dup_dir2);
        free(dup_dir1);
        free(dup_exe_path);
        return 1;
    }

    int written = snprintf(JSON_BASE_DIR, sizeof(JSON_BASE_DIR), "%s/tmp", dup_dir3);
    if (written >= sizeof(JSON_BASE_DIR) || written < 0) {
        fprintf(stderr, "Error: Path buffer too small or snprintf error when creating JSON_BASE_DIR.\n");
        free(dup_dir3);
        free(dup_dir2);
        free(dup_dir1);
        free(dup_exe_path);
        return 1;
    }
    JSON_BASE_DIR[PATH_MAX - 1] = '\0';

    printf("Calculated JSON output base directory: %s\n", JSON_BASE_DIR);

    free(dup_dir3);
    free(dup_dir2);
    free(dup_dir1);
    free(dup_exe_path);

    int fd;
    char serialData[64];
    int dataIndex = 0;

    time_t rawtime;
    struct tm *info;
    char date_buffer[11];
    char time_buffer[9];

    printf("Waiting for data from ESP32 in '|humidity|temperature|ADC|' format...\n");

    if (wiringPiSetup() == -1) {
        fprintf(stderr, "Failed to initialize WiringPi. Exiting.\n");
        return 1;
    }

    fd = serialOpen(SERIAL_PORT, BAUD_RATE);
    printf("Init UART catch from ESP32\n");
    if (fd == -1) {
        fprintf(stderr, "Error opening serial port %s: %s\n", SERIAL_PORT, strerror(errno));
        return 1;
    }
    printf("Serial port %s open using %d baudrate.\n", SERIAL_PORT, BAUD_RATE);

    button_init(BTN_PIN);
    printf("Init button\n");

    pthread_t ntp_thread_id;

    if (force_system_ntp_sync() == 0) {
        printf("--- First NTP Sync Request Successful ---\n");
        json_post_type = NTP_TYPE;

        if (pthread_create(&ntp_thread_id, NULL, ntp_timer_thread, NULL) != 0) {
            fprintf(stderr, "Error creating NTP timer thread.\n");
            return 1;
        }
        printf("NTP timer thread started.\n");

    } else {
        fprintf(stderr, "Failed to force NTP synchronization. Exiting.\n");
        return 1;
    }

    char latestSerialData[64] = {0};

    while (1) {
        if (button_press) {
            printf("Button Pressed.............\n");
            button_press = false;
            json_post_type = ALARM_STOP_TYPE;
        }

        if (serialDataAvail(fd)) {
            char byte = serialGetchar(fd);

            if (byte == '\n' || byte == '\r') {
                serialData[dataIndex] = '\0';

                if (dataIndex > 0) {
                    printf("UART Rx: '%s'\n", serialData);
                    strncpy(latestSerialData, serialData, sizeof(latestSerialData) - 1);
                    latestSerialData[sizeof(latestSerialData) - 1] = '\0';
                    json_post_type = SENSOR_TYPE;
                }

                dataIndex = 0;
                memset(serialData, 0, sizeof(serialData));
            } else {
                if (dataIndex < sizeof(serialData) - 1) {
                    serialData[dataIndex++] = byte;
                } else {
                    fprintf(stderr, "Warning: Read buffer full. Possible data loss. Resetting buffer.\n");
                    dataIndex = 0;
                    memset(serialData, 0, sizeof(serialData));
                }
            }
        }

        switch (json_post_type) {
        case ALARM_STOP_TYPE: {
            printf("[JSON Sender] Ready to send ALARM STOP JSON.\n");
            cJSON *root_alarm = cJSON_CreateObject();
            if (root_alarm != NULL) {
                cJSON_AddBoolToObject(root_alarm, "alarm_stopped", true);
                char *json_string_alarm = cJSON_Print(root_alarm);
                if (json_string_alarm != NULL) {
                    printf("[ALARM_STOP JSON] %s\n", json_string_alarm);
                    write_json_to_file(json_string_alarm, "alarm");
                    free(json_string_alarm);
                }
                cJSON_Delete(root_alarm);
            }
            break;
        }
        case NTP_TYPE: {
            printf("[JSON Sender] Ready to send NTP JSON.\n");

            time(&rawtime);
            info = localtime(&rawtime);
            strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", info);
            strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", info);

            cJSON *root_ntp = cJSON_CreateObject();
            if (root_ntp != NULL) {
                cJSON_AddStringToObject(root_ntp, "date", date_buffer);
                cJSON_AddStringToObject(root_ntp, "hour", time_buffer);
                char *json_string_ntp = cJSON_Print(root_ntp);
                if (json_string_ntp != NULL) {
                    printf("[NTP JSON] %s\n", json_string_ntp);
                    write_json_to_file(json_string_ntp, "ntp");
                    free(json_string_ntp);
                }
                cJSON_Delete(root_ntp);
            }
            break;
        }
        case SENSOR_TYPE: {
            printf("[JSON Sender] Ready to send SENSOR JSON.\n");

            float hum, tempC;
            int mq7Adc;

            int parsed_items = sscanf(latestSerialData, "|%f|%f|%d|", &hum, &tempC, &mq7Adc);

            if (parsed_items == 3) {
                printf("Parsed data: Humidity=%.2f, Temperature=%.2f, MQ7_ADC=%d\n", hum, tempC, mq7Adc);

                cJSON *root_sensor = cJSON_CreateObject();
                if (root_sensor != NULL) {
                    cJSON_AddNumberToObject(root_sensor, "hum", hum);
                    cJSON_AddNumberToObject(root_sensor, "tempC", tempC);
                    cJSON_AddNumberToObject(root_sensor, "mq7Adc", mq7Adc);

                    char *json_string_sensor = cJSON_Print(root_sensor);
                    if (json_string_sensor != NULL) {
                        printf("[SENSOR JSON] %s\n", json_string_sensor);
                        write_json_to_file(json_string_sensor, "sensor");
                        free(json_string_sensor);
                    }
                    cJSON_Delete(root_sensor);
                }
            } else {
                fprintf(stderr, "Error: Could not parse serial data: '%s'. Expected items: 3, Read items: %d\n", latestSerialData, parsed_items);
            }
            break;
        }
        case IDLE_TYPE:
            break;
        default:
            break;
        }

        json_post_type = IDLE_TYPE;
        delay(1);
    }

    serialClose(fd);
    return 0;
}
