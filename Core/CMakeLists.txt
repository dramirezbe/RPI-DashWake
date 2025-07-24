cmake_minimum_required(VERSION 3.10)

project(DashWake-Core-exec C)

# Set the C standard explicitly (opcional pero recomendable)
set(CMAKE_C_STANDARD 99)

# Include directories for headers
include_directories(Modules)

# Find all .c files in the Modules directory
file(GLOB MODULE_SOURCES "Modules/*.c")

# Add the main executable, including main.c + modules
add_executable(DashWake-Core-exec main.c ${MODULE_SOURCES})

# Link libraries (wiringPi, cjson, pthread)
target_link_libraries(DashWake-Core-exec PRIVATE wiringPi cjson pthread)
