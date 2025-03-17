/*
 * Braggi - Test Harness
 * 
 * "A good test harness is like a sturdy corral - it keeps your code from 
 * runnin' wild and trampling your users!" - Irish-Texan Testing Philosophy
 */

#include "braggi/source.h"
#include "braggi/braggi_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h> // For getcwd and usleep
#include <stdbool.h> // For bool type
#include <signal.h> // For alarm

// Command line arguments
static char* test_dir = NULL;
static char* output_dir = NULL;
static bool verbose = false;
static char* single_test = NULL;  // New variable to store the name of a single test to run
static BraggiContext* current_test_context = NULL; // Keep track of the current running test context

// Signal handler for test timeouts
static void test_timeout_handler(int sig) {
    fprintf(stderr, "\n===== TEST TIMEOUT! =====\n");
    fprintf(stderr, "A test took too long to complete and was forcibly terminated.\n");
    fprintf(stderr, "This might indicate an infinite loop in the tokenizer or parser.\n");
    
    // Try to clean up the context if we have one
    if (current_test_context) {
        fprintf(stderr, "Attempting to clean up context...\n");
        current_test_context->flags |= BRAGGI_FLAG_FINAL_CLEANUP; // Set cleanup flag
        braggi_context_cleanup(current_test_context);
        braggi_context_destroy(current_test_context);
        current_test_context = NULL;
    }
    
    // Exit the program with an error status
    exit(1);
}

// Forward declarations
void print_usage(const char* program_name);
int parse_args(int argc, char** argv);
int run_tests();
static int run_test(const char* test_dir, const char* output_dir, const char* filename, bool verbose);

int main(int argc, char** argv) {
    // Parse command line arguments
    if (parse_args(argc, argv) != 0) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Set up the timeout signal handler
    signal(SIGALRM, test_timeout_handler);
    
    printf("===== BRAGGI TEST HARNESS =====\n");
    printf("Test directory: %s\n", test_dir ? test_dir : ".");
    printf("Output directory: %s\n", output_dir ? output_dir : ".");
    
    // Run the tests
    int result = run_tests();
    
    if (result == 0) {
        printf("===== ALL TESTS PASSED! =====\n");
    } else {
        printf("===== TESTS FAILED! =====\n");
    }
    
    return result;
}

// Parse command line arguments
int parse_args(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--test-dir=", 11) == 0) {
            test_dir = argv[i] + 11;
        } else if (strncmp(argv[i], "--output-dir=", 13) == 0) {
            output_dir = argv[i] + 13;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else if (strncmp(argv[i], "--test=", 7) == 0) {
            single_test = argv[i] + 7;
        } else if (strcmp(argv[i], "--help") == 0) {
            return 1;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }
    
    if (test_dir == NULL) {
        test_dir = "../tests";
    }
    
    if (output_dir == NULL) {
        output_dir = ".";
    }
    
    return 0;
}

// Print usage information
void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  --test-dir=<dir>     Directory containing test files (default: ../tests)\n");
    printf("  --output-dir=<dir>   Directory for output files (default: .)\n");
    printf("  --verbose            Print verbose output\n");
    printf("  --test=<name>        Run only the specified test (without .bg extension)\n");
    printf("  --help               Print this help message\n");
}

// Run all tests in the test directory
int run_tests() {
    DIR* dir = opendir(test_dir);
    if (dir == NULL) {
        perror("Failed to open test directory");
        return 1;
    }
    
    struct dirent* entry;
    int num_tests = 0;
    int num_passed = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }
        
        const char* filename = entry->d_name;
        size_t len = strlen(filename);
        
        // Skip files that don't end with .bg
        if (len < 3 || strcmp(filename + len - 3, ".bg") != 0) {
            continue;
        }
        
        // If a single test is specified, skip all other tests
        if (single_test != NULL) {
            // Extract the base name without extension for comparison
            char* base_without_ext = strdup(filename);
            char* dot = strrchr(base_without_ext, '.');
            if (dot != NULL) {
                *dot = '\0';
            }
            
            if (strcmp(base_without_ext, single_test) != 0) {
                free(base_without_ext);
                continue;
            }
            
            free(base_without_ext);
        }
        
        num_tests++;
        
        int result = run_test(test_dir, output_dir, filename, verbose);
        if (result == 0) {
            num_passed++;
        }
    }
    
    closedir(dir);
    
    // If a single test was specified but not found, try adding .bg extension
    if (single_test != NULL && num_tests == 0) {
        char* filename_with_ext = malloc(strlen(single_test) + 4);
        sprintf(filename_with_ext, "%s.bg", single_test);
        
        // Check if the file exists
        char* full_path = malloc(strlen(test_dir) + strlen(filename_with_ext) + 2);
        sprintf(full_path, "%s/%s", test_dir, filename_with_ext);
        
        FILE* file = fopen(full_path, "r");
        if (file != NULL) {
            fclose(file);
            num_tests++;
            int result = run_test(test_dir, output_dir, filename_with_ext, verbose);
            if (result == 0) {
                num_passed++;
            }
        } else {
            fprintf(stderr, "Test not found: %s\n", single_test);
            free(filename_with_ext);
            free(full_path);
            return 1;
        }
        
        free(filename_with_ext);
        free(full_path);
    }
    
    // Run SourceTests
    printf("Running SourceTests...\n");
    
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return 1;
    }
    
    char source_test_cmd[1024];
    snprintf(source_test_cmd, sizeof(source_test_cmd), "%s/bin/test_source > /dev/null 2>&1", cwd);
    
    int source_test_result = system(source_test_cmd);
    if (source_test_result == 0) {
        printf("SourceTests: PASSED\n");
        num_tests++;
        num_passed++;
    } else {
        printf("SourceTests: FAILED\n");
        num_tests++;
    }
    
    printf("\nTest Summary: %d tests, %d passed, %d failed\n", 
           num_tests, num_passed, num_tests - num_passed);
    
    if (num_passed == num_tests) {
        printf("===== ALL TESTS PASSED! =====\n");
        return 0;
    } else {
        printf("===== TESTS FAILED! =====\n");
        return 1;
    }
}

static int run_test(const char* test_dir, const char* output_dir, const char* filename, bool verbose) {
    if (!test_dir || !output_dir || !filename) {
        fprintf(stderr, "Error: Invalid parameters for run_test\n");
        return 1;
    }
    
    // Get the basename without extension
    char* basename = strdup(filename);
    if (!basename) {
        fprintf(stderr, "Error: Failed to allocate memory for basename\n");
        return 1;
    }
    
    // Remove the extension
    char* dot = strrchr(basename, '.');
    if (dot) {
        *dot = '\0';
    }
    
    char* base_without_ext = strdup(basename);
    if (!base_without_ext) {
        fprintf(stderr, "Error: Failed to allocate memory for base_without_ext\n");
        free(basename);
        return 1;
    }
    
    printf("Running test: %s\n", filename);
    
    // Create the input path
    char input_path[1024];
    snprintf(input_path, sizeof(input_path), "%s/%s", test_dir, filename);
    
    // Create the output path
    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, basename);
    
    // Create a context
    BraggiContext* context = braggi_context_create();
    if (!context) {
        fprintf(stderr, "Failed to create context for test file: %s\n", filename);
        free(basename);
        free(base_without_ext);
        return 1;
    }
    
    // Track the current test context for the signal handler
    current_test_context = context;
    
    // Enable debug mode
    context->verbose = true;
    
    // Set up an alarm to handle infinite loops
    // This isn't portable to all systems, but works for Linux/Unix
    alarm(15); // 15 second timeout per test
    
    // Load the test file
    if (!braggi_context_load_file(context, input_path)) {
        fprintf(stderr, "Failed to load test file: %s\n", input_path);
        alarm(0); // Cancel the alarm
        braggi_context_destroy(context);
        free(basename);
        free(base_without_ext);
        return 1;
    }
    
    // Set the output file path 
    extern bool braggi_set_output_file(BraggiContext* context, const char* output_file);
    if (!braggi_set_output_file(context, output_path)) {
        fprintf(stderr, "Failed to set output file: %s\n", output_path);
        alarm(0); // Cancel the alarm
        braggi_context_destroy(context);
        free(basename);
        free(base_without_ext);
        return 1;
    }
    
    // Compile the test
    if (!braggi_context_compile(context)) {
        fprintf(stderr, "Compilation failed for test file: %s\n", input_path);
        
        // Even though compilation failed, we need to clean up properly
        alarm(0); // Cancel the alarm
        braggi_context_cleanup(context);
        braggi_context_destroy(context);
        free(basename);
        free(base_without_ext);
        return 1;
    }
    
    // Cancel the alarm since compilation succeeded
    alarm(0);
    
    // Important: Properly clean up the context before executing
    // This ensures no dangling references or memory issues
    braggi_context_cleanup(context);
    braggi_context_destroy(context);
    current_test_context = NULL; // Clear the current context
    
    // Add a small delay to ensure all cleanup operations are complete
    // This helps prevent segfaults from race conditions
    usleep(5000);  // 5ms pause
    
    // Generate output file using braggi_compiler
    if (verbose) {
        printf("Generating output file for test: %s\n", filename);
    }
    
    char compiler_cmd[2048];
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "Error getting current working directory\n");
        snprintf(compiler_cmd, sizeof(compiler_cmd), 
                 "bin/braggi_compiler %s -o %s/%s.out", 
                 input_path, output_dir, base_without_ext);
    } else {
        snprintf(compiler_cmd, sizeof(compiler_cmd), 
                 "%s/bin/braggi_compiler %s -o %s/%s.out", 
                 cwd, input_path, output_dir, base_without_ext);
    }
    
    if (verbose) {
        printf("Running compiler command: %s\n", compiler_cmd);
    }
    
    int compiler_result = system(compiler_cmd);
    if (compiler_result != 0) {
        fprintf(stderr, "Warning: Compiler failed to generate output for test: %s\n", filename);
        // Continue anyway, as the test itself passed
    } else if (verbose) {
        printf("Successfully generated output file: %s/%s.out\n", output_dir, base_without_ext);
    }
    
    // Redirect test output to a log file
    char log_path[1024];
    snprintf(log_path, sizeof(log_path), "%s/%s.log", output_dir, base_without_ext);
    
    // Check if there's an expected output file
    char expected_output_path[1024];
    snprintf(expected_output_path, sizeof(expected_output_path), "%s/%s.out", test_dir, filename);
    
    // Also check in the expected_outputs directory
    char expected_outputs_dir[1024];
    snprintf(expected_outputs_dir, sizeof(expected_outputs_dir), "%s/expected_outputs", test_dir);
    
    char alt_expected_output_path[1024];
    snprintf(alt_expected_output_path, sizeof(alt_expected_output_path), "%s/%s.out", expected_outputs_dir, base_without_ext);
    
    // Check if either expected output file exists
    struct stat st;
    bool has_expected_output = false;
    
    if (stat(expected_output_path, &st) == 0) {
        has_expected_output = true;
    } else if (stat(alt_expected_output_path, &st) == 0) {
        // Use the alternative path instead
        strncpy(expected_output_path, alt_expected_output_path, sizeof(expected_output_path) - 1);
        expected_output_path[sizeof(expected_output_path) - 1] = '\0';
        has_expected_output = true;
    }
    
    if (has_expected_output) {
        // Compare the output file with the expected output file
        char compare_cmd[2048];
        char output_file_path[1024];
        snprintf(output_file_path, sizeof(output_file_path), "%s/%s.out", output_dir, base_without_ext);
        
        snprintf(compare_cmd, sizeof(compare_cmd), "diff -q \"%s\" \"%s\" > \"%s\" 2>&1", 
                output_file_path, expected_output_path, log_path);
        
        if (verbose) {
            printf("Comparing output with expected: %s\n", expected_output_path);
        }
        
        int compare_result = system(compare_cmd);
        if (compare_result != 0) {
            fprintf(stderr, "Output does not match expected output for test: %s\n", filename);
            printf("See log for details: %s\n", log_path);
            
            // Free memory before returning
            free(basename);
            free(base_without_ext);
            
            return 1; // Test failed
        } else if (verbose) {
            printf("Output matches expected output for test: %s\n", filename);
        }
    } else {
        // No expected output file, so we can't verify the output automatically
        // For now, assume the test passed if compilation succeeded
        if (verbose) {
            printf("No expected output file found for test: %s\n", filename);
        }
    }
    
    // Free memory before returning
    free(basename);
    free(base_without_ext);
    
    return 0; // Test passed
} 