#pragma once

#include <cstdint>

namespace truthraw::performance_hint::v0_1 {

class CpuWorkerSession final {
public:
    CpuWorkerSession() noexcept = default;
    ~CpuWorkerSession();

    CpuWorkerSession(const CpuWorkerSession&) = delete;
    CpuWorkerSession& operator=(const CpuWorkerSession&) = delete;
    CpuWorkerSession(CpuWorkerSession&&) = delete;
    CpuWorkerSession& operator=(CpuWorkerSession&&) = delete;

    bool ensure_started(std::int64_t targetDurationNanos) noexcept;
    void report_actual(std::int64_t actualDurationNanos) noexcept;
    void close() noexcept;

    bool active() const noexcept { return session_ != nullptr; }
    bool everStarted() const noexcept { return everStarted_; }

private:
    void* session_ = nullptr;
    bool attempted_ = false;
    bool everStarted_ = false;
};

} // namespace truthraw::performance_hint::v0_1
