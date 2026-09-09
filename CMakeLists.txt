cmake_minimum_required(VERSION 3.16)
project(nesemu C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(NES_ASAN "Enable AddressSanitizer/UndefinedBehaviorSanitizer" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(SDL2 REQUIRED)

add_executable(nesemu
    src/main.c
)

target_include_directories(nesemu PRIVATE
    include
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(nesemu PRIVATE ${SDL2_LIBRARIES})

target_compile_options(nesemu PRIVATE
    -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes
    -Wpointer-arith -Wcast-align -fno-strict-aliasing
)

if(NES_ASAN)
    target_compile_options(nesemu PRIVATE -O0 -g3 -fsanitize=address,undefined)
    target_link_options(nesemu PRIVATE -fsanitize=address,undefined)
else()
    target_compile_options(nesemu PRIVATE -O2 -g)
endif()
