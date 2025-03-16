# Braggi Tools CMakeLists.txt
# "Good tools make the job go smoother, like a good bourbon makes the evenin' go smoother!" - Texan wisdom

# Test harness tool
add_executable(braggi_test_harness
    test_harness.c
)
target_link_libraries(braggi_test_harness braggi)

# Set output directory
set_target_properties(braggi_test_harness
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Create test output directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_output)

# Copy test files to build directory
file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../tests/*.bg)
foreach(TEST_FILE ${TEST_FILES})
    get_filename_component(FILE_NAME ${TEST_FILE} NAME)
    configure_file(${TEST_FILE} ${CMAKE_BINARY_DIR}/tests/${FILE_NAME} COPYONLY)
endforeach()

# Add a test target that runs all Braggi tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_BINARY_DIR}/bin/braggi_test_harness --test-dir=${CMAKE_BINARY_DIR}/tests --output-dir=${CMAKE_BINARY_DIR}/test_output
    DEPENDS braggi_test_harness
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all Braggi tests..."
)

# Check if we have readline
find_package(Readline QUIET)

# REPL source files
set(REPL_SOURCES
    repl.c
)

# Add conditionals for readline
if(Readline_FOUND)
    message(STATUS "Readline found - REPL will have full functionality")
    add_executable(braggi_repl ${REPL_SOURCES})
    target_link_libraries(braggi_repl braggi readline)
    target_compile_definitions(braggi_repl PRIVATE -DHAVE_READLINE)
else()
    message(STATUS "Building simplified REPL without readline support")
    # Create a simple version that doesn't depend on readline
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/simple_repl.c "#include <stdio.h>\n#include \"braggi/braggi.h\"\n#include \"braggi/stdlib.h\"\n\nint main() {\n    printf(\"Braggi REPL (Simple version without readline)\\n\");\n    printf(\"Just a placeholder - not fully functional\\n\");\n    return 0;\n}\n")
    add_executable(braggi_repl simple_repl.c)
    target_link_libraries(braggi_repl braggi)
endif()

# Install rules
install(TARGETS braggi_test_harness braggi_repl
    RUNTIME DESTINATION bin
) 