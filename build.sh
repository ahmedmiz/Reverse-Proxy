#!/bin/bash

# Configure build type (Debug or Release)
BUILD_TYPE=${1:-Debug}
BUILD_DIR="build_${BUILD_TYPE}"

if [ "$1" == "test" ]; then
    if [ -d "build_debug_test" ]; then
        echo "Running tests in $BUILD_DIR..."
        cd "build_debug_test"
        ctest -V
        exit 0
    else
        echo "Error: Build directory $BUILD_DIR does not exist. Run the build first."
        exit 1
    fi
fi

# Create build directory
mkdir -p $BUILD_DIR 2>/dev/null 
cd $BUILD_DIR

# Configure via CMake
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. >/dev/null 2>&1 || {
    echo "Error: CMake configuration failed."
    exit 1
}

# Get number of CPU cores for parallel build
if [ -f /proc/cpuinfo ]; then
    # Linux
    CORES=$(grep -c ^processor /proc/cpuinfo)
elif [ "$(uname)" == "Darwin" ]; then
    # macOS
    CORES=$(sysctl -n hw.ncpu)
else
    # Default to 2 cores
    CORES=2
fi

# Build with multiple cores
cmake --build . -- -j$CORES 


# Run tests if requested
if [ "$2" == "test" ]; then
    echo "Running tests..."
    ctest -V
fi

echo "Build complete: $BUILD_TYPE configuration"