# FindJansson.cmake
# "Huntin' down Jansson like a bloodhound on a Texas trail - it won't stay hidden for long!" 🤠
#
# Find Jansson JSON library
# Defines:
#  JANSSON_FOUND - System has Jansson
#  JANSSON_INCLUDE_DIRS - The Jansson include directories
#  JANSSON_LIBRARIES - The libraries needed to use Jansson
#  JANSSON_VERSION - Version of Jansson found

# Find include directory
find_path(JANSSON_INCLUDE_DIR
  NAMES jansson.h
  PATHS
    /usr/include
    /usr/local/include
    /opt/local/include
  DOC "Jansson include directory"
)

# Find library
find_library(JANSSON_LIBRARY
  NAMES jansson
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
  DOC "Jansson library"
)

# Handle version detection if jansson_config.h exists
if(JANSSON_INCLUDE_DIR AND EXISTS "${JANSSON_INCLUDE_DIR}/jansson_config.h")
  file(STRINGS "${JANSSON_INCLUDE_DIR}/jansson_config.h" JANSSON_VERSION_LINE REGEX "^#define[ \t]+JANSSON_VERSION[ \t]+\"[^\"]+\"")
  string(REGEX REPLACE "^#define[ \t]+JANSSON_VERSION[ \t]+\"([^\"]+)\".*" "\\1" JANSSON_VERSION "${JANSSON_VERSION_LINE}")
endif()

# Set output variables
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Jansson
  REQUIRED_VARS JANSSON_LIBRARY JANSSON_INCLUDE_DIR
  VERSION_VAR JANSSON_VERSION
)

if(JANSSON_FOUND)
  set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
  set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
endif()

# Hide internal variables
mark_as_advanced(
  JANSSON_INCLUDE_DIR
  JANSSON_LIBRARY
)

# "Found ya, you sneaky JSON critter! 🤠" 