# Braggi - Wave Function Constraint Collapse Compilation
# CMake build configuration
# "Building code faster than a Texas tornado, with Irish precision!"

cmake_minimum_required(VERSION 3.10)
project(Braggi VERSION 0.1.0 LANGUAGES C)

# Add our custom cmake modules path
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Check for readline (for REPL)
find_package(Readline QUIET)
if(Readline_FOUND)
    include_directories(${Readline_INCLUDE_DIR})
    message(STATUS "  Readline: Found - Building full REPL")
    add_definitions(-DHAVE_READLINE)
else()
    message(STATUS "  Readline not found - REPL will have limited functionality")
    message(STATUS "  To install readline: sudo apt-get install libreadline-dev")
    message(STATUS "Building simplified REPL without readline support")
endif()

# Create necessary directories for test outputs
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_output)

# Source files
set(BRAGGI_SOURCES
    src/util/vector.c
    src/util/hashmap.c
    src/error.c
    src/error_handler.c
    src/source.c
    src/source_position.c
    src/region.c
    src/allocation.c
    src/braggi_context.c
    src/braggi.c
    src/codegen/codegen.c
    # Comment out missing architecture-specific files for now
    # src/codegen/arm64.c
    # src/codegen/arm.c
    # src/codegen/x86_64.c
    # src/codegen/x86.c
    src/token.c
    src/token_propagator.c
    src/grammar_patterns.c
    src/functional_validator.c
    src/runtime/runtime.c
    src/stdlib/stdlib.c
    src/builtins/builtins.c
    src/codegen/x86_64.c
    src/ecs.c
    # Add other source files as they're created
)

# Create placeholder/stub files for missing required sources
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen/x86_64.c "// Placeholder for x86_64 codegen\n#include \"braggi/codegen.h\"\n\n// This is a stub implementation\n")
file(APPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen/x86_64.c "void braggi_codegen_x86_64_init(void) {\n    // Stub implementation\n}\n")

# Add the file back to sources now that we've created it
list(APPEND BRAGGI_SOURCES src/codegen/x86_64.c)

# Header files for IDE integration
set(BRAGGI_HEADERS
    include/braggi/braggi.h
    include/braggi/braggi_context.h
    include/braggi/entropy.h
    include/braggi/constraint.h
    include/braggi/region.h
    include/braggi/source.h
    include/braggi/error.h
    include/braggi/allocation.h
    include/braggi/codegen.h
    include/braggi/token.h
    include/braggi/token_propagator.h
    include/braggi/grammar_patterns.h
    include/braggi/runtime.h
    include/braggi/builtins.h
    include/braggi/stdlib.h
    include/braggi/util/vector.h
    # Add other header files as they're created
)

# Library target
add_library(braggi STATIC ${BRAGGI_SOURCES} ${BRAGGI_HEADERS})

# Executable target
add_executable(braggi_compiler src/main.c)
target_link_libraries(braggi_compiler braggi)

# REPL executable
add_executable(braggi-repl src/repl.c)
target_link_libraries(braggi-repl braggi)
if(Readline_FOUND)
    target_link_libraries(braggi-repl ${Readline_LIBRARY})
endif()

# Install rules
install(TARGETS braggi braggi_compiler braggi-repl
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)

# Configuration summary
message(STATUS "Braggi Configuration:")
message(STATUS "  CMake Version: ${CMAKE_VERSION}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")

# Enable testing
enable_testing()

# Source module tests
add_executable(test_source tests/test_source.c)
target_link_libraries(test_source braggi)
add_test(NAME SourceTests COMMAND test_source)

# Add test harness directory
add_subdirectory(tools)

# Add the test target
add_test(NAME braggi_tests
         COMMAND ${CMAKE_BINARY_DIR}/bin/braggi_test_harness --test-dir=${CMAKE_BINARY_DIR}/tests --output-dir=${CMAKE_BINARY_DIR}/test_output)

# Create a directory structure for the library
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Copy standard library files to build directory
file(GLOB STDLIB_FILES ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.bg)
foreach(STDLIB_FILE ${STDLIB_FILES})
    get_filename_component(FILE_NAME ${STDLIB_FILE} NAME)
    configure_file(${STDLIB_FILE} ${CMAKE_BINARY_DIR}/lib/${FILE_NAME} COPYONLY)
endforeach() 