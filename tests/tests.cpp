#include "bounded_executor.hpp"
#include <atomic>
#include <iostream>
#include <string>

#define CHECK(x) do { if (!(x)) throw std::runtime_error("check failed: " #x); } while (false)

int main() {
    try {
        bool invalid = false;
        try { tasking::bounded_executor invalid_pool(0, 1); }
        catch (const std::invalid_argument&) { invalid = true; }
        CHECK(invalid);

        // Synchronization, rather than sleeps, makes queue saturation deterministic.
        tasking::bounded_executor pool(1, 1);
        std::promise<void> started, release;
        auto gate = release.get_future().share();
        auto first = pool.try_submit([&] { started.set_value(); gate.wait(); return 7; });
        started.get_future().wait();
        auto second = pool.try_submit([] { return 8; });
        const bool saturated = !pool.try_submit([] {}).has_value();
        pool.close();
        pool.close();
        const bool closed = !pool.try_submit([] {}).has_value();
        release.set_value(); // release before assertions so failure cannot hang destruction
        CHECK(saturated && closed && first && second);
        CHECK(first->get() == 7 && second->get() == 8);
        CHECK(pool.stats().accepted == 2);
        CHECK(pool.stats().rejected == 2);

        tasking::bounded_executor exceptions(1, 4);
        auto failure = exceptions.try_submit([]() -> int { throw std::runtime_error("task error"); });
        bool propagated = false;
        try { (void)failure->get(); } catch (const std::runtime_error&) { propagated = true; }
        CHECK(propagated);
        auto move_only = exceptions.try_submit([p = std::make_unique<int>(42)] { return *p; });
        CHECK(move_only->get() == 42);

        std::atomic<int> calls{0};
        {
            tasking::bounded_executor stress(4, 4000);
            std::vector<std::thread> producers;
            std::atomic<bool> rejected{false};
            for (int p = 0; p < 4; ++p) producers.emplace_back([&] {
                for (int i = 0; i < 1000; ++i)
                    if (!stress.try_submit([&] { ++calls; })) rejected = true;
            });
            for (auto& producer : producers) producer.join();
            CHECK(!rejected);
        } // destructor must drain every accepted task
        CHECK(calls == 4000);
        std::cout << "All executor tests passed (including 4000 concurrent submissions).\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
