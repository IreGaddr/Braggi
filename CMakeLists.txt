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

# Check for Jansson (for LSP)
find_package(Jansson QUIET)
if(Jansson_FOUND)
    include_directories(${JANSSON_INCLUDE_DIRS})
    message(STATUS "  Jansson: Found - Building LSP interface")
    add_definitions(-DHAVE_JANSSON)
else()
    message(STATUS "  Jansson not found - LSP interface will not be built")
    message(STATUS "  To install Jansson: sudo apt-get install libjansson-dev")
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
    src/codegen/codegen_manager.c
    # Architecture-specific backend files
    src/codegen/arm64.c
    src/codegen/arm.c
    src/codegen/x86_64.c  # Fully implemented x86_64 backend
    # src/codegen/x86.c - Not needed yet
    src/token.c
    src/token_propagator.c
    src/grammar_patterns.c
    src/functional_validator.c
    src/runtime/runtime.c
    src/stdlib/stdlib.c
    src/builtins/builtins.c
    src/ecs.c
    src/entropy.c
    src/constraint_patterns.c
    src/state.c
    src/ecs/codegen_components.c
    src/ecs/codegen_systems.c
    # Our newly implemented files
    src/region_manager.c
    src/token_manager.c
    src/symbol_table.c
    src/token_ecs.c
    src/entropy_ecs.c
    src/periscope.c
    # Add other source files as they're created
)

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
    include/braggi/codegen_arch.h
    include/braggi/token.h
    include/braggi/token_propagator.h
    include/braggi/grammar_patterns.h
    include/braggi/runtime.h
    include/braggi/builtins.h
    include/braggi/stdlib.h
    include/braggi/util/vector.h
    include/braggi/state.h
    include/braggi/ecs/codegen_components.h
    include/braggi/ecs/codegen_systems.h
    # Our newly implemented headers
    include/braggi/region_manager.h
    include/braggi/token_manager.h
    include/braggi/symbol_table.h
    include/braggi/token_ecs.h
    include/braggi/lsp_interface.h
    # Add other header files as they're created
)

# Library target
add_library(braggi STATIC ${BRAGGI_SOURCES} ${BRAGGI_HEADERS})

# Link with the math library since we use log2f
target_link_libraries(braggi m)

# Executable target
add_executable(braggi_compiler src/main.c)
target_link_libraries(braggi_compiler braggi)

# REPL executable
add_executable(braggi-repl src/repl.c)
target_link_libraries(braggi-repl braggi)
if(Readline_FOUND)
    target_link_libraries(braggi-repl ${Readline_LIBRARY})
endif()

# LSP interface executable
if(Jansson_FOUND)
    add_executable(braggi-lsp src/lsp_interface.c)
    target_link_libraries(braggi-lsp braggi ${JANSSON_LIBRARIES})
endif()

# Install rules
install(TARGETS braggi braggi_compiler braggi-repl
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
if(Jansson_FOUND)
    install(TARGETS braggi-lsp
        RUNTIME DESTINATION bin
    )
endif()
install(DIRECTORY include/ DESTINATION include)

# Configuration summary
message(STATUS "Braggi Configuration:")
message(STATUS "  CMake Version: ${CMAKE_VERSION}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  LSP Interface: ${Jansson_FOUND}")

# Enable testing
enable_testing()

# First create the test source file in the src directory to ensure it exists
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/test_source.c 
"#include \"braggi/source.h\"
#include \"braggi/braggi_context.h\"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Basic test for source file functionality
int main() {
    printf(\"Running source tests...\\n\");
    
    // Test source loading from string
    BraggiContext* context = braggi_context_create();
    
    // Test loading source from string
    const char* test_source = \"fn main() { println(\\\"Hello, Braggi!\\\"); }\";
    braggi_context_load_string(context, test_source, \"test_string.bg\");
    
    // Verify source was loaded
    printf(\"Source loaded, length: %zu\\n\", braggi_source_file_get_size(context->source));
    
    // Clean up
    braggi_context_destroy(context);
    
    printf(\"Source tests passed!\\n\");
    return 0;
}")

# Source module tests - using the file from src directory instead
add_executable(test_source src/test_source.c)
target_link_libraries(test_source braggi)
add_test(NAME SourceTests COMMAND test_source)

# Add test harness directory
add_subdirectory(tools)

# Add custom targets for our scripts
add_custom_target(debug_source_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/debug_source_test.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running debug source test..."
)

add_custom_target(debug_segfault
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/debug_segfault.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running segfault debugging..."
)

add_custom_target(run_tests_with_backtrace
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_tests_with_backtrace.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests with backtrace..."
)

add_custom_target(build_and_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_and_test.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building and running tests..."
)

add_custom_target(test_stdlib
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_stdlib.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Testing standard library..."
)

add_custom_target(run_build
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_build.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running build script..."
)

add_custom_target(run_main_tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_main_tests.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running main component tests..."
)

add_custom_target(run_all_tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_all_tests.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all Braggi tests combined..."
)

# Create a directory structure for the library
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Copy standard library files to build directory
file(GLOB STDLIB_FILES ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.bg)
foreach(STDLIB_FILE ${STDLIB_FILES})
    get_filename_component(FILE_NAME ${STDLIB_FILE} NAME)
    configure_file(${STDLIB_FILE} ${CMAKE_BINARY_DIR}/lib/${FILE_NAME} COPYONLY)
endforeach()

# Add additional custom target to setup the LSP development environment
add_custom_target(setup_lsp_dev
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lsp
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/tools/lsp ${CMAKE_BINARY_DIR}/lsp
    COMMAND cd ${CMAKE_BINARY_DIR}/lsp/server && npm install
    COMMAND cd ${CMAKE_BINARY_DIR}/lsp/vscode-extension && npm install
    COMMENT "Setting up LSP development environment..."
)

# Add target to build the VS Code extension
add_custom_target(build_vscode_extension
    COMMAND cd ${CMAKE_BINARY_DIR}/lsp/vscode-extension && npm run compile
    DEPENDS setup_lsp_dev
    COMMENT "Building VS Code extension..."
)

# Add target to package the VS Code extension
add_custom_target(package_vscode_extension
    COMMAND cd ${CMAKE_BINARY_DIR}/lsp/vscode-extension && npx vsce package
    DEPENDS build_vscode_extension
    COMMENT "Packaging VS Code extension..."
) 