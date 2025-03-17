/*
 * Braggi - Interactive REPL Tool
 * 
 * "Talkin' to a computer is like talkin' to a stubborn mule - 
 * ya gotta know the right commands or it'll just stand there!" - Texas Coding Wisdom
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "braggi/braggi.h"
#include "braggi/error.h"
#include "braggi/source.h"
#include "braggi/stdlib.h"

// ANSI color codes for pretty output
#define COLOR_RESET    "\x1b[0m"
#define COLOR_RED      "\x1b[31m"
#define COLOR_GREEN    "\x1b[32m"
#define COLOR_YELLOW   "\x1b[33m"
#define COLOR_BLUE     "\x1b[34m"
#define COLOR_MAGENTA  "\x1b[35m"
#define COLOR_CYAN     "\x1b[36m"

// REPL state structure
typedef struct {
    BraggiContext* context;
    bool running;
    char* last_input;
} ReplState;

// Forward declarations
void print_welcome(void);
void print_help(void);
ReplState* create_repl_state(void);
void destroy_repl_state(ReplState* state);
bool process_command(ReplState* state, const char* input);
bool execute_code(ReplState* state, const char* code);
bool execute_command(ReplState* state, const char* input);

int main(int argc, char** argv) {
    print_welcome();
    
    // Create REPL state
    ReplState* state = create_repl_state();
    if (!state) {
        fprintf(stderr, COLOR_RED "Failed to initialize REPL state\n" COLOR_RESET);
        return 1;
    }
    
    // Main REPL loop
    char* input;
    while (state->running && (input = readline(COLOR_CYAN "braggi> " COLOR_RESET))) {
        // Add to history if non-empty
        if (input[0] != '\0') {
            add_history(input);
        }
        
        // Process input
        process_command(state, input);
        
        // Free input and continue
        free(input);
    }
    
    printf("\nGoodbye!\n");
    destroy_repl_state(state);
    return 0;
}

void print_welcome(void) {
    printf(COLOR_MAGENTA);
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                   Welcome to Braggi REPL                      ║\n");
    printf("║                                                               ║\n");
    printf("║   Type your code directly, or use one of these commands:      ║\n");
    printf("║   :help   - Display help information                          ║\n");
    printf("║   :load   - Load code from a file                             ║\n");
    printf("║   :import - Import a standard library module                  ║\n");
    printf("║   :quit   - Exit the REPL                                     ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
}

void print_help(void) {
    printf(COLOR_CYAN);
    printf("Available commands:\n");
    printf("  :help              Show this help message\n");
    printf("  :load <filename>   Load and execute a Braggi source file\n");
    printf("  :import <module>   Import a standard library module\n");
    printf("  :clear             Clear the screen\n");
    printf("  :quit              Exit the REPL\n");
    printf("\n");
    printf("Any other input will be evaluated as Braggi code.\n");
    printf(COLOR_RESET);
}

ReplState* create_repl_state(void) {
    ReplState* state = (ReplState*)malloc(sizeof(ReplState));
    if (!state) return NULL;
    
    state->context = braggi_context_create();
    if (!state->context) {
        free(state);
        return NULL;
    }
    
    state->running = true;
    state->last_input = NULL;
    
    return state;
}

void destroy_repl_state(ReplState* state) {
    if (!state) return;
    
    if (state->context) {
        braggi_context_destroy(state->context);
    }
    
    if (state->last_input) {
        free(state->last_input);
    }
    
    free(state);
}

bool process_command(ReplState* state, const char* input) {
    if (!input || !state) return false;
    
    // Skip whitespace
    while (*input && isspace(*input)) input++;
    
    // Empty input
    if (*input == '\0') return true;
    
    // Check if it's a command (starts with ':')
    if (*input == ':') {
        return execute_command(state, input + 1);
    }
    
    // Otherwise, treat as code
    return execute_code(state, input);
}

bool execute_command(ReplState* state, const char* cmd) {
    // Skip whitespace
    while (*cmd && isspace(*cmd)) cmd++;
    
    // Command: help
    if (strncmp(cmd, "help", 4) == 0) {
        print_help();
        return true;
    }
    
    // Command: quit/exit
    if (strncmp(cmd, "quit", 4) == 0 || strncmp(cmd, "exit", 4) == 0) {
        state->running = false;
        return true;
    }
    
    // Command: clear
    if (strncmp(cmd, "clear", 5) == 0) {
        printf("\033[2J\033[H");  // ANSI escape sequence to clear screen
        print_welcome();
        return true;
    }
    
    // Command: import <module>
    if (strncmp(cmd, "import ", 7) == 0) {
        const char* module_name = cmd + 7;
        printf(COLOR_YELLOW "Importing module: %s\n" COLOR_RESET, module_name);
        
        if (braggi_stdlib_load_module(state->context, module_name)) {
            printf(COLOR_GREEN "Successfully imported module '%s'\n" COLOR_RESET, module_name);
        } else {
            printf(COLOR_RED "Failed to import module '%s'\n" COLOR_RESET, module_name);
        }
        return true;
    }
    
    // Command: load <filename>
    if (strncmp(cmd, "load", 4) == 0) {
        cmd += 4;
        // Skip whitespace to get to the filename
        while (*cmd && isspace(*cmd)) cmd++;
        
        if (*cmd == '\0') {
            fprintf(stderr, COLOR_RED "Error: No filename provided\n" COLOR_RESET);
            return false;
        }
        
        char filename[256];
        strncpy(filename, cmd, 255);
        filename[255] = '\0';
        
        printf(COLOR_BLUE "Loading file: %s\n" COLOR_RESET, filename);
        
        // Read file content
        FILE* file = fopen(filename, "r");
        if (!file) {
            fprintf(stderr, COLOR_RED "Error: Could not open file %s\n" COLOR_RESET, filename);
            return false;
        }
        
        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        // Allocate buffer
        char* buffer = (char*)malloc(file_size + 1);
        if (!buffer) {
            fprintf(stderr, COLOR_RED "Error: Out of memory\n" COLOR_RESET);
            fclose(file);
            return false;
        }
        
        // Read file content
        size_t read_size = fread(buffer, 1, file_size, file);
        buffer[read_size] = '\0';
        fclose(file);
        
        // Process the file content
        Source* source = braggi_source_string_create(buffer, filename);
        if (!source) {
            fprintf(stderr, COLOR_RED "Error: Failed to create source\n" COLOR_RESET);
            free(buffer);
            return false;
        }
        
        // Try to process the code
        bool result = execute_code(state, buffer);
        
        // Clean up
        braggi_source_file_destroy(source);
        free(buffer);
        
        return result;
    }
    
    fprintf(stderr, COLOR_RED "Unknown command: :%s\n" COLOR_RESET, cmd);
    return false;
}

bool execute_code(ReplState* state, const char* code) {
    if (!code || !state || !state->context) return false;
    
    printf(COLOR_BLUE "Executing code...\n" COLOR_RESET);
    
    // Create a source from the string
    Source* source = braggi_source_string_create(code, "<repl>");
    if (!source) {
        fprintf(stderr, COLOR_RED "Error: Failed to create source\n" COLOR_RESET);
        return false;
    }
    
    // Process the code using the source
    bool success = braggi_context_process_source(state->context, source);
    
    // Clean up
    braggi_source_file_destroy(source);
    
    if (!success) {
        fprintf(stderr, COLOR_RED "Error: Failed to process code\n" COLOR_RESET);
        
        // Get and print the error if available
        const Error* error = braggi_context_get_last_error(state->context);
        if (error) {
            char* error_str = braggi_error_to_string(error);
            if (error_str) {
                fprintf(stderr, COLOR_RED "Error: %s\n" COLOR_RESET, error_str);
                free(error_str);
            }
        }
        
        return false;
    }
    
    printf(COLOR_GREEN "Code executed successfully!\n" COLOR_RESET);
    return true;
} 