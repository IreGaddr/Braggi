/*
 * Braggi - Source Code Handling
 * 
 * "Your code's source is as important as your family's - 
 * y'all better keep track of it properly!" - Texan wisdom
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "braggi/source.h"
#include "braggi/source_position.h"
#include "braggi/ecs.h" // Include the real ECS header

// Global ECS for source positions
static ECSWorld* global_ecs = NULL;

// Initialize the global ECS if not already initialized
static bool ensure_global_ecs(void) {
    if (global_ecs == NULL) {
        global_ecs = braggi_ecs_world_create(1024, 64); // Reasonable default sizes
        if (global_ecs == NULL) {
            return false;
        }
    }
    return true;
}

// Source component ID
static ComponentTypeID source_position_component_type = INVALID_COMPONENT_TYPE;

// Ensure the source position component type is registered
static bool ensure_source_position_component_type(void) {
    if (!ensure_global_ecs()) {
        return false;
    }
    
    if (source_position_component_type == INVALID_COMPONENT_TYPE) {
        source_position_component_type = braggi_ecs_register_component(global_ecs, sizeof(SourcePosition));
        if (source_position_component_type == INVALID_COMPONENT_TYPE) {
            return false;
        }
    }
    
    return true;
}

// Create a source position with the given file ID and location
SourcePosition* braggi_source_position_create_full(uint32_t file_id, uint32_t line, uint32_t column, uint32_t offset, uint32_t length) {
    SourcePosition* pos = (SourcePosition*)malloc(sizeof(SourcePosition));
    if (!pos) {
        return NULL;
    }
    
    // Initialize the position data
    pos->file_id = file_id;
    pos->line = line;
    pos->column = column;
    pos->offset = offset; 
    pos->length = length;
    
    return pos;
}

// Clone a source position
SourcePosition* braggi_source_position_clone(const SourcePosition* position) {
    if (!position) {
        return NULL;
    }
    
    return braggi_source_position_create_full(position->file_id, position->line, position->column, 
                                            position->offset, position->length);
}

// Destroy a source position
void braggi_source_position_destroy(SourcePosition* position) {
    if (position) {
        free(position);
    }
}

// Convert a source position to an entity ID
uint64_t braggi_source_position_to_entity(const Source* source, const SourcePosition* position) {
    if (!source || !position || position->file_id == 0) {
        return 0;
    }
    
    if (!ensure_global_ecs() || !ensure_source_position_component_type()) {
        return 0; // Error: Could not ensure ECS or component type
    }
    
    // Create a new entity
    EntityID entity = braggi_ecs_create_entity(global_ecs);
    if (entity == INVALID_ENTITY) {
        return 0; // Error creating entity
    }
    
    // Clone the position
    SourcePosition* pos_copy = braggi_source_position_clone(position);
    if (!pos_copy) {
        return 0; // Out of memory
    }
    
    // Add the component to the entity using the correct API
    braggi_ecs_add_component(global_ecs, entity, source_position_component_type, pos_copy);
    
    // Add to source tracking (if needed)
    // We need to cast away const to modify the source
    Source* mutable_source = (Source*)source;
    
    // Make sure we have space in the tracking array
    if (mutable_source->num_position_entities >= mutable_source->max_position_entities) {
        // Resize the array
        size_t new_max = mutable_source->max_position_entities == 0 ? 16 : mutable_source->max_position_entities * 2;
        uint64_t* new_array = (uint64_t*)realloc(mutable_source->position_entities, 
                                             sizeof(uint64_t) * new_max);
        if (!new_array) {
            // Failed to resize, but the entity is still valid in the ECS
            // so we'll return it anyway
            return entity;
        }
        
        mutable_source->position_entities = new_array;
        mutable_source->max_position_entities = new_max;
    }
    
    // Add the entity to the tracking array
    mutable_source->position_entities[mutable_source->num_position_entities++] = entity;
    
    return entity;
}

// Add a source position to an entity
bool braggi_ecs_add_source_position(const Source* source, uint64_t entity_id, const SourcePosition* position) {
    if (!source || entity_id == INVALID_ENTITY || !position || position->file_id == 0) {
        return false;
    }
    
    if (!ensure_global_ecs() || !ensure_source_position_component_type()) {
        return false;
    }
    
    // Clone the position
    SourcePosition* pos_copy = braggi_source_position_clone(position);
    if (!pos_copy) {
        return false; // Out of memory
    }
    
    // Add the component to the entity using the correct API
    braggi_ecs_add_component(global_ecs, entity_id, source_position_component_type, pos_copy);
    
    // Add to source tracking (if needed)
    // We need to cast away const to modify the source
    Source* mutable_source = (Source*)source;
    
    // Make sure we have space in the tracking array
    if (mutable_source->num_position_entities >= mutable_source->max_position_entities) {
        // Resize the array
        size_t new_max = mutable_source->max_position_entities == 0 ? 16 : mutable_source->max_position_entities * 2;
        uint64_t* new_array = (uint64_t*)realloc(mutable_source->position_entities, 
                                             sizeof(uint64_t) * new_max);
        if (!new_array) {
            // Failed to resize, but the component is still valid in the ECS
            // so we'll return success anyway
            return true;
        }
        
        mutable_source->position_entities = new_array;
        mutable_source->max_position_entities = new_max;
    }
    
    // Add the entity to the tracking array
    mutable_source->position_entities[mutable_source->num_position_entities++] = entity_id;
    
    return true;
}

// Get a source position from an entity
const SourcePosition* braggi_ecs_get_source_position(EntityID entity) {
    if (entity == INVALID_ENTITY || !ensure_global_ecs() || !ensure_source_position_component_type()) {
        return NULL;
    }
    
    // Check if the entity exists
    if (!braggi_ecs_entity_exists(global_ecs, entity)) {
        return NULL;
    }
    
    // Check if the entity has the component
    if (!braggi_ecs_has_component(global_ecs, entity, source_position_component_type)) {
        return NULL;
    }
    
    // Get the component data
    return (const SourcePosition*)braggi_ecs_get_component(global_ecs, entity, source_position_component_type);
}

// Helper function to count lines in a string
static unsigned int count_lines(const char* str) {
    unsigned int count = 0;
    const char* p = str;
    
    while (*p) {
        if (*p == '\n') {
            count++;
        }
        p++;
    }
    
    // Count the last line if it doesn't end with a newline
    if (p > str && *(p-1) != '\n') {
        count++;
    }
    
    return count;
}

// Helper function to split a string into lines
static char** split_lines(const char* str, unsigned int* num_lines) {
    *num_lines = count_lines(str);
    if (*num_lines == 0) {
        return NULL;
    }
    
    char** lines = (char**)malloc(sizeof(char*) * (*num_lines));
    if (!lines) {
        return NULL;
    }
    
    const char* start = str;
    const char* end;
    unsigned int line_index = 0;
    
    while (*start) {
        // Find the end of the current line
        end = strchr(start, '\n');
        if (!end) {
            // Last line without newline
            end = start + strlen(start);
        }
        
        // Allocate memory for the line
        size_t line_length = end - start;
        lines[line_index] = (char*)malloc(line_length + 1);
        if (!lines[line_index]) {
            // Clean up on allocation failure
            for (unsigned int i = 0; i < line_index; i++) {
                free(lines[i]);
            }
            free(lines);
            return NULL;
        }
        
        // Copy line contents
        strncpy(lines[line_index], start, line_length);
        lines[line_index][line_length] = '\0';
        
        // Move to the next line
        line_index++;
        start = (*end) ? end + 1 : end;
    }
    
    return lines;
}

// Global file ID counter for unique source file IDs
static uint32_t next_file_id = 1;

Source* braggi_source_file_create(const char* filename) {
    if (!filename) {
        return NULL;
    }
    
    // Open the file
    FILE* file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }
    
    // Determine file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return NULL;
    }
    
    // Read the entire file into a buffer
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);
    
    // Split the file into lines
    unsigned int num_lines;
    char** lines = split_lines(buffer, &num_lines);
    free(buffer);
    
    if (!lines) {
        return NULL;
    }
    
    // Create the Source structure
    Source* source = (Source*)malloc(sizeof(Source));
    if (!source) {
        for (unsigned int i = 0; i < num_lines; i++) {
            free(lines[i]);
        }
        free(lines);
        return NULL;
    }
    
    source->filename = strdup(filename);
    source->lines = lines;
    source->num_lines = num_lines;
    source->is_file = true;
    source->file_id = next_file_id++;
    
    // Initialize ECS integration data
    source->position_entities = NULL;
    source->num_position_entities = 0;
    source->max_position_entities = 0;
    
    return source;
}

Source* braggi_source_string_create(const char* source_str, const char* name) {
    if (!source_str) {
        return NULL;
    }
    
    // Split the string into lines
    unsigned int num_lines;
    char** lines = split_lines(source_str, &num_lines);
    
    if (!lines) {
        return NULL;
    }
    
    // Create the Source structure
    Source* source = (Source*)malloc(sizeof(Source));
    if (!source) {
        for (unsigned int i = 0; i < num_lines; i++) {
            free(lines[i]);
        }
        free(lines);
        return NULL;
    }
    
    source->filename = name ? strdup(name) : strdup("<string>");
    source->lines = lines;
    source->num_lines = num_lines;
    source->is_file = false;
    source->file_id = next_file_id++;
    
    // Initialize ECS integration data
    source->position_entities = NULL;
    source->num_position_entities = 0;
    source->max_position_entities = 0;
    
    return source;
}

void braggi_source_file_destroy(Source* source) {
    if (!source) {
        return;
    }
    
    // Free the filename
    if (source->filename) {
        free(source->filename);
    }
    
    // Free each line
    if (source->lines) {
        for (unsigned int i = 0; i < source->num_lines; i++) {
            free(source->lines[i]);
        }
        free(source->lines);
    }
    
    // Free the ECS integration data
    if (source->position_entities) {
        free(source->position_entities);
    }
    
    // Free the structure itself
    free(source);
}

const char* braggi_source_file_get_name(const Source* source) {
    if (!source) {
        return NULL;
    }
    return source->filename;
}

const char* braggi_source_file_get_line(const Source* source, unsigned int line) {
    if (!source || line == 0 || line > source->num_lines) {
        return NULL;
    }
    
    // Lines are 1-indexed externally, 0-indexed internally
    return source->lines[line - 1];
}

unsigned int braggi_source_file_get_line_count(const Source* source) {
    if (!source) {
        return 0;
    }
    return source->num_lines;
}

bool braggi_source_file_is_valid_position(const Source* source, unsigned int line, unsigned int column) {
    if (!source || line == 0 || line > source->num_lines) {
        return false;
    }
    
    // Lines are 1-indexed externally, 0-indexed internally
    const char* line_str = source->lines[line - 1];
    return column > 0 && column <= strlen(line_str) + 1;
}

// New ECS integration functions

SourcePosition braggi_source_get_position(const Source* source, 
                                          unsigned int line, 
                                          unsigned int column,
                                          unsigned int length) {
    SourcePosition pos;
    
    if (!source || !braggi_source_file_is_valid_position(source, line, column)) {
        // Create an invalid position
        pos.file_id = 0;
        pos.line = 0;
        pos.column = 0;
        pos.offset = 0;
        pos.length = 0;
        return pos;
    }
    
    // Calculate the offset
    unsigned int offset = 0;
    for (unsigned int i = 0; i < line - 1; i++) {
        offset += strlen(source->lines[i]) + 1; // +1 for newline
    }
    offset += column - 1; // -1 to convert to 0-indexed
    
    pos.file_id = source->file_id;
    pos.line = line;
    pos.column = column;
    pos.offset = offset;
    pos.length = length;
    
    return pos;
}

bool braggi_entity_to_source_position(const Source* source, uint64_t entity_id, SourcePosition* position) {
    if (!source || !position || entity_id == 0) {
        return false;
    }
    
    // Check if this entity is in our tracking array
    for (size_t i = 0; i < source->num_position_entities; i++) {
        if (source->position_entities[i] == entity_id) {
            // Get the source position component from the ECS
            // For now, we'll just stub this out
            // TODO: Implement the actual ECS component retrieval
            position->file_id = source->file_id;
            position->line = 1;  // Placeholder
            position->column = 1; // Placeholder
            position->offset = 0; // Placeholder
            position->length = 1; // Placeholder
            return true;
        }
    }
    
    return false;
}

/*
 * Adds content to a source file object.
 * 
 * "Fillin' a source file with content is like pourin' whiskey in a glass - 
 * don't spill a drop of that precious liquid!" - Braggi developer wisdom
 */
bool braggi_source_file_add_content(Source *source, const char *content, size_t length) {
    if (!source || !content) {
        return false;
    }
    
    // Free any existing lines
    if (source->lines) {
        for (unsigned int i = 0; i < source->num_lines; i++) {
            free(source->lines[i]);
        }
        free(source->lines);
    }
    
    // Make a copy of the content for processing
    char *content_copy = malloc(length + 1);
    if (!content_copy) {
        return false;
    }
    
    memcpy(content_copy, content, length);
    content_copy[length] = '\0';
    
    // Split the content into lines
    unsigned int num_lines;
    source->lines = split_lines(content_copy, &num_lines);
    free(content_copy); // split_lines makes its own copies
    
    if (!source->lines) {
        return false;
    }
    
    source->num_lines = num_lines;
    
    return true;
}

/*
 * Gets the size of a source file in bytes.
 * 
 * "Knowin' the size of your source is like knowin' the acreage of your ranch -
 * essential information for proper management!" - Quantum rancher proverb
 */
size_t braggi_source_file_get_size(const Source *source) {
    if (!source) {
        return 0;
    }
    // Calculate size by summing up line lengths and newlines
    size_t total_size = 0;
    for (unsigned int i = 0; i < source->num_lines; i++) {
        total_size += strlen(source->lines[i]) + 1; // +1 for newline
    }
    return total_size;
}

/*
 * Create a Source from an in-memory string
 *
 * "Like readin' from memory instead of paper - still gets the job done,
 * but it's already right there in yer hand!" - REPL philosopher
 */
Source* braggi_source_from_string(const char* name, const char* content, size_t length) {
    if (!name || !content) return NULL;
    
    // Make a null-terminated copy of the content
    char* content_copy = (char*)malloc(length + 1);
    if (!content_copy) return NULL;
    
    memcpy(content_copy, content, length);
    content_copy[length] = '\0';
    
    // Split the content into lines
    unsigned int num_lines;
    char** lines = split_lines(content_copy, &num_lines);
    free(content_copy);  // split_lines makes its own copies
    
    if (!lines) return NULL;
    
    // Create the Source structure
    Source* source = (Source*)malloc(sizeof(Source));
    if (!source) {
        // Clean up lines on failure
        for (unsigned int i = 0; i < num_lines; i++) {
            free(lines[i]);
        }
        free(lines);
        return NULL;
    }
    
    // Initialize the source fields
    source->filename = strdup(name);
    if (!source->filename) {
        for (unsigned int i = 0; i < num_lines; i++) {
            free(lines[i]);
        }
        free(lines);
        free(source);
        return NULL;
    }
    
    source->lines = lines;
    source->num_lines = num_lines;
    source->is_file = false;
    source->file_id = next_file_id++;
    
    // Initialize ECS integration data
    source->position_entities = NULL;
    source->num_position_entities = 0;
    source->max_position_entities = 0;
    
    return source;
} 