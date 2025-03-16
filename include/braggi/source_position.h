/*
 * Braggi - Source Position
 * 
 * "Knowin' where ya are in the code is like knowin' where ya are on the ranch - 
 * essential for not gettin' lost in the wilderness!" - Texas Code Navigation
 */

#ifndef BRAGGI_SOURCE_POSITION_H
#define BRAGGI_SOURCE_POSITION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>  /* Added for bool type - can't rope cattle without a lasso! */

/* Define the component ID for source positions */
#define SOURCE_POSITION_COMPONENT 0

/*
 * Represents a position in source code
 */
typedef struct {
    uint32_t file_id;  // ID of the source file
    uint32_t line;     // Line number (1-based)
    uint32_t column;   // Column number (1-based)
    uint32_t offset;   // Byte offset from the start of the file
    uint32_t length;   // Length of the token or entity
} SourcePosition;

/*
 * Represents a source file with line mapping information.
 */
typedef struct SourceFile {
    char* filename;           // Name of the file
    char* content;            // Full content of the file
    size_t length;            // Length of the content
    uint32_t* line_map;       // Mapping from line numbers to offsets
    size_t line_count;        // Number of lines
    uint32_t file_id;         // Unique ID for this file
} SourceFile;

/*
 * Create a source position from line and column
 * 
 * "Makin' a position from scratch is like brandin' a new calf - 
 * ya gotta do it right the first time!" - Rancher's Guide to Code
 */
SourcePosition braggi_source_position_from_line_col(uint32_t line, uint32_t column);

/*
 * Create a full source position
 */
SourcePosition braggi_source_position_create(uint32_t file_id, uint32_t line, 
                                           uint32_t column, uint32_t offset, 
                                           uint32_t length);

/*
 * Compare two source positions
 * Returns:
 *   < 0 if a comes before b
 *   0 if a and b are equal
 *   > 0 if a comes after b
 */
int braggi_source_position_compare(const SourcePosition* a, const SourcePosition* b);

/*
 * Update the line and column information in a position based on the file content.
 * 
 * @param file The source file
 * @param position The position to update
 */
void braggi_source_position_get_line_col(const SourceFile* file, SourcePosition* position);

/*
 * Get a position from line and column information.
 * 
 * @param file The source file
 * @param line The line number (1-based)
 * @param column The column number (1-based)
 * @param position The position to fill
 * @return true if successful, false if the line/column are invalid
 */
bool braggi_source_file_get_position_from_line_col(const SourceFile* file, uint32_t line, uint32_t column, SourcePosition* position);

/*
 * Get a text snippet around a position, with line numbers and error pointer.
 * 
 * @param file The source file
 * @param position The position to get a snippet for
 * @param context_lines The number of lines to include before and after the position
 * @return A newly allocated string containing the snippet, or NULL on error
 */
char* braggi_source_position_get_snippet(const SourceFile* file, const SourcePosition* position, int context_lines);

#endif /* BRAGGI_SOURCE_POSITION_H */ 