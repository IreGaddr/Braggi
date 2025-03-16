/*
 * Braggi - Source Code Management
 * 
 * "Trackin' source code is like trackin' cattle - ya gotta know 
 * where they've been and where they're goin'!" - Texas Code Rancher
 */

#ifndef BRAGGI_SOURCE_H
#define BRAGGI_SOURCE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Include the source position definitions
#include "source_position.h"

/**
 * Source file structure
 * Contains information about a source file and its contents
 */
typedef struct Source {
    char* filename;        // Name/path of the source file
    char** lines;          // Array of lines in the file
    unsigned int num_lines; // Number of lines
    bool is_file;          // Whether this source is from a file or a string
    uint32_t file_id;      // Unique ID for this file in the source position system
    
    // Map of entity IDs to source positions for ECS integration
    uint64_t* position_entities; // Array of entity IDs
    size_t num_position_entities; // Number of entity IDs
    size_t max_position_entities; // Capacity of the entity ID array
} Source;

/**
 * Create a source file from a file path.
 * The file will be read and parsed into lines for easy access.
 * 
 * @param filename The path to the file
 * @return A new Source object, or NULL on error
 */
Source* braggi_source_file_create(const char* filename);

/**
 * Create a source file from a string.
 * The string will be parsed into lines for easy access.
 * 
 * @param source_str The source code string
 * @param name Optional name for the source (may be NULL)
 * @return A new Source object, or NULL on error
 */
Source* braggi_source_string_create(const char* source_str, const char* name);

/**
 * Destroy a source file and free its resources.
 * 
 * @param source The source file to destroy
 */
void braggi_source_file_destroy(Source* source);

/**
 * Get the name of a source file.
 * 
 * @param source The source file
 * @return The name of the source file
 */
const char* braggi_source_file_get_name(const Source* source);

/**
 * Get a specific line from a source file.
 * Lines are 1-indexed (first line is 1, not 0).
 * 
 * @param source The source file
 * @param line The line number (1-based)
 * @return The line text, or NULL if out of range
 */
const char* braggi_source_file_get_line(const Source* source, unsigned int line);

/**
 * Get the number of lines in a source file.
 * 
 * @param source The source file
 * @return The number of lines
 */
unsigned int braggi_source_file_get_line_count(const Source* source);

/**
 * Check if a position is valid in a source file.
 * 
 * @param source The source file
 * @param line The line number (1-based)
 * @param column The column number (1-based)
 * @return true if the position is valid, false otherwise
 */
bool braggi_source_file_is_valid_position(const Source* source, unsigned int line, unsigned int column);

/**
 * Get a source position from line and column information.
 * 
 * @param source The source file
 * @param line The line number (1-based)
 * @param column The column number (1-based)
 * @param length The length of the token/region
 * @return A SourcePosition structure
 */
SourcePosition braggi_source_get_position(const Source* source, 
                                         unsigned int line, 
                                         unsigned int column,
                                         unsigned int length);

/**
 * Convert a source position to an entity in the ECS.
 * 
 * @param source The source file
 * @param position The source position
 * @return An entity ID, or 0 on error
 */
uint64_t braggi_source_position_to_entity(const Source* source, const SourcePosition* position);

/**
 * Get source position information from an entity.
 * 
 * @param source The source file
 * @param entity_id The entity ID
 * @param position Output parameter to store the position
 * @return true if successful, false otherwise
 */
bool braggi_entity_to_source_position(const Source* source, uint64_t entity_id, SourcePosition* position);

/**
 * Add source position information to an entity.
 * 
 * @param source The source file
 * @param entity_id The entity ID
 * @param position The source position
 * @return true if successful, false otherwise
 */
bool braggi_ecs_add_source_position(const Source* source, uint64_t entity_id, const SourcePosition* position);

/**
 * Adds content to a source file object.
 * This replaces any existing content.
 * 
 * @param source The source file
 * @param content The content to add
 * @param length The length of the content
 * @return true if successful, false otherwise
 */
bool braggi_source_file_add_content(Source* source, const char* content, size_t length);

/**
 * Gets the size of a source file in bytes.
 * 
 * @param source The source file
 * @return The size in bytes
 */
size_t braggi_source_file_get_size(const Source* source);

/**
 * Create a new Source from a filename
 */
Source* braggi_source_file_create(const char* filename);

/**
 * Create a new Source from a string in memory
 * 
 * @param name A name to associate with this source (e.g. "repl")
 * @param content The string content
 * @param length The length of the content
 * @return A new Source or NULL on failure
 */
Source* braggi_source_from_string(const char* name, const char* content, size_t length);

/**
 * Get the filename of a source
 */
const char* braggi_source_get_filename(const Source* source);

#endif /* BRAGGI_SOURCE_H */ 