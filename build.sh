#!/bin/bash
# filepath: e:\Projects\my_memory_lib\build.sh
# Professional build script for memory arena library
set -e  # Exit on any error

# Color formatting for output messages
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Compiler settings
CC=gcc
COMMON_FLAGS="-Wall -Wextra -pedantic"
DEBUG_FLAGS="-g -O0 -DDEBUG"
RELEASE_FLAGS="-O3 -DNDEBUG"

# Library and test targets
LIB_NAME="memory_arena"
TEST_NAME="memory_arena_test"

# Default build mode
BUILD_TYPE="release"

# Source files (space-separated lists instead of arrays)
LIB_SOURCES="memory_arena.c"
TEST_SOURCES="game_test.c memory_arena.c"

# Directory structure
SRC_DIR="src"
OBJ_DIR="obj"
BIN_DIR="bin"

# Output info
echo_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Output success
echo_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Output warning
echo_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Output error and exit
echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
    exit 1
}

# Show help information
show_help() {
    echo "Usage: $0 [options] [target]"
    echo ""
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo "  -d, --debug    Build with debug flags"
    echo "  -r, --release  Build with release flags (default)"
    echo ""
    echo "Targets:"
    echo "  all            Build library and tests (default)"
    echo "  lib            Build only the library"
    echo "  test           Build the memory arena test"
    echo "  clean          Remove build artifacts"
    echo ""
    echo "Examples:"
    echo "  $0             # Build everything with release flags"
    echo "  $0 -d test     # Build the test with debug flags"
    echo "  $0 clean       # Clean all build artifacts"
}

# Create required directories
create_directories() {
    echo_info "Creating directory structure..."
    mkdir -p "$SRC_DIR" "$OBJ_DIR" "$BIN_DIR"
}

# Clean all build artifacts
clean() {
    echo_info "Cleaning build artifacts..."
    rm -rf "$OBJ_DIR" "$BIN_DIR"
    create_directories
    echo_success "Clean completed"
}

# Set build flags based on type
set_build_flags() {
    if [ "$BUILD_TYPE" == "debug" ]; then
        CFLAGS="-Wall -Wextra -pedantic -g -O0 -DDEBUG"
        echo_info "Building with debug flags"
    else
        CFLAGS="-Wall -Wextra -pedantic -O3 -DNDEBUG"
        echo_info "Building with release flags"
    fi
}

# Check if source files exist
check_sources() {
    local src
    for src in $1; do
        if [ ! -f "$SRC_DIR/$src" ]; then
            echo_error "Source file not found: $SRC_DIR/$src"
        fi
    done
}

# Compile an object file
compile_object() {
    local src="$1"
    local obj="$OBJ_DIR/${src%.c}.o"
    
    echo_info "Compiling $src..."
    $CC $CFLAGS -I"$SRC_DIR" -c "$SRC_DIR/$src" -o "$obj" 2>&1 || {
        echo_error "Failed to compile $src"
    }
    
    echo_success "Compiled $obj"
    echo "${src%.c}.o"
}

# Build the library
build_lib() {
    echo_info "Building $LIB_NAME library..."
    check_sources "$LIB_SOURCES"
    local objects=""
    
    for src in $LIB_SOURCES; do
        compile_object "$src" > /dev/null
        objects="$objects $OBJ_DIR/${src%.c}.o"
    done
    
    ar rcs "$BIN_DIR/lib${LIB_NAME}.a" $objects 2>&1 || {
        echo_error "Failed to create static library $LIB_NAME"
    }
    
    echo_success "Built lib${LIB_NAME}.a"
}

# Build the memory arena tests
build_test() {
    echo_info "Building $TEST_NAME..."
    check_sources "$TEST_SOURCES"
    local objects=""
    
    for src in $TEST_SOURCES; do
        compile_object "$src" #> /dev/null
        objects="$objects $OBJ_DIR/${src%.c}.o"
    done
    
    $CC $CFLAGS $objects -o "$BIN_DIR/${TEST_NAME}.exe" 2>&1 || {
        echo_error "Failed to link $TEST_NAME"
    }
    
    echo_success "Built ${TEST_NAME}.exe"
}

# Parse command line arguments
TARGET="all"

while [ "$#" -gt 0 ]; do
    case "$1" in
        -h|--help)
            show_help
            exit 0
            ;;
        -d|--debug)
            BUILD_TYPE="debug"
            shift
            ;;
        -r|--release)
            BUILD_TYPE="release"
            shift
            ;;
        all|lib|test|clean)
            TARGET="$1"
            shift
            ;;
        *)
            echo_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Set build flags
set_build_flags

# Execute the target
case "$TARGET" in
    all)
        create_directories
        build_lib
        build_test
        ;;
    lib)
        create_directories
        build_lib
        ;;
    test)
        create_directories
        build_test
        ;;
    clean)
        clean
        ;;
esac

echo_success "Build completed successfully"