# UTF-8 check

Only checks well-formed UTF-8 (RFC 3629 §3). Incremental: a code point may
split across feeds.

## Build

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

CMake fetches [cgreen](https://github.com/cgreen-devs/cgreen) when tests are on.
Examples and tests are off when this project is not the CMake top level.

## Style

`--experimental-custom-checks` is required so `CustomChecks` in `.clang-tidy` run.

```sh
clang-format-23 --dry-run --Werror include/*.h src/*.c examples/*.c tests/*.c
clang-tidy-23 --experimental-custom-checks $(jq -r '.[].file' compile_commands.json)
```

Apache-2.0. See `LICENSE`.
