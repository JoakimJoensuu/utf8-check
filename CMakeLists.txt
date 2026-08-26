cmake_minimum_required(VERSION 3.21)
project(utf8-check LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json"
     "${CMAKE_SOURCE_DIR}/compile_commands.json" SYMBOLIC COPY_ON_ERROR)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

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

option(UTF8C_SANITIZE "Build with ASan/UBSan" OFF)
if(UTF8C_SANITIZE)
  target_compile_options(utf8c_warnings INTERFACE -fsanitize=address,undefined
                                                  -fno-omit-frame-pointer)
  target_link_options(utf8c_warnings INTERFACE -fsanitize=address,undefined)
endif()

add_library(utf8c src/utf8c.c)
target_include_directories(utf8c PUBLIC include)
target_link_libraries(utf8c PRIVATE utf8c_warnings)

option(UTF8C_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(UTF8C_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})

if(UTF8C_EXAMPLES)
  add_subdirectory(examples)
endif()

if(UTF8C_TESTS)
  include(CTest)
  add_subdirectory(tests)
endif()

