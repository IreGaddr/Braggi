#!/bin/bash

# Braggi Build Script
# "Good code builds like a well-made whiskey - smooth, strong, and worth the wait!"

# Set some pretty colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if the user wants to reset the build directory
RESET_BUILD=0
for arg in "$@"; do
    if [ "$arg" == "--reset" ] || [ "$arg" == "-r" ]; then
        RESET_BUILD=1
    fi
done

echo -e "${BLUE}=======================================${NC}"
echo -e "${BLUE}     BRAGGI BUILD SCRIPT ${NC}"
echo -e "${BLUE}     Fixin' to make some magic...     ${NC}"
echo -e "${BLUE}=======================================${NC}"

# Determine the project root directory (where the top-level CMakeLists.txt is)
PROJECT_ROOT=$(dirname "$(dirname "$(readlink -f "$0")")")
BUILD_DIR="${PROJECT_ROOT}/build"

# Check for ninja build system (faster builds)
if command -v ninja &> /dev/null; then
    CMAKE_GENERATOR="-GNinja"
    echo -e "${GREEN}Using Ninja build system for faster builds${NC}"
else
    CMAKE_GENERATOR=""
    echo -e "${YELLOW}Ninja build system not found. Using default generator.${NC}"
    echo -e "${YELLOW}For faster builds, consider installing Ninja:${NC}"
    echo -e "${YELLOW}  sudo apt-get install ninja-build${NC}"
fi

# Check if we need to reset the build directory
if [ $RESET_BUILD -eq 1 ]; then
    echo -e "${YELLOW}Resetting build directory as requested...${NC}"
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        echo -e "${GREEN}Build directory removed!${NC}"
    fi
fi

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Creating build directory at ${BUILD_DIR}${NC}"
    mkdir -p "$BUILD_DIR"
else
    # Check if we need to clean due to generator switch
    if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
        current_generator=$(grep "CMAKE_GENERATOR" "$BUILD_DIR/CMakeCache.txt" | cut -d= -f2)
        
        # If we're switching generators, clean the build dir
        if [[ "$CMAKE_GENERATOR" == *"Ninja"* && "$current_generator" != *"Ninja"* ]] || 
           [[ "$CMAKE_GENERATOR" == "" && "$current_generator" == *"Ninja"* ]]; then
            echo -e "${YELLOW}Detected generator switch. Cleaning build directory...${NC}"
            rm -rf "$BUILD_DIR/CMakeCache.txt" "$BUILD_DIR/CMakeFiles"
            echo -e "${GREEN}Build directory cleaned!${NC}"
        fi
    fi
fi

# Move to build directory
cd "$BUILD_DIR" || { echo -e "${RED}Failed to cd into ${BUILD_DIR}${NC}"; exit 1; }

# Run CMake configuration step
echo -e "${BLUE}Configuring project with CMake...${NC}"
cmake $CMAKE_GENERATOR -DCMAKE_BUILD_TYPE=Debug .. || { 
    echo -e "${RED}CMake configuration failed!${NC}"
    echo -e "${YELLOW}Tip: Check for missing dependencies or configuration errors.${NC}"
    echo -e "${YELLOW}If errors persist, try running with the --reset flag:${NC}"
    echo -e "${YELLOW}  ${PROJECT_ROOT}/scripts/run_build.sh --reset${NC}"
    exit 1
}

# Run the build
echo -e "${BLUE}Building Braggi...${NC}"
cmake --build . || {
    echo -e "${RED}Build failed!${NC}"
    echo -e "${YELLOW}Tip: Check the compiler errors above.${NC}"
    exit 1
}

# Create bin and lib dirs if they don't exist (just in case)
mkdir -p bin lib

# Print success message
echo -e "${GREEN}=======================================${NC}"
echo -e "${GREEN}     BUILD COMPLETED SUCCESSFULLY     ${NC}"
echo -e "${GREEN}=======================================${NC}"
echo -e "${BLUE}Binaries can be found in:${NC} ${BUILD_DIR}/bin"
echo -e "${BLUE}Libraries can be found in:${NC} ${BUILD_DIR}/lib"
echo -e "${YELLOW}Yeehaw! Built faster than a jackrabbit on a hot day!${NC}"

exit 0 