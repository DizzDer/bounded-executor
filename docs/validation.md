# Validation record

Date: 2026-09-28. Local environment: Windows x64, GCC 16.2.0 (w64devkit), CMake and Ninja.

Both Debug and Release builds completed with warnings treated as errors. CTest passed all registered cases, including the runnable example. Test checks use explicit exceptions rather than `assert`, so they remain active under `NDEBUG`.

```sh
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel 2
ctest --test-dir build-debug --output-on-failure
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel 2
ctest --test-dir build-release --output-on-failure
```

For current cross-platform and sanitizer results, inspect the linked GitHub Actions run rather than inferring success from the presence of a workflow file. Sanitizers are configured on Linux; they were not run by the local Windows compiler. Stress tests are regression coverage, not proof of race-freedom or production performance. No performance or production-deployment claims are made.
