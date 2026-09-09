# FindSDL2.cmake
#
# Locates the SDL2 library and headers, preferring pkg-config when
# available and falling back to a manual find_path/find_library search.
#
# Defines:
#   SDL2_FOUND
#   SDL2_INCLUDE_DIRS
#   SDL2_LIBRARIES

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_SDL2 QUIET sdl2)
endif()

find_path(SDL2_INCLUDE_DIR
    NAMES SDL.h
    HINTS ${PC_SDL2_INCLUDE_DIRS}
    PATH_SUFFIXES SDL2
)

find_library(SDL2_LIBRARY
    NAMES SDL2
    HINTS ${PC_SDL2_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2
    REQUIRED_VARS SDL2_LIBRARY SDL2_INCLUDE_DIR
)

if(SDL2_FOUND)
    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
    set(SDL2_LIBRARIES ${SDL2_LIBRARY})
endif()

mark_as_advanced(SDL2_INCLUDE_DIR SDL2_LIBRARY)
