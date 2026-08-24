# UTF-8 check

Incremental well-formed UTF-8 (RFC 3629) in C.

## Build

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

## Style

`--experimental-custom-checks` is required so `CustomChecks` in `.clang-tidy` run.

```sh
clang-format-23 --dry-run --Werror include/*.h src/*.[ch] examples/*.c
clang-tidy-23 --experimental-custom-checks $(jq -r '.[].file' compile_commands.json)
```

Apache-2.0. See `LICENSE`.
