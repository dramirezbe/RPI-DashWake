/*******************************************************************************
 * File: ntp_over_http.c
 * Author: Leandro Zapata
 *
 * Description:
 * This program fetches the current time from an NTP-backed web service using
 * an HTTP GET request via the cURL library. It then parses the response
 * and stores the date and time components into a custom structure.
 *
 * This is designed to run on a Linux system like a Raspberry Pi.
 *
 * How to compile:
 * gcc -o time_fetcher ntp_over_http.c -lcurl
 *
 * How to run:
 * ./time_fetcher
 *
 * Dependencies:
 * You must have the libcurl development library installed. On Debian/Ubuntu/Raspberry Pi OS:
 * sudo apt-get install libcurl4-openssl-dev
 *
 ******************************************************************************/

#include <stdio.h>    // For standard input/output (printf)
#include <stdlib.h>   // For memory allocation (malloc, free)
#include <string.h>   // For string manipulation (strstr, strncpy)
#include <curl/curl.h> // The main header for the cURL library

// The URL of the time service API. It provides time in UTC.
#define TIME_API_URL "http://worldtimeapi.org/api/timezone/Etc/UTC"

/**
 * @brief This is the custom structure to hold our parsed time data.
 */
typedef struct {
    /**
     * @brief Character array to store date components.
     * As per the requirement, a char array of size 3 is used.
     * Note: A 'char' can hold values up to 127 (or 255 if unsigned).
     * This is sufficient for day and month, but for the year, we will store
     * the last two digits (e.g., 24 for the year 2024).
     *
     * date[0]: Day of the month (e.g., 23)
     * date[1]: Month of the year (e.g., 7 for July)
     * date[2]: Year (last two digits, e.g., 24 for 2024)
     */
    char date[3];

    /**
     * @brief Integer array to store time components.
     *
     * time[0]: Seconds (0-59)
     * time[1]: Minutes (0-59)
     * time[2]: Hours (0-23)
     */
    int time[3];
} NTP_Time;


/**
 * @brief A helper structure to hold the data received from the cURL request.
 */
struct MemoryStruct {
    char *memory;
    size_t size;
};

/**
 * @brief This is the callback function that cURL will call when it receives data.
 *
 * This function is responsible for taking the received data chunk and appending it
 * to our MemoryStruct.
 *
 * @param contents Pointer to the received data.
 * @param size Size of each data element.
 * @param nmemb Number of data elements.
 * @param userp Pointer to our custom data structure (the MemoryStruct).
 * @return The number of bytes successfully handled.
 */
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb; // Calculate the total size of the received chunk
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    // Reallocate the memory buffer to fit the new data
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        // out of memory!
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize); // Append the new data
    mem->size += realsize; // Update the size
    mem->memory[mem->size] = 0; // Null-terminate the string

    return realsize;
}

/**
 * @brief Fetches time via HTTP and populates the NTP_Time structure.
 *
 * This function initializes cURL, performs an HTTP GET request to the TIME_API_URL,
 * receives the response, parses the datetime string, and fills the provided
 * NTP_Time struct with the data.
 *
 * @param time_struct A pointer to the NTP_Time structure that will be filled.
 * @return 0 on success, -1 on failure.
 */
int get_http_time(NTP_Time *time_struct) {
    CURL *curl_handle;
    CURLcode res;
    int return_code = -1; // Default to failure

    struct MemoryStruct chunk;
    chunk.memory = malloc(1); // Will be grown by the callback
    chunk.size = 0;           // No data at this point

    // Initialize the cURL library
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    if (curl_handle) {
        // Set the URL to fetch
        curl_easy_setopt(curl_handle, CURLOPT_URL, TIME_API_URL);
        // Set the callback function to handle the response data
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        // Pass our 'chunk' struct to the callback function
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
        // Set a user agent string, which is good practice
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        // Perform the request. 'res' will get the return code.
        res = curl_easy_perform(curl_handle);

        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // The request was successful. Now, parse the JSON response.
            // We will do a simple string search for the "datetime" field.
            // A full JSON parser would be more robust, but this is simpler.
            // Example response: ..."datetime":"2024-07-23T10:30:00.12345Z"...

            char *datetime_ptr = strstr(chunk.memory, "\"datetime\":\"");
            if (datetime_ptr) {
                // Move the pointer past the key ("\"datetime\":\"") to the actual value.
                datetime_ptr += strlen("\"datetime\":\"");

                int year, month, day, hour, minute, second;

                // Use sscanf to parse the formatted string.
                int items_scanned = sscanf(datetime_ptr, "%d-%d-%dT%d:%d:%d",
                                           &year, &month, &day, &hour, &minute, &second);

                if (items_scanned == 6) {
                    // Successfully parsed all 6 items. Now populate the struct.
                    time_struct->date[0] = (char)day;
                    time_struct->date[1] = (char)month;
                    time_struct->date[2] = (char)(year % 100); // Store last two digits of the year

                    time_struct->time[0] = second;
                    time_struct->time[1] = minute;
                    time_struct->time[2] = hour;
                    
                    printf("Successfully parsed time.\n");
                    return_code = 0; // Set return code to success
                } else {
                    fprintf(stderr, "Failed to parse the datetime string.\n");
                }
            } else {
                fprintf(stderr, "Could not find 'datetime' field in the response.\n");
            }
        }

        // Cleanup cURL handle
        curl_easy_cleanup(curl_handle);
    }

    // Free the memory used by our chunk
    free(chunk.memory);

    // Cleanup global cURL resources
    curl_global_cleanup();
    
    return return_code;
}