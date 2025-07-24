/**
 * @file main.c
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <wiringSerial.h>
#include <wiringPi.h>
#include <pthread.h> // Added for thread support!
#include <unistd.h>  // Added for sleep!
#include <cJSON.h>
#include <time.h>
#include <limits.h>
#include <libgen.h>   // For dirname
#include <sys/stat.h> // For mkdir
#include <unistd.h>   // For access (F_OK)

#include "Modules/btn_handler.h"
#include "Modules/force_ntp_sync.h"

#define SERIAL_PORT "/dev/serial0"
#define BAUD_RATE 9600
#define BTN_PIN 0

// Global variable to store the calculated base directory for JSON output
char JSON_BASE_DIR[PATH_MAX];

typedef enum {
    IDLE_TYPE,       // if not event, do nothing
    ALARM_STOP_TYPE, // bool json
    NTP_TYPE,        // strings json, every 10 min
    SENSOR_TYPE,     // hum, temp, mq7adc  json
} json_post_type_t;

// Global variable to control the JSON POST type
json_post_type_t json_post_type = IDLE_TYPE;

extern volatile bool button_press; // Make sure this line is present if button_press is external

// --- Helper function to write JSON to file ---
// Simplified: always uses a fixed filename and overwrites
static void write_json_to_file(const char *json_string, const char *filename_base) {
    if (json_string == NULL || filename_base == NULL || JSON_BASE_DIR[0] == '\0') {
        fprintf(stderr, "Error: Invalid arguments or JSON_BASE_DIR not set for write_json_to_file.\n");
        return;
    }

    char filepath[PATH_MAX];
    // Use a fixed filename directly (e.g., "ntp.json")
    snprintf(filepath, sizeof(filepath) - 1, "%s/%s.json", JSON_BASE_DIR, filename_base);
    filepath[PATH_MAX - 1] = '\0'; // Ensure null termination

    if (access(JSON_BASE_DIR, F_OK) == -1) {
        printf("Directory %s does not exist, attempting to create...\n", JSON_BASE_DIR);
        if (mkdir(JSON_BASE_DIR, 0755) == -1) {
            fprintf(stderr, "Error creating directory %s: %s\n", JSON_BASE_DIR, strerror(errno));
            return;
        }
    }

    FILE *fp = fopen(filepath, "w"); // "w" mode will overwrite if file exists
    if (fp == NULL) {
        fprintf(stderr, "Error opening file %s for writing: %s\n", filepath, strerror(errno));
        return;
    }

    fprintf(fp, "%s", json_string);
    fclose(fp);
    printf("Successfully wrote JSON to %s\n", filepath);
}

// --- Thread for NTP timer ---
void *ntp_timer_thread(void *arg) {
    while (1) {
        printf("[NTP Timer Thread] Waiting 1 minute for the next NTP JSON...\n");
        sleep(60); // Wait 60 seconds (1 minute for testing, revert to 600 for 10 min)
        // When the timer expires, set the POST type
        json_post_type = NTP_TYPE;
        printf("[NTP Timer Thread] 1 minute has passed! Marking NTP_TYPE for JSON dispatch.\n");
    }
    return NULL;
}
// --- End of Thread for NTP timer ---


int main(void) {
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        perror("readlink");
        return 1;
    }
    exe_path[len] = '\0'; // Null-terminate the path

    // Duplicate strings for dirname, as dirname might modify its input.
    // It's crucial to free these duplicated strings later to prevent memory leaks.
    char *dup_exe_path = strdup(exe_path);
    char *dup_dir1 = strdup(dirname(dup_exe_path)); // /path/to/your/bin
    char *dup_dir2 = strdup(dirname(dup_dir1));     // /path/to/your
    char *dup_dir3 = strdup(dirname(dup_dir2));     // /path/to

    // The target directory is 3 levels up (dir_level4)
    // Make sure JSON_BASE_DIR is large enough to hold the path.
    // We are setting JSON_BASE_DIR to the directory pointed to by dup_dir3, which is `dirname` of `dup_dir2`.
    // Be careful with the logic here if you need to go higher, as `dirname("/")` is "/".
    strncpy(JSON_BASE_DIR, dup_dir3, PATH_MAX - 1);
    JSON_BASE_DIR[PATH_MAX - 1] = '\0'; // Ensure null termination

    printf("Calculated JSON output base directory: %s\n", JSON_BASE_DIR);

    // Free the duplicated strings to prevent memory leaks
    free(dup_dir3); // dirname(dirname(dirname(exe_path)))
    free(dup_dir2); // dirname(dirname(exe_path))
    free(dup_dir1); // dirname(exe_path)
    free(dup_exe_path); // original duplicated exe_path


    int fd;                 // Serial File descriptor
    char serialData[64];    // Buffer storage for serial data
    int dataIndex = 0;      // Buffer index

    time_t rawtime;
    struct tm *info;
    char date_buffer[11]; // YYYY-MM-DD\0
    char time_buffer[9];  // HH:MM:SS\0

    printf("Waiting for data from ESP32 in '|humidity|temperature|ADC|' format...\n");

    if (wiringPiSetup() == -1) {
        fprintf(stderr, "Failed to initialize WiringPi. Exiting.\n");
        return 1; // Use return 1 for error exit
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

    pthread_t ntp_thread_id; // NTP thread identifier

    // First NTP request at program start
    if (force_system_ntp_sync() == 0) { // Check if the command was successfully launched
        printf("--- First NTP Sync Request Successful ---\n");

        // Mark for an initial NTP JSON POST
        json_post_type = NTP_TYPE;

        // Create the NTP timer thread for the next 10 minutes
        if (pthread_create(&ntp_thread_id, NULL, ntp_timer_thread, NULL) != 0) {
            fprintf(stderr, "Error creating NTP timer thread.\n");
            return 1;
        }
        printf("NTP timer thread started.\n");

    } else {
        fprintf(stderr, "Failed to force NTP synchronization. Exiting.\n");
        return 1;
    }

    // Global variable to store the latest valid serial data
    // Will be accessed by the main function to build the sensor JSON
    char latestSerialData[64] = {0}; // Initialize to zeros

    while (1) {
        // --- Button handling ---
        if (button_press) {
            printf("Button Pressed.............\n");
            button_press = false;               // Reset the button flag
            json_post_type = ALARM_STOP_TYPE; // Mark for alarm stop JSON
        }

        // --- Serial data handling ---
        if (serialDataAvail(fd)) {
            char byte = serialGetchar(fd); // Read a single byte from the serial port

            // Byte accumulation logic and end-of-string detection
            if (byte == '\n' || byte == '\r') {
                serialData[dataIndex] = '\0'; // Add the null terminator

                if (dataIndex > 0) {
                    printf("UART Rx: '%s'\n", serialData);
                    // Copy valid data to the global buffer for SENSOR_TYPE to use
                    strncpy(latestSerialData, serialData, sizeof(latestSerialData) - 1);
                    latestSerialData[sizeof(latestSerialData) - 1] = '\0'; // Ensure null termination
                    json_post_type = SENSOR_TYPE; // Mark for sensor JSON only if valid data
                }

                // Reset buffer index and clear buffer for the next string
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

        // --- JSON dispatch logic ---
        switch (json_post_type) {
        case ALARM_STOP_TYPE: {
            printf("[JSON Sender] Ready to send ALARM STOP JSON.\n");
            cJSON *root_alarm = cJSON_CreateObject();
            if (root_alarm == NULL) {
                fprintf(stderr, "Error creating ALARM_STOP JSON object.\n");
            } else {
                cJSON_AddBoolToObject(root_alarm, "alarm_stopped", true);
                char *json_string_alarm = cJSON_Print(root_alarm);
                if (json_string_alarm == NULL) {
                    fprintf(stderr, "Error converting ALARM_STOP JSON to string.\n");
                } else {
                    printf("[ALARM_STOP JSON] %s\n", json_string_alarm);
                    write_json_to_file(json_string_alarm, "alarm"); // Fixed filename "alarm.json"
                    free(json_string_alarm);
                }
                cJSON_Delete(root_alarm);
            }
            break;
        }
        case NTP_TYPE: {
            printf("[JSON Sender] Ready to send NTP JSON.\n");

            // Get current system time
            time(&rawtime);
            info = localtime(&rawtime);

            // Format date as YYYY-MM-DD
            strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", info);

            // Format time as HH:MM:SS
            strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", info);

            // Create the JSON object
            cJSON *root_ntp = cJSON_CreateObject();
            if (root_ntp == NULL) {
                fprintf(stderr, "Error creating NTP JSON object.\n");
            } else {
                cJSON_AddStringToObject(root_ntp, "date", date_buffer);
                cJSON_AddStringToObject(root_ntp, "hour", time_buffer);

                // Convert JSON to string
                char *json_string_ntp = cJSON_Print(root_ntp);
                if (json_string_ntp == NULL) {
                    fprintf(stderr, "Error converting NTP JSON to string.\n");
                } else {
                    printf("[NTP JSON] %s\n", json_string_ntp);
                    write_json_to_file(json_string_ntp, "ntp"); // Fixed filename "ntp.json"
                    free(json_string_ntp);                           // Free memory allocated by cJSON_Print
                }
                cJSON_Delete(root_ntp); // Free cJSON object memory
            }
            break;
        }
        case SENSOR_TYPE: {
            printf("[JSON Sender] Ready to send SENSOR JSON.\n");

            float hum, tempC;
            int mq7Adc;

            // Use sscanf to parse the received string.
            // It's crucial that the format here EXACTLY matches what the ESP32 sends.
            // Example: "|50.50|25.00|800|"
            int parsed_items = sscanf(latestSerialData, "|%f|%f|%d|", &hum, &tempC, &mq7Adc);

            if (parsed_items == 3) {
                printf("Parsed data: Humidity=%.2f, Temperature=%.2f, MQ7_ADC=%d\n", hum, tempC, mq7Adc);

                // Create the JSON object for sensor data
                cJSON *root_sensor = cJSON_CreateObject();
                if (root_sensor == NULL) {
                    fprintf(stderr, "Error creating SENSOR JSON object.\n");
                } else {
                    cJSON_AddNumberToObject(root_sensor, "hum", hum);
                    cJSON_AddNumberToObject(root_sensor, "tempC", tempC);
                    cJSON_AddNumberToObject(root_sensor, "mq7Adc", mq7Adc);

                    char *json_string_sensor = cJSON_Print(root_sensor);
                    if (json_string_sensor == NULL) {
                        fprintf(stderr, "Error converting SENSOR JSON to string.\n");
                    } else {
                        // Print the sensor JSON
                        printf("[SENSOR JSON] %s\n", json_string_sensor);
                        write_json_to_file(json_string_sensor, "sensor"); // Fixed filename "sensor.json"
                        free(json_string_sensor);                               // Free memory
                    }
                    cJSON_Delete(root_sensor); // Free cJSON object memory
                }
            } else {
                fprintf(stderr, "Error: Could not parse serial data: '%s'. Expected items: 3, Read items: %d\n", latestSerialData, parsed_items);
            }
            break;
        }
        case IDLE_TYPE:
            // Do nothing, waiting for events
            break;
        default:
            // Handle unexpected state, if necessary
            break;
        }

        // Important! Reset the POST type after handling it
        // This prevents the same event from being processed repeatedly in the main loop.
        json_post_type = IDLE_TYPE;

        delay(1); // Small pause to avoid saturating the CPU
    }

    // These lines would only execute if the while(1) loop was interrupted
    serialClose(fd);
    // pthread_cancel(ntp_thread_id); // If you need to stop the thread gracefully on exit
    // pthread_join(ntp_thread_id, NULL); // Wait for the thread to terminate (if it was canceled)
    return 0;
}