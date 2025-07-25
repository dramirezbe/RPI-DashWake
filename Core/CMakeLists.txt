cmake_minimum_required(VERSION 3.10)

project(DashWake-Core-exec C)

# Set the C standard explicitly
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON) # Make sure C99 is strictly enforced

# Find the cJSON package
# This will set CJSON_FOUND, CJSON_INCLUDE_DIRS, and CJSON_LIBRARIES (or create an imported target)
find_package(cJSON CONFIG REQUIRED)

# Include directories for headers
# Add your own Modules directory
include_directories(Modules)

# Add cJSON's include directories if found by find_package
if(CJSON_FOUND)
    target_include_directories(DashWake-Core-exec PRIVATE ${cJSON_INCLUDE_DIRS})
    # If cJSON provides an imported target (e.g., cJSON::cJSON), we'll use that for linking.
    # Otherwise, we'll use the CJSON_LIBRARIES variable.
endif()

# Find all .c files in the Modules directory
file(GLOB MODULE_SOURCES "Modules/*.c")

# Add the main executable, including main.c + modules
add_executable(DashWake-Core-exec main.c ${MODULE_SOURCES})

# Link libraries (wiringPi, cjson, pthread)
# Use the cJSON imported target if available, otherwise use the found libraries
if(TARGET cJSON::cJSON)
    target_link_libraries(DashWake-Core-exec PRIVATE wiringPi cJSON::cJSON pthread)
else()
    target_link_libraries(DashWake-Core-exec PRIVATE wiringPi cjson pthread) # Fallback to direct library name
endif()

