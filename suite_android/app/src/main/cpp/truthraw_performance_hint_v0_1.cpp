#include "truthraw_performance_hint_v0_1.h"

#include <android/performance_hint.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>

namespace truthraw::performance_hint::v0_1 {
namespace {

std::int32_t current_tid() noexcept {
    return static_cast<std::int32_t>(::syscall(SYS_gettid));
}

} // namespace

CpuWorkerSession::~CpuWorkerSession() {
    close();
}

bool CpuWorkerSession::ensure_started(
    std::int64_t targetDurationNanos) noexcept {
    if (session_ != nullptr) return true;
    if (attempted_) return false;
    attempted_ = true;

    if (targetDurationNanos <= 0) return false;
    APerformanceHintManager* manager = APerformanceHint_getManager();
    if (manager == nullptr) return false;

    const std::int32_t tid = current_tid();
    if (tid <= 0) return false;

    session_ = APerformanceHint_createSession(
        manager,
        &tid,
        1u,
        targetDurationNanos);
    everStarted_ = session_ != nullptr;
    return session_ != nullptr;
}

void CpuWorkerSession::report_actual(
    std::int64_t actualDurationNanos) noexcept {
    if (session_ == nullptr || actualDurationNanos <= 0) return;
    (void)APerformanceHint_reportActualWorkDuration(
        session_,
        actualDurationNanos);
}

void CpuWorkerSession::close() noexcept {
    if (session_ == nullptr) return;
    APerformanceHint_closeSession(session_);
    session_ = nullptr;
}

} // namespace truthraw::performance_hint::v0_1
