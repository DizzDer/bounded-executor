#include "bounded_executor.hpp"
#include <iostream>

int main() {
    concurrency::bounded_executor pool(4, 32);
    std::vector<std::future<int>> results;
    for (int i = 1; i <= 10; ++i) {
        auto submitted = pool.try_submit([i] { return i * i; });
        if (!submitted) { std::cerr << "Backpressure: queue full\n"; return 1; }
        results.push_back(std::move(*submitted));
    }
    pool.close();
    for (auto& result : results) std::cout << result.get() << '\n';
}
