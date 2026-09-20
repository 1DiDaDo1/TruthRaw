#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace truthraw::ordered_parallel_executor::v0_1 {

struct Status final {
    bool ok = true;
    std::string message;

    static Status success() { return {}; }
    static Status error(std::string why) {
        Status s;
        s.ok = false;
        s.message = std::move(why);
        return s;
    }
    explicit operator bool() const noexcept { return ok; }
};

/**
 * Bounded parallel compute with strictly ordered commit.
 *
 * - compute(index, workerIndex, result) may run concurrently.
 * - commit(index, result) runs only on the caller thread and strictly in
 *   increasing index order.
 * - at most maxInFlight task results exist at once.
 * - first failure cancels further assignment; already-running workers exit.
 *
 * This is intended for TruthRaw tile compute where expensive per-tile math can
 * be parallelized while byte streams, digests and provenance remain canonical.
 */
template <class Result, class ComputeFn, class CommitFn>
Status run(
    std::size_t taskCount,
    int requestedWorkers,
    std::size_t maxInFlight,
    ComputeFn&& compute,
    CommitFn&& commit) {
    if (taskCount == 0u) return Status::success();
    if (requestedWorkers <= 0) {
        return Status::error("ordered parallel executor: worker count must be positive");
    }

    const std::size_t workers = std::min<std::size_t>(
        static_cast<std::size_t>(requestedWorkers),
        taskCount);
    maxInFlight = std::max<std::size_t>(maxInFlight, workers);
    maxInFlight = std::min<std::size_t>(maxInFlight, taskCount);

    enum class SlotState : unsigned char { Empty, Computing, Ready, Failed };
    struct Slot final {
        std::size_t taskIndex = 0u;
        SlotState state = SlotState::Empty;
        std::optional<Result> result;
        std::string error;
    };

    std::vector<Slot> slots(maxInFlight);
    std::mutex mutex;
    std::condition_variable cv;
    std::size_t nextAssign = 0u;
    std::size_t nextCommit = 0u;
    bool cancelled = false;
    std::string firstError;

    auto workerBody = [&](std::size_t workerIndex) {
        while (true) {
            std::size_t index = 0u;
            std::size_t slotIndex = 0u;
            {
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock, [&] {
                    return cancelled ||
                           nextAssign >= taskCount ||
                           nextAssign < nextCommit + maxInFlight;
                });
                if (cancelled || nextAssign >= taskCount) return;

                index = nextAssign++;
                slotIndex = index % maxInFlight;
                auto& slot = slots[slotIndex];

                cv.wait(lock, [&] {
                    return cancelled || slot.state == SlotState::Empty;
                });
                if (cancelled) return;

                slot.taskIndex = index;
                slot.state = SlotState::Computing;
                slot.result.reset();
                slot.error.clear();
            }

            std::optional<Result> produced;
            std::string error;
            try {
                Result result{};
                const Status status = compute(index, workerIndex, result);
                if (status) {
                    produced.emplace(std::move(result));
                } else {
                    error = status.message.empty()
                        ? "ordered parallel executor: compute failed"
                        : status.message;
                }
            } catch (const std::exception& e) {
                error = e.what();
            } catch (...) {
                error = "ordered parallel executor: compute threw unknown exception";
            }

            {
                std::lock_guard<std::mutex> lock(mutex);
                auto& slot = slots[slotIndex];
                if (!error.empty()) {
                    slot.state = SlotState::Failed;
                    slot.error = error;
                    if (firstError.empty()) firstError = error;
                    cancelled = true;
                } else {
                    slot.result = std::move(produced);
                    slot.state = SlotState::Ready;
                }
            }
            cv.notify_all();
        }
    };

    std::vector<std::thread> pool;
    pool.reserve(workers);
    for (std::size_t worker = 0u; worker < workers; ++worker) {
        pool.emplace_back(workerBody, worker);
    }

    Status finalStatus = Status::success();
    while (nextCommit < taskCount) {
        const std::size_t slotIndex = nextCommit % maxInFlight;
        std::optional<Result> value;

        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [&] {
                const auto& slot = slots[slotIndex];
                return cancelled ||
                       (slot.taskIndex == nextCommit &&
                        (slot.state == SlotState::Ready ||
                         slot.state == SlotState::Failed));
            });

            auto& slot = slots[slotIndex];
            if (slot.taskIndex == nextCommit && slot.state == SlotState::Failed) {
                finalStatus = Status::error(
                    slot.error.empty() ? firstError : slot.error);
                cancelled = true;
                cv.notify_all();
                break;
            }

            if (cancelled &&
                !(slot.taskIndex == nextCommit && slot.state == SlotState::Ready)) {
                finalStatus = Status::error(
                    firstError.empty()
                        ? "ordered parallel executor: cancelled"
                        : firstError);
                cv.notify_all();
                break;
            }

            if (!slot.result.has_value()) {
                finalStatus = Status::error(
                    "ordered parallel executor: ready slot has no result");
                cancelled = true;
                cv.notify_all();
                break;
            }
            value.emplace(std::move(*slot.result));
        }

        Status commitStatus;
        try {
            commitStatus = commit(nextCommit, *value);
        } catch (const std::exception& e) {
            commitStatus = Status::error(e.what());
        } catch (...) {
            commitStatus = Status::error(
                "ordered parallel executor: commit threw unknown exception");
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            auto& slot = slots[slotIndex];
            slot.result.reset();
            slot.error.clear();
            slot.state = SlotState::Empty;

            if (!commitStatus) {
                finalStatus = commitStatus;
                cancelled = true;
            } else {
                ++nextCommit;
            }
        }
        cv.notify_all();

        if (!commitStatus) break;
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        cancelled = true;
    }
    cv.notify_all();
    for (auto& thread : pool) {
        if (thread.joinable()) thread.join();
    }

    if (!finalStatus) return finalStatus;
    if (nextCommit != taskCount) {
        return Status::error(
            firstError.empty()
                ? "ordered parallel executor: incomplete commit sequence"
                : firstError);
    }
    return Status::success();
}

} // namespace truthraw::ordered_parallel_executor::v0_1
