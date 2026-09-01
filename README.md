# UTF-8 check

Only checks well-formed UTF-8 (RFC 3629 §3). Incremental: a code point may
split across feeds. API in [include/utf8c.h](include/utf8c.h).

Freestanding library.

## Build

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

See [CMakeLists.txt](CMakeLists.txt) and [tests/CMakeLists.txt](tests/CMakeLists.txt) for
options and dependencies.

## Style

`--experimental-custom-checks` is required for `CustomChecks` in `.clang-tidy`.

```sh
clang-format-23 --dry-run --Werror $(find include src examples tests -type f -name '*.[ch]' | sort)
clang-tidy-23 --experimental-custom-checks $(jq -r '.[].file' compile_commands.json)
```
