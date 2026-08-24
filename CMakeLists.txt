cmake_minimum_required(VERSION 3.21)
project(utf8-check LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json"
     "${CMAKE_SOURCE_DIR}/compile_commands.json" SYMBOLIC COPY_ON_ERROR)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

add_library(utf8 src/utf8.c)
target_include_directories(utf8 PUBLIC include)

add_executable(check examples/check.c)
target_link_libraries(check PRIVATE utf8)

enable_testing()
add_executable(test_utf8 tests/utf8.c)
target_link_libraries(test_utf8 PRIVATE utf8)
add_test(NAME utf8 COMMAND test_utf8)
