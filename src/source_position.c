/*
 * Braggi - Source Position Implementation
 * 
 * "Keepin' track of yer position in code is like knowin' where yer cattle are -
 * the whole operation depends on it!" - Texas Code Management
 */

#include "braggi/source_position.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * Create a source position from line and column
 * 
 * "Whippin' up a position from just line and column is like makin' biscuits with just flour and water -
 * gets the job done in a pinch, but it ain't fancy!" - Code Chef's Handbook
 */
SourcePosition braggi_source_position_from_line_col(uint32_t line, uint32_t column) {
    SourcePosition pos;
    pos.file_id = 0;      // Unknown file
    pos.line = line;
    pos.column = column;
    pos.offset = 0;       // Unknown offset
    pos.length = 0;       // Unknown length
    
    return pos;
}

/*
 * Create a full source position
 * 
 * "When ya need all the details, ya need 'em all - like a complete cattle report
 * with breed, age, weight, and temperament!" - Comprehensive Code Trackin'
 */
SourcePosition braggi_source_position_create(uint32_t file_id, uint32_t line, 
                                         uint32_t column, uint32_t offset, 
                                         uint32_t length) {
    SourcePosition pos;
    pos.file_id = file_id;
    pos.line = line;
    pos.column = column;
    pos.offset = offset;
    pos.length = length;
    
    return pos;
}

/*
 * Compare two source positions
 * 
 * "Comparin' positions is like comparin' two ranches - first by county,
 * then by road, then by mile marker!" - Logical Location Sortin'
 */
int braggi_source_position_compare(const SourcePosition* a, const SourcePosition* b) {
    if (!a || !b) {
        return a ? 1 : (b ? -1 : 0);
    }
    
    // Compare file IDs first
    if (a->file_id != b->file_id) {
        return a->file_id < b->file_id ? -1 : 1;
    }
    
    // Same file, compare lines
    if (a->line != b->line) {
        return a->line < b->line ? -1 : 1;
    }
    
    // Same line, compare columns
    if (a->column != b->column) {
        return a->column < b->column ? -1 : 1;
    }
    
    // Same column, compare offsets
    if (a->offset != b->offset) {
        return a->offset < b->offset ? -1 : 1;
    }
    
    // Same offset, compare lengths
    if (a->length != b->length) {
        return a->length < b->length ? -1 : 1;
    }
    
    // Identical positions
    return 0;
}

/*
 * Update the line and column information in a position based on the file content.
 * 
 * "Like findin' where a steer wandered off to by trackin' its hoofprints -
 * sometimes ya gotta look at the whole trail to know where it's at!" - Position Detective
 */
void braggi_source_position_get_line_col(const SourceFile* file, SourcePosition* position) {
    if (!file || !position || !file->content || !file->line_map) {
        return;
    }
    
    // If the offset is out of bounds, do nothing
    if (position->offset >= file->length) {
        return;
    }
    
    // Find the line by binary searching the line map
    size_t low = 0;
    size_t high = file->line_count - 1;
    size_t line = 0;
    
    while (low <= high) {
        size_t mid = low + (high - low) / 2;
        
        if (file->line_map[mid] <= position->offset) {
            line = mid;
            if (mid == file->line_count - 1 || file->line_map[mid + 1] > position->offset) {
                break;
            }
            low = mid + 1;
        } else {
            if (mid == 0) {
                line = 0;
                break;
            }
            high = mid - 1;
        }
    }
    
    // Calculate column (offset from line start)
    uint32_t column = position->offset - file->line_map[line] + 1;
    
    // Update the position
    position->line = line + 1;  // Convert to 1-based
    position->column = column;
}

/*
 * Get a position from line and column information.
 * 
 * "Convertatin' line and column to a real position is like findin' a ranch
 * when all ya got is the county and mile marker!" - Cowboy Cartographer
 */
bool braggi_source_file_get_position_from_line_col(const SourceFile* file, uint32_t line, 
                                                 uint32_t column, SourcePosition* position) {
    if (!file || !position || !file->content || !file->line_map) {
        return false;
    }
    
    // Check if the line is valid (line is 1-based externally)
    if (line == 0 || line > file->line_count) {
        return false;
    }
    
    // Get the line start offset
    uint32_t line_start = file->line_map[line - 1];
    
    // Get the line end
    uint32_t line_end;
    if (line < file->line_count) {
        line_end = file->line_map[line] - 1;  // -1 to exclude newline
    } else {
        line_end = file->length;
    }
    
    // Calculate line length
    uint32_t line_length = line_end - line_start;
    
    // Check if the column is valid (column is 1-based)
    if (column == 0 || column > line_length + 1) {
        return false;
    }
    
    // Calculate offset
    uint32_t offset = line_start + column - 1;
    
    // Update the position
    position->file_id = file->file_id;
    position->line = line;
    position->column = column;
    position->offset = offset;
    position->length = 1;  // Default to 1
    
    return true;
}

/*
 * Get a text snippet around a position, with line numbers and error pointer.
 * 
 * "A good error message is like a trail map with a big 'YOU ARE HERE' stamp -
 * shows exactly where ya went wrong and how to get back on track!" - Code Trail Guide
 */
char* braggi_source_position_get_snippet(const SourceFile* file, const SourcePosition* position, int context_lines) {
    if (!file || !position || !file->content) {
        return NULL;
    }
    
    // Make sure the position is valid
    if (position->line == 0 || position->line > file->line_count) {
        return NULL;
    }
    
    // Calculate the start and end lines to include
    uint32_t start_line = (position->line <= context_lines) ? 1 : position->line - context_lines;
    uint32_t end_line = (position->line + context_lines >= file->line_count) ? 
                        file->line_count : position->line + context_lines;
    
    // Calculate buffer size needed (estimate)
    // Each line: line number + separator + line content + newline + pointer line
    size_t buffer_size = 0;
    for (uint32_t i = start_line; i <= end_line; i++) {
        uint32_t line_start = file->line_map[i - 1];
        uint32_t line_end;
        
        if (i < file->line_count) {
            line_end = file->line_map[i] - 1;  // -1 to exclude newline
        } else {
            line_end = file->length;
        }
        
        // Line number + separator + content + newline
        buffer_size += 10 + 2 + (line_end - line_start) + 1;
        
        // Add space for pointer line if this is the error line
        if (i == position->line) {
            buffer_size += position->column + 10;  // Column + "^~~~~" and some extra
        }
    }
    
    // Allocate buffer
    char* buffer = (char*)malloc(buffer_size + 1);
    if (!buffer) {
        return NULL;
    }
    
    // Fill the buffer
    char* ptr = buffer;
    
    for (uint32_t i = start_line; i <= end_line; i++) {
        uint32_t line_start = file->line_map[i - 1];
        uint32_t line_end;
        
        if (i < file->line_count) {
            line_end = file->line_map[i] - 1;
        } else {
            line_end = file->length;
        }
        
        // Add line number
        ptr += sprintf(ptr, "%4d | ", i);
        
        // Add line content
        uint32_t line_length = line_end - line_start;
        memcpy(ptr, file->content + line_start, line_length);
        ptr += line_length;
        *ptr++ = '\n';
        
        // Add pointer line if this is the error line
        if (i == position->line) {
            ptr += sprintf(ptr, "     | ");
            for (uint32_t j = 1; j < position->column; j++) {
                *ptr++ = ' ';
            }
            
            // Add markers for the error
            *ptr++ = '^';
            for (uint32_t j = 1; j < position->length && j < 10; j++) {
                *ptr++ = '~';
            }
            *ptr++ = '\n';
        }
    }
    
    // Null-terminate the buffer
    *ptr = '\0';
    
    return buffer;
} 