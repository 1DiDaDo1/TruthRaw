#pragma once

#include "libraw_compat_probe_v0_1.h"

#include <libraw/libraw.h>

#include <cstddef>
#include <cstdint>
#include <mutex>

namespace truthraw::libraw_fd_datastream::v0_1 {

namespace probe = truthraw::libraw_compat_probe::v0_1;

class BorrowedFdDatastream final : public LibRaw_abstract_datastream {
public:
    explicit BorrowedFdDatastream(int fd) noexcept;
    ~BorrowedFdDatastream() override = default;

    BorrowedFdDatastream(const BorrowedFdDatastream&) = delete;
    BorrowedFdDatastream& operator=(const BorrowedFdDatastream&) = delete;

    int valid() override;
    int read(void* ptr, size_t size, size_t nmemb) override;
    int seek(INT64 offset, int whence) override;
    INT64 tell() override;
    INT64 size() override;
    int get_char() override;
    char* gets(char* s, int n) override;
    int scanf_one(const char* fmt, void* val) override;
    int eof() override;
    int lock() override;
    void unlock() override;

    [[nodiscard]] int borrowedFd() const noexcept { return fd_; }
    [[nodiscard]] std::size_t residentBytesUpperBound() const noexcept {
        return sizeof(*this);
    }

private:
    int fd_ = -1;
    INT64 fileSize_ = -1;
    INT64 position_ = 0;
    bool valid_ = false;
    std::recursive_mutex mutex_;
};

[[nodiscard]] probe::ProbeStatus probe_fd(
    int borrowedFd,
    probe::ProbeResult& out) noexcept;

} // namespace truthraw::libraw_fd_datastream::v0_1
