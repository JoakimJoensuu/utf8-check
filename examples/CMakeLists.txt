add_executable(check check.c)
target_link_libraries(check PRIVATE utf8 utf8_warnings)
