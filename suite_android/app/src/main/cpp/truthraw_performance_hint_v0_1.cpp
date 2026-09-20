#include "truthraw_performance_hint_v0_1.h"

#include <dlfcn.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>

namespace truthraw::performance_hint::v0_1 {
namespace {

using GetManagerFn = void* (*)();
using CreateSessionFn = void* (*)(
    void*,
    const std::int32_t*,
    std::size_t,
    std::int64_t);
using ReportActualFn = int (*)(void*, std::int64_t);
using CloseSessionFn = void (*)(void*);

struct Api final {
    void* handle = nullptr;
    GetManagerFn getManager = nullptr;
    CreateSessionFn createSession = nullptr;
    ReportActualFn reportActual = nullptr;
    CloseSessionFn closeSession = nullptr;

    Api() noexcept {
        handle = ::dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
        if (handle == nullptr) return;
        getManager = reinterpret_cast<GetManagerFn>(
            ::dlsym(handle, "APerformanceHint_getManager"));
        createSession = reinterpret_cast<CreateSessionFn>(
            ::dlsym(handle, "APerformanceHint_createSession"));
        reportActual = reinterpret_cast<ReportActualFn>(
            ::dlsym(handle, "APerformanceHint_reportActualWorkDuration"));
        closeSession = reinterpret_cast<CloseSessionFn>(
            ::dlsym(handle, "APerformanceHint_closeSession"));

        if (getManager == nullptr ||
            createSession == nullptr ||
            reportActual == nullptr ||
            closeSession == nullptr) {
            getManager = nullptr;
            createSession = nullptr;
            reportActual = nullptr;
            closeSession = nullptr;
        }
        // Keep libandroid loaded for process lifetime. Function pointers stored
        // above must remain valid for all worker sessions.
    }
};

Api& api() noexcept {
    static Api instance;
    return instance;
}

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
    auto& runtime = api();
    if (runtime.getManager == nullptr ||
        runtime.createSession == nullptr) {
        return false;
    }

    void* manager = runtime.getManager();
    if (manager == nullptr) return false;

    const std::int32_t tid = current_tid();
    if (tid <= 0) return false;

    session_ = runtime.createSession(
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
    auto& runtime = api();
    if (runtime.reportActual == nullptr) return;
    (void)runtime.reportActual(session_, actualDurationNanos);
}

void CpuWorkerSession::close() noexcept {
    if (session_ == nullptr) return;
    auto& runtime = api();
    if (runtime.closeSession != nullptr) {
        runtime.closeSession(session_);
    }
    session_ = nullptr;
}

} // namespace truthraw::performance_hint::v0_1
