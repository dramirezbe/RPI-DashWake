cmake_minimum_required(VERSION 3.10)

project(DashWake-Core-exec C)

# Add the main executable
add_executable(DashWake-Core-exec main.c)

# Include directories for modules
include_directories(Modules)

# Find all .c files in the Modules directory and add them to the executable
file(GLOB MODULE_SOURCES "Modules/*.c")
target_sources(DashWake-Core-exec PRIVATE ${MODULE_SOURCES})

# Link the gpiod library
target_link_libraries(DashWake-Core-exec PRIVATE wiringPi wiringPiDev)

# Optional: If you also need to install the executable
# install(TARGETS DashWake-Core-exec DESTINATION bin)