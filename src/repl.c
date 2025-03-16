/*
 * Braggi - Simple REPL (Read-Eval-Print Loop)
 * 
 * "A good REPL is like a good conversation at an Irish pub - 
 * it responds promptly and doesn't judge ya too harshly!" - Texirish wisdom
 */

#include "braggi/braggi.h"
#include "braggi/source.h"
#include "braggi/braggi_context.h"
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
    "===================================================\n"
    "  Braggi Language REPL v0.1.0\n"
    "  'Cause every good language deserves a good REPL!\n"
    "  Type 'help' for commands, 'exit' to quit\n"
    "===================================================\n";

// Forward declarations
static char* get_input(const char* prompt);
static void process_command(const char* input, bool* should_exit);
static void print_help(void);
static void handle_special_command(const char* cmd);

int main(void) {
    char* input;
    bool should_exit = false;
    
    // Display welcome message
    printf("%s\n", WELCOME_MESSAGE);
    
    // Main REPL loop
    while (!should_exit) {
        // Get user input
        input = get_input("braggi> ");
        
        // Check for EOF
        if (!input) {
            printf("Y'all come back now, ya hear?\n");
            break;
        }
        
        // Process the command
        process_command(input, &should_exit);
        
        // Clean up
        free(input);
    }
    
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
    printf("%s", prompt);
    fflush(stdout);
    
    if (fgets(buffer, MAX_INPUT_LENGTH, stdin) == NULL) {
        return NULL;
    }
    
    // Remove trailing newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    
    return strdup(buffer);
#endif
}

// Print help information
static void print_help(void) {
    printf("\nAvailable commands:\n");
    printf("  help, ?            Show this help message\n");
    printf("  :source <filename> Load a source file (not implemented yet)\n");
    printf("  exit, quit, q      Exit the REPL\n");
    printf("\nAny other input will be processed as Braggi code.\n");
    printf("(Actual interpretation not implemented yet)\n\n");
}

// Handle special commands starting with ':'
static void handle_special_command(const char* cmd) {
    if (strncmp(cmd, "source ", 7) == 0) {
        const char* filename = cmd + 7;
        printf("Would load source file: %s (not implemented yet)\n", filename);
    } else {
        printf("Unknown command: %s\n", cmd);
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
        printf("Y'all come back now, ya hear?\n");
        return;
    }
    
    // Check for help command
    if (strcmp(input, "help") == 0 || strcmp(input, "?") == 0) {
        print_help();
        return;
    }
    
    // Check for special commands
    if (input[0] == ':') {
        handle_special_command(input + 1);
        return;
    }
    
    // Process as Braggi code
    BraggiContext* context = braggi_context_create();
    if (context) {
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
        
        braggi_context_destroy(context);
    } else {
        printf("Failed to create Braggi context.\n");
    }
} 