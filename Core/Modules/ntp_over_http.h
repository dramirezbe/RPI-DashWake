#ifndef NTP_OVER_HTTP_H
#define NTP_OVER_HTTP_H

// Header guard to prevent multiple inclusions

/**
 * @brief This is the custom structure to hold our parsed time data.
 */
typedef struct {
    char date[3]; // [0]: Day, [1]: Month, [2]: Year (last two digits)
    int  time[3]; // [0]: Seconds, [1]: Minutes, [2]: Hours
} NTP_Time;

/**
 * @brief Parses a JSON string from the WorldTimeAPI to populate an NTP_Time struct.
 *
 * This is the core logic that our test will target. It searches for the "datetime"
 * field and extracts the time and date information.
 *
 * @param json_response A constant character string containing the JSON data.
 * @param time_struct A pointer to the NTP_Time structure to be filled.
 * @return 0 on success, -1 on failure (e.g., field not found, parsing error).
 */
int parse_time_string(const char* json_response, NTP_Time* time_struct);

/**
 * @brief Fetches time via HTTP and populates the NTP_Time structure.
 *
 * This function handles the network request and then calls parse_time_string
 * to process the response.
 *
 * @param time_struct A pointer to the NTP_Time structure that will be filled.
 * @return 0 on success, -1 on failure.
 */
int get_http_time(NTP_Time *time_struct);

#endif // NTP_OVER_HTTP_H