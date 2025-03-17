/*
 * Braggi - Simple REPL (Read-Eval-Print Loop)
 * 
 * "A good REPL is like a good conversation at an Irish pub - 
 * it responds promptly and doesn't judge ya too harshly!" - Texirish wisdom
 */

#include "braggi/braggi.h"
#include "braggi/source.h"
#include "braggi/braggi_context.h"
#include "braggi/stdlib.h"  // Include the stdlib header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define MAX_INPUT_LENGTH 1024

// Welcome message with some Texan flair
const char* WELCOME_MESSAGE = 
    "╔═══════════════════════════════════════════════════════════════╗\n"
    "║                   Welcome to Braggi REPL                      ║\n"
    "║                                                               ║\n"
    "║   Type your code directly, or use one of these commands:      ║\n"
    "║   :help   - Display help information                          ║\n"
    "║   :load   - Load code from a file                             ║\n"
    "║   :import - Import a standard library module                  ║\n"
    "║   :quit   - Exit the REPL                                     ║\n"
    "╚═══════════════════════════════════════════════════════════════╝";

// Forward declarations
static char* get_input(const char* prompt);
static void process_command(const char* input, bool* should_exit);
static void print_help(void);
static void handle_special_command(const char* cmd, BraggiContext* context);

// Global context for the REPL session
static BraggiContext* g_repl_context = NULL;

int main(void) {
    char* input;
    bool should_exit = false;
    
    // Create a global context for the REPL session
    g_repl_context = braggi_context_create();
    if (!g_repl_context) {
        fprintf(stderr, "Failed to create Braggi context. Exiting.\n");
        return 1;
    }
    
    // Initialize the standard library
    if (!braggi_stdlib_initialize(g_repl_context)) {
        fprintf(stderr, "Failed to initialize standard library. Continuing anyway.\n");
    }
    
    // Display welcome message
    printf("%s\n", WELCOME_MESSAGE);
    
    // Main REPL loop
    while (!should_exit) {
        // Get user input
        input = get_input("braggi> ");
        
        // Check for EOF
        if (!input) {
            printf("Goodbye!\n");
            break;
        }
        
        // Process the command
        process_command(input, &should_exit);
        
        // Clean up
        free(input);
    }
    
    // Clean up the standard library
    braggi_stdlib_cleanup(g_repl_context);
    
    // Clean up the context
    braggi_context_destroy(g_repl_context);
    g_repl_context = NULL;
    
    return 0;
}

// Get input from user, using readline if available
static char* get_input(const char* prompt) {
#ifdef HAVE_READLINE
    char* input = readline(prompt);
    if (input && *input) {
        add_history(input);
    }
    return input;
#else
    static char buffer[MAX_INPUT_LENGTH];
    char* result = NULL;
    size_t input_size = 0;
    size_t total_read = 0;
    
    printf("%s", prompt);
    fflush(stdout);
    
    // Handle potentially large inputs that exceed buffer size
    while (1) {
        // Read a chunk of input
        if (fgets(buffer + total_read, MAX_INPUT_LENGTH - total_read, stdin) == NULL) {
            // Check for EOF or error
            if (feof(stdin)) {
                // EOF reached - could be Ctrl+D
                if (total_read == 0) {
                    // No input received before EOF, return NULL to signal exit
                    return NULL;
                } else {
                    // We got some input before EOF, so process what we have
                    break;
                }
            } else {
                // Some other error occurred
                fprintf(stderr, "Error reading input: %s\n", strerror(errno));
                return NULL;
            }
        }
        
        // Calculate how much we read
        size_t chunk_len = strlen(buffer + total_read);
        total_read += chunk_len;
        
        // Check if we got a newline, which indicates end of input
        if (total_read > 0 && buffer[total_read - 1] == '\n') {
            // Remove the trailing newline
            buffer[total_read - 1] = '\0';
            total_read--;
            break;
        }
        
        // Check if we've filled the buffer
        if (total_read >= MAX_INPUT_LENGTH - 1) {
            fprintf(stderr, "Warning: Input too long, truncating to %d characters\n", MAX_INPUT_LENGTH - 1);
            buffer[MAX_INPUT_LENGTH - 1] = '\0';
            break;
        }
    }
    
    // Now we need to allocate and copy the result
    if (total_read > 0) {
        // We have actual input to return
        result = strdup(buffer);
        if (!result) {
            fprintf(stderr, "Error: Failed to allocate memory for input\n");
            return NULL;
        }
    } else {
        // No input received (empty line)
        result = strdup("");
        if (!result) {
            fprintf(stderr, "Error: Failed to allocate memory for empty input\n");
            return NULL;
        }
    }
    
    return result;
#endif
}

// Print help information
static void print_help(void) {
    printf("\nAvailable commands:\n");
    printf("  help, ?            Show this help message\n");
    printf("  :source <filename> Load a source file (not implemented yet)\n");
    printf("  :import <module>   Import a standard library module\n");
    printf("  exit, quit, q      Exit the REPL\n");
    printf("\nAny other input will be processed as Braggi code.\n");
    printf("(Actual interpretation not implemented yet)\n\n");
}

// Handle special commands starting with ':'
static void handle_special_command(const char* cmd, BraggiContext* context) {
    if (strncmp(cmd, "source ", 7) == 0 || strncmp(cmd, "load ", 5) == 0) {
        const char* filename = cmd;
        if (strncmp(cmd, "source ", 7) == 0) {
            filename = cmd + 7;
        } else {
            filename = cmd + 5;
        }
        printf("Would load source file: %s (not implemented yet)\n", filename);
    } else if (strncmp(cmd, "import ", 7) == 0) {
        const char* module = cmd + 7;
        printf("Importing module: %s\n", module);
        
        // Try to load the module
        if (braggi_stdlib_load_module(context, module)) {
            printf("Successfully imported module: %s\n", module);
        } else {
            printf("Error: Failed to import module: %s\n", module);
        }
    } else {
        printf("Unknown command: :%s\n", cmd);
    }
}

// Process a command
static void process_command(const char* input, bool* should_exit) {
    if (!input || !*input) {
        return;
    }
    
    // Check for exit commands
    if (strcmp(input, "exit") == 0 || 
        strcmp(input, "quit") == 0 || 
        strcmp(input, "q") == 0) {
        *should_exit = true;
        printf("Goodbye!\n");
        return;
    }
    
    // Check for help command
    if (strcmp(input, "help") == 0 || strcmp(input, "?") == 0) {
        print_help();
        return;
    }
    
    // Check for special commands
    if (input[0] == ':') {
        handle_special_command(input + 1, g_repl_context);
        return;
    }
    
    // Process as Braggi code
    if (g_repl_context) {
        // Create a source from the input string
        size_t input_length = strlen(input);
        Source* source = braggi_source_from_string("repl_input", input, input_length);
        
        if (source) {
            // Execute the code (placeholder - real execution would be here)
            printf("Processing code: %s\n", input);
            printf("Source has %u lines\n", braggi_source_file_get_line_count(source));
            
            // In the future, replace with actual execution
            // if (braggi_execute(context, source)) {
            //     printf("Execution successful.\n");
            // }
            
            // Clean up
            braggi_source_file_destroy(source);
        } else {
            printf("Failed to create source from input.\n");
        }
    } else {
        printf("Failed to create Braggi context.\n");
    }
} 