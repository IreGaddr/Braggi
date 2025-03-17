/*
 * Braggi - Language Server Protocol Interface
 * 
 * "Keepin' your IDE and compiler talkin' like old friends at a Texas barbecue!" 
 * - LSP Communication Specialist
 */

#ifndef BRAGGI_LSP_INTERFACE_H
#define BRAGGI_LSP_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Compile a source file and return diagnostics in JSON format
 * 
 * @param source_text The source code text to compile
 * @param file_path   The path/name of the source file
 * @return            A JSON string containing diagnostics (must be freed with braggi_lsp_free_string)
 */
const char* braggi_lsp_compile_and_get_diagnostics(const char* source_text, const char* file_path);

/*
 * Get completion items at a specific position in JSON format
 * 
 * @param source_text The source code text
 * @param file_path   The path/name of the source file
 * @param line        The line number (0-based, LSP convention)
 * @param character   The character position (0-based, LSP convention)
 * @return            A JSON string containing completion items (must be freed with braggi_lsp_free_string)
 */
const char* braggi_lsp_get_completions(const char* source_text, const char* file_path, int line, int character);

/*
 * Get hover information at a specific position in JSON format
 * 
 * @param source_text The source code text
 * @param file_path   The path/name of the source file
 * @param line        The line number (0-based, LSP convention)
 * @param character   The character position (0-based, LSP convention)
 * @return            A JSON string containing hover information (must be freed with braggi_lsp_free_string)
 */
const char* braggi_lsp_get_hover_info(const char* source_text, const char* file_path, int line, int character);

/*
 * Free a string returned by the LSP interface
 * 
 * @param str The string to free
 */
void braggi_lsp_free_string(const char* str);

#ifdef __cplusplus
}
#endif

#endif /* BRAGGI_LSP_INTERFACE_H */ 