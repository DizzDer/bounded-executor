# Contributing

Use C++17, keep the runtime dependency-free, and add regression coverage for behavioral changes.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Also test Release: checks must not disappear with `NDEBUG`. Concurrency tests should use promises or barriers rather than timing assumptions. Do not describe a feature as production-proven without evidence. Include the compiler, platform, commands and limitations when reporting a benchmark or bug.

Submit a pull request explaining the concrete behavior changed, compatibility impact and validation performed. Keep formatting changes separate from behavioral changes.
