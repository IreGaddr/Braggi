# FindReadline.cmake
# 
# "Findin' readline is like findin' a good Irish pub in Texas - 
# ya gotta know where to look!" - Texirish CMake Philosophy
#
# Finds the GNU Readline library
# 
# This module defines:
#  Readline_FOUND - True if readline is found
#  Readline_INCLUDE_DIR - The include directory
#  Readline_LIBRARY - The readline library
#  Readline_VERSION - The version string (if available)

# Include some standard helper functions
include(FindPackageHandleStandardArgs)

# Find the include directory
find_path(Readline_INCLUDE_DIR
  NAMES readline/readline.h
  PATHS
    /usr/include
    /usr/local/include
    /opt/local/include
    /mingw/include
    /mingw64/include
    /opt/csw/include
    /opt/include
    /sw/include
  DOC "The readline include directory"
)

# Find the readline library
find_library(Readline_LIBRARY
  NAMES readline
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    /mingw/lib
    /mingw64/lib
    /opt/csw/lib
    /opt/lib
    /sw/lib
    /usr/lib/x86_64-linux-gnu  # Common location on Ubuntu/Debian
  DOC "The readline library"
)

# Try to find the version
if(Readline_INCLUDE_DIR AND EXISTS "${Readline_INCLUDE_DIR}/readline/readline.h")
  file(STRINGS "${Readline_INCLUDE_DIR}/readline/readline.h" Readline_H REGEX "^#define RL_VERSION_MAJOR[ \t]+[0-9]+$")
  if(Readline_H)
    string(REGEX REPLACE "^#define RL_VERSION_MAJOR[ \t]+([0-9]+)$" "\\1" Readline_VERSION_MAJOR "${Readline_H}")
    string(REGEX REPLACE "^#define RL_VERSION_MINOR[ \t]+([0-9]+)$" "\\1" Readline_VERSION_MINOR "${Readline_H}")
    set(Readline_VERSION "${Readline_VERSION_MAJOR}.${Readline_VERSION_MINOR}")
  else()
    # Older versions of readline don't have the version macros
    set(Readline_VERSION "unknown")
  endif()
endif()

# Handle the find_package() arguments and set Readline_FOUND
find_package_handle_standard_args(Readline
  REQUIRED_VARS Readline_LIBRARY Readline_INCLUDE_DIR
  VERSION_VAR Readline_VERSION
)

# Mark these variables as advanced in the CMake GUI
mark_as_advanced(Readline_INCLUDE_DIR Readline_LIBRARY)

# If found, add a message with installation instructions for platforms that might not have it
if(NOT Readline_FOUND)
  message(STATUS "Readline library not found. To install:")
  message(STATUS "  On Debian/Ubuntu: sudo apt-get install libreadline-dev")
  message(STATUS "  On Fedora/RHEL/CentOS: sudo dnf install readline-devel")
  message(STATUS "  On macOS with Homebrew: brew install readline")
endif() 