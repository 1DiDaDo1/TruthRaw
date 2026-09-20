#include "../truthraw_ordered_parallel_executor_v0_1.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace exec = truthraw::ordered_parallel_executor::v0_1;

int main() {
    {
        std::atomic<int> active{0};
        std::atomic<int> peak{0};
        std::vector<std::size_t> committed;
        std::mutex committedMutex;

        const auto status = exec::run<int>(
            48u,
            6,
            12u,
            [&](std::size_t index, std::size_t worker, int& result) {
                const int now = ++active;
                int old = peak.load();
                while (now > old && !peak.compare_exchange_weak(old, now)) {}
                std::this_thread::sleep_for(
                    std::chrono::microseconds(50 + ((47u - index) % 7u) * 20u));
                result = static_cast<int>(index * 17u + worker * 0u);
                --active;
                return exec::Status::success();
            },
            [&](std::size_t index, const int& result) {
                assert(result == static_cast<int>(index * 17u));
                std::lock_guard<std::mutex> lock(committedMutex);
                committed.push_back(index);
                return exec::Status::success();
            });

        assert(status);
        assert(committed.size() == 48u);
        for (std::size_t i = 0; i < committed.size(); ++i) {
            assert(committed[i] == i);
        }
        assert(peak.load() > 1);
        assert(peak.load() <= 6);
    }

    {
        std::vector<std::size_t> committed;
        const auto status = exec::run<int>(
            32u,
            4,
            8u,
            [&](std::size_t index, std::size_t, int& result) {
                if (index == 9u) {
                    return exec::Status::error("intentional task failure");
                }
                result = static_cast<int>(index);
                return exec::Status::success();
            },
            [&](std::size_t index, const int&) {
                committed.push_back(index);
                return exec::Status::success();
            });

        assert(!status);
        assert(status.message == "intentional task failure");
        for (std::size_t i = 0; i < committed.size(); ++i) {
            assert(committed[i] == i);
            assert(committed[i] < 9u);
        }
    }

    {
        const auto status = exec::run<int>(
            10u,
            3,
            4u,
            [&](std::size_t index, std::size_t, int& result) {
                result = static_cast<int>(index);
                return exec::Status::success();
            },
            [&](std::size_t index, const int&) {
                if (index == 5u) {
                    return exec::Status::error("intentional commit failure");
                }
                return exec::Status::success();
            });
        assert(!status);
        assert(status.message == "intentional commit failure");
    }

    std::cout << "ordered_parallel_executor_v0_1: PASS\n";
    return 0;
}
