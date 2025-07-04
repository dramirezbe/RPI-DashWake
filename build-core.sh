#!/bin/bash

# --- Configuration ---
VERBOSE=false # Change to true for verbose output

# --- Helper function for conditional logging ---
log_message() {
    if $VERBOSE; then
        echo "$@"
    fi
}

# Detect the script's own directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

log_message "Script is located at: $SCRIPT_DIR"

# Define the Core directory path
CORE_DIR="$SCRIPT_DIR/Core"

# Define the build directory path
BUILD_DIR_IN_CORE="$CORE_DIR/build"

# --- Cleanup previous build artifacts ---

log_message "--- Cleaning up previous build artifacts ---"

# Remove the build directory if it exists
if [ -d "$BUILD_DIR_IN_CORE" ]; then
    log_message "Removing existing build directory: $BUILD_DIR_IN_CORE"
    rm -rf "$BUILD_DIR_IN_CORE"
else
    log_message "No existing build directory found at: $BUILD_DIR_IN_CORE"
fi

log_message "--- Cleanup complete ---"

# --- Start new build process ---

# Check if the Core directory exists
if [ ! -d "$CORE_DIR" ]; then
    echo "Error: 'Core' directory not found at $CORE_DIR" # This error is critical, always show
    exit 1
fi

log_message "Navigating to Core directory: $CORE_DIR"
cd "$CORE_DIR"

# Create build directory, and navigate into it
# We already ensured it doesn't exist by removing it above, so we just create it.
log_message "Creating build directory..."
mkdir build

log_message "Navigating to build directory..."
cd build

# Run CMake to configure the project (output always visible)
echo "Running CMake to configure project..."
cmake ..

# Build the project (output always visible)
echo "Building the project..."
cmake --build .

# Check if the build was successful and the executable was created inside the build directory
if [ -f "DashWake-Core-exec" ]; then
    echo "Building Done."
    # If verbose, also show the full path
    if $VERBOSE; then
        echo "Executable created in: $PWD/DashWake-Core-exec"
    fi
else
    echo "Building Failed."
    # If verbose, show more details about the failure
    if $VERBOSE; then
        echo "Executable 'DashWake-Core-exec' not found in: $PWD"
    fi
    exit 1
fi

# No final "Build process completed." message here, as "Building Done." covers it.