# Bounded Executor

[![CI](https://github.com/DizzDer/bounded-executor/actions/workflows/ci.yml/badge.svg)](https://github.com/DizzDer/bounded-executor/actions/workflows/ci.yml)

A dependency-free C++17 thread pool with explicit backpressure, future-based results and drain-on-close shutdown. Designed for services that must bound pending work instead of accumulating an unbounded task queue.

```cpp
#include "bounded_executor.hpp"
concurrency::bounded_executor pool(4, 64);
auto result = pool.try_submit([] { return 6 * 7; });
if (result) {
    const auto answer = result->get(); // 42; task exceptions propagate here
}
pool.close(); // reject new work; drain previously admitted work
```

## Build and run

Requires CMake 3.20+, a C++17 compiler, and platform thread support. No package manager or external C++ library is required.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Run `./build/demo` on single-configuration generators; Visual Studio uses `build/Release/demo.exe`. The example prints the squares from 1 to 100.

## Contract

| Operation | Behavior |
| --- | --- |
| `try_submit(callable)` | Returns an optional future; empty when full or closed |
| `close()` | Idempotent; may be called from a worker; does not join |
| Destructor | Closes admission, drains accepted work, joins workers |
| `stats()` | Consistent snapshot under the queue mutex |

Capacity bounds **pending tasks**, excluding currently running tasks. FIFO applies to dispatch, not completion. Admission does not wait for queue space, but can wait for a mutex and performs allocation. This is not a lock-free or hard-real-time API. Move-only callables are supported; a nullary lambda can capture any arguments you need.

Task exceptions are stored in the future. A future can become ready just before the completed counter increments. Callers must not use the object concurrently with its destruction.

## Shutdown and liveness

Tasks are not forcibly cancelled. A task that never returns will keep destruction waiting. Do not destroy the executor from one of its own tasks, and do not synchronously wait inside a task for work requiring the same fully occupied pool. Those patterns violate the lifetime/liveness contract.

## Validation

Tests exercise invalid configuration, deterministic queue saturation, rejection after close, repeated close, exception propagation, move-only tasks, four concurrent producers and draining 4000 accepted tasks. Assertions remain active in Release builds. CI covers Linux, Windows and macOS in Debug/Release, plus Linux ASan/UBSan.

See [architecture](docs/architecture.md), [validation record](docs/validation.md), and [contributing](CONTRIBUTING.md). MIT licensed. This is a focused reference library; production deployment should include workload-specific load and shutdown testing.

