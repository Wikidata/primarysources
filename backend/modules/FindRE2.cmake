# Find Google RE2

find_path(RE2_INCLUDE_PATH NAMES re2/re2.h)
find_library(RE2_LIBRARY NAMES re2)

if(RE2_INCLUDE_PATH AND RE2_LIBRARY)
    set(RE2_FOUND TRUE)
endif(RE2_INCLUDE_PATH AND RE2_LIBRARY)

if(RE2_FOUND)
    if(NOT RE2_FIND_QUIETLY)
        message(STATUS "Found RE2: ${RE2_LIBRARY}; includes - ${RE2_INCLUDE_PATH}")
    endif(NOT RE2_FIND_QUIETLY)
else(RE2_FOUND)
    if(RE2_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find RE2 library.")
    endif(RE2_FIND_REQUIRED)
endif(RE2_FOUND)