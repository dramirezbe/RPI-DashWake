/**
 * @file test.c
 * @brief ESP32 UART Simulator - Sends dummy sensor data with the same format as ESP32
 * 
 * This program simulates ESP32 behavior by sending dummy sensor data
 * through UART in the format: |humidity|temperature|ADC|
 * 
 * @author Leandro
 * @date 2024
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>

// Configuration constants
#define SERIAL_PORT "/dev/serial0"  // Serial port device
#define BAUD_RATE B9600            // Baud rate matching main.c
#define SEND_INTERVAL 5            // Send data every 5 seconds

// Sensor data ranges for realistic simulation
#define HUMIDITY_MIN 30.0f         // Minimum humidity percentage
#define HUMIDITY_MAX 80.0f         // Maximum humidity percentage
#define TEMP_MIN 15.0f             // Minimum temperature in Celsius
#define TEMP_MAX 35.0f             // Maximum temperature in Celsius
#define MQ7_ADC_MIN 200            // Minimum MQ7 ADC value
#define MQ7_ADC_MAX 1000           // Maximum MQ7 ADC value

// Global variable to control program execution
volatile bool running = true;

/**
 * @brief Signal handler for graceful program termination
 * @param sig Signal number received
 */
void signal_handler(int sig) {
    printf("\nReceived signal %d. Shutting down gracefully...\n", sig);
    running = false;
}

/**
 * @brief Configure and open serial port with proper settings
 * @param port Serial port device path
 * @return File descriptor on success, -1 on failure
 */
int setup_serial(const char* port) {
    // Open serial port for writing only, non-blocking
    int fd = open(port, O_WRONLY | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("Error opening serial port");
        return -1;
    }
    
    struct termios options;
    
    // Get current port settings
    if (tcgetattr(fd, &options) != 0) {
        perror("Error getting serial port attributes");
        close(fd);
        return -1;
    }
    
    // Set baud rate for both input and output
    cfsetispeed(&options, BAUD_RATE);
    cfsetospeed(&options, BAUD_RATE);
    
    // Configure 8N1 (8 data bits, no parity, 1 stop bit)
    options.c_cflag &= ~PARENB;    // No parity bit
    options.c_cflag &= ~CSTOPB;    // One stop bit
    options.c_cflag &= ~CSIZE;     // Clear data size bits
    options.c_cflag |= CS8;        // 8 data bits
    
    // Disable hardware flow control
    options.c_cflag &= ~CRTSCTS;
    
    // Enable receiver and set local mode
    options.c_cflag |= CREAD | CLOCAL;
    
    // Set raw input mode (no canonical processing)
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // Set raw output mode (no processing)
    options.c_oflag &= ~OPOST;
    
    // Disable software flow control
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    
    // Apply configuration immediately
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        perror("Error setting serial port attributes");
        close(fd);
        return -1;
    }
    
    printf("Serial port %s configured successfully at 9600 baud\n", port);
    return fd;
}

/**
 * @brief Generate realistic dummy sensor data
 * @param humidity Pointer to store generated humidity value (%)
 * @param temperature Pointer to store generated temperature value (°C)
 * @param mq7_adc Pointer to store generated MQ7 ADC value
 */
void generate_sensor_data(float *humidity, float *temperature, int *mq7_adc) {
    // Generate humidity between HUMIDITY_MIN and HUMIDITY_MAX with 2 decimal precision
    *humidity = HUMIDITY_MIN + ((float)(rand() % ((int)((HUMIDITY_MAX - HUMIDITY_MIN) * 100)))) / 100.0f;
    
    // Generate temperature between TEMP_MIN and TEMP_MAX with 2 decimal precision
    *temperature = TEMP_MIN + ((float)(rand() % ((int)((TEMP_MAX - TEMP_MIN) * 100)))) / 100.0f;
    
    // Generate MQ7 ADC value between MQ7_ADC_MIN and MQ7_ADC_MAX
    *mq7_adc = MQ7_ADC_MIN + (rand() % (MQ7_ADC_MAX - MQ7_ADC_MIN + 1));
}

/**
 * @brief Print program usage and troubleshooting information
 */
void print_usage_info(void) {
    printf("=== ESP32 UART Simulator ===\n");
    printf("Data format: |humidity|temperature|ADC|\n");
    printf("Send interval: %d seconds\n", SEND_INTERVAL);
    printf("Serial port: %s\n", SERIAL_PORT);
    printf("Baud rate: 9600\n");
    printf("Press Ctrl+C to exit\n\n");
}

/**
 * @brief Print troubleshooting suggestions for serial port issues
 */
void print_troubleshooting(void) {
    fprintf(stderr, "Troubleshooting suggestions:\n");
    fprintf(stderr, "1. Check if port exists: ls -la /dev/serial*\n");
    fprintf(stderr, "2. Check permissions: sudo usermod -a -G dialout $USER\n");
    fprintf(stderr, "3. Run as root: sudo ./test\n");
    fprintf(stderr, "4. Ensure no other program is using the port\n");
}

/**
 * @brief Main program entry point
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, 1 on failure
 */
int main(int argc, char *argv[]) {
    int serial_fd;                  // Serial port file descriptor
    float humidity, temperature;    // Sensor data variables
    int mq7_adc;                   // MQ7 sensor ADC value
    char data_buffer[64];          // Buffer for formatted data string
    int transmission_counter = 0;   // Counter for transmitted messages
    
    // Print program information
    print_usage_info();
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);   // Handle Ctrl+C
    signal(SIGTERM, signal_handler);  // Handle termination signal
    
    // Initialize random number generator with current time
    srand((unsigned int)time(NULL));
    
    // Configure and open serial port
    serial_fd = setup_serial(SERIAL_PORT);
    if (serial_fd == -1) {
        fprintf(stderr, "Error: Cannot open serial port %s\n", SERIAL_PORT);
        print_troubleshooting();
        return 1;
    }
    
    printf("Starting data transmission...\n\n");
    
    // Main transmission loop
    while (running) {
        // Generate realistic sensor data
        generate_sensor_data(&humidity, &temperature, &mq7_adc);
        
        // Format data string exactly as ESP32 would send it
        // Format: |humidity|temperature|ADC|\n
        int bytes_formatted = snprintf(data_buffer, sizeof(data_buffer), 
                                     "|%.2f|%.2f|%d|\n", 
                                     humidity, temperature, mq7_adc);
        
        // Validate buffer size
        if (bytes_formatted >= sizeof(data_buffer)) {
            fprintf(stderr, "Warning: Data buffer overflow, truncating data\n");
        }
        
        // Transmit data through UART
        ssize_t bytes_written = write(serial_fd, data_buffer, strlen(data_buffer));
        if (bytes_written < 0) {
            perror("Error writing to serial port");
            break;
        } else if (bytes_written != (ssize_t)strlen(data_buffer)) {
            fprintf(stderr, "Warning: Partial write - expected %zu bytes, wrote %zd bytes\n", 
                   strlen(data_buffer), bytes_written);
        }
        
        // Display transmission information
        printf("[%03d] Transmitted: %s", ++transmission_counter, data_buffer);
        printf("      Humidity: %.2f%%, Temperature: %.2f°C, MQ7 ADC: %d\n", 
               humidity, temperature, mq7_adc);
        
        // Wait before next transmission
        sleep(SEND_INTERVAL);
    }
    
    // Cleanup and graceful shutdown
    if (close(serial_fd) != 0) {
        perror("Error closing serial port");
    }
    
    printf("\nSerial port closed successfully. Program terminated.\n");
    printf("Total transmissions sent: %d\n", transmission_counter);
    
    return 0;
}