include(FetchContent)
find_package(Threads REQUIRED)

# Their CMake always compiles C++. Build the C library only.
FetchContent_Declare(
  cgreen
  GIT_REPOSITORY https://github.com/cgreen-devs/cgreen.git
  GIT_TAG 1.6.3
  GIT_SHALLOW TRUE
  SOURCE_SUBDIR skip-cgreen-cmake)
FetchContent_MakeAvailable(cgreen)

file(WRITE "${cgreen_SOURCE_DIR}/gitrevision.h" "#define GITREVISION \"1.6.3\"\n")
file(WRITE "${cgreen_BINARY_DIR}/config.h" "\n")

set(cgreen_src_dir "${cgreen_SOURCE_DIR}/src")
add_library(
  cgreen_static STATIC
  ${cgreen_src_dir}/assertions.c
  ${cgreen_src_dir}/boxed_double.c
  ${cgreen_src_dir}/breadcrumb.c
  ${cgreen_src_dir}/cgreen_time.c
  ${cgreen_src_dir}/cgreen_value.c
  ${cgreen_src_dir}/constraint.c
  ${cgreen_src_dir}/cute_reporter.c
  ${cgreen_src_dir}/cdash_reporter.c
  ${cgreen_src_dir}/messaging.c
  ${cgreen_src_dir}/message_formatting.c
  ${cgreen_src_dir}/mocks.c
  ${cgreen_src_dir}/parameters.c
  ${cgreen_src_dir}/reporter.c
  ${cgreen_src_dir}/runner.c
  ${cgreen_src_dir}/string_comparison.c
  ${cgreen_src_dir}/suite.c
  ${cgreen_src_dir}/text_reporter.c
  ${cgreen_src_dir}/utils.c
  ${cgreen_src_dir}/vector.c
  ${cgreen_src_dir}/posix_cgreen_pipe.c
  ${cgreen_src_dir}/posix_cgreen_time.c
  ${cgreen_src_dir}/posix_runner_platform.c)
set_target_properties(cgreen_static PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
target_compile_definitions(cgreen_static PRIVATE "VERSION=\"1.6.3\"")
target_include_directories(
  cgreen_static
  SYSTEM PUBLIC ${cgreen_SOURCE_DIR}/include
  PRIVATE ${cgreen_SOURCE_DIR} ${cgreen_src_dir} ${cgreen_BINARY_DIR})
target_link_libraries(cgreen_static PUBLIC Threads::Threads m)

add_executable(test_utf8 utf8.c)
target_link_libraries(test_utf8 PRIVATE utf8c utf8_warnings cgreen_static)
add_test(NAME utf8 COMMAND test_utf8)
