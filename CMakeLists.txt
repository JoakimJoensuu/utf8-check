cmake_minimum_required(VERSION 3.21)
project(utf8-check LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json"
     "${CMAKE_SOURCE_DIR}/compile_commands.json" SYMBOLIC COPY_ON_ERROR)

add_library(utf8c_warnings INTERFACE)
target_compile_options(
  utf8c_warnings
  INTERFACE -Wall
            -Wextra
            -Wpedantic
            -Werror
            -Wconversion
            -Wformat=2
            -Wmissing-prototypes
            -Wshadow
            -Wstrict-prototypes)
add_library(utf8c_sanitize INTERFACE)
if(PROJECT_IS_TOP_LEVEL AND (CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU"))
  target_compile_options(utf8c_sanitize INTERFACE -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(utf8c_sanitize INTERFACE -fsanitize=address,undefined)
endif()

add_library(utf8c src/utf8c.c)
target_include_directories(utf8c PUBLIC include)
target_compile_options(utf8c PRIVATE -ffreestanding)
target_link_libraries(utf8c PRIVATE utf8c_warnings utf8c_sanitize)

option(UTF8C_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(UTF8C_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})

if(UTF8C_EXAMPLES)
  add_subdirectory(examples)
endif()

if(UTF8C_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
