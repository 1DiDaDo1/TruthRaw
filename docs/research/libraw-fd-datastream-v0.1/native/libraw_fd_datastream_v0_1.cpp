#include "libraw_fd_datastream_v0_1.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace truthraw::libraw_fd_datastream::v0_1 {
namespace {
std::string safe_string(const char* s) {
    return s ? std::string{s} : std::string{};
}

bool checked_add(INT64 a, INT64 b, INT64& out) noexcept {
    if ((b > 0 && a > std::numeric_limits<INT64>::max() - b) ||
        (b < 0 && a < std::numeric_limits<INT64>::min() - b)) {
        return false;
    }
    out = a + b;
    return true;
}
}

BorrowedFdDatastream::BorrowedFdDatastream(int fd) noexcept : fd_(fd) {
    if (fd_ < 0) return;
    struct stat st {};
    if (::fstat(fd_, &st) != 0 || st.st_size < 0) return;
    if (static_cast<unsigned long long>(st.st_size) >
        static_cast<unsigned long long>(std::numeric_limits<INT64>::max())) {
        return;
    }
    fileSize_ = static_cast<INT64>(st.st_size);
    position_ = 0;
    valid_ = true;
}

int BorrowedFdDatastream::valid() {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    return valid_ ? 1 : 0;
}

int BorrowedFdDatastream::read(void* ptr, size_t elementSize, size_t nmemb) {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    if (!valid_ || (ptr == nullptr && elementSize != 0 && nmemb != 0)) return 0;
    if (elementSize == 0 || nmemb == 0) return 0;
    if (nmemb > std::numeric_limits<size_t>::max() / elementSize) return 0;

    const size_t requested = elementSize * nmemb;
    auto* dst = static_cast<unsigned char*>(ptr);
    size_t done = 0;
    while (done < requested) {
        if (position_ < 0 || position_ >= fileSize_) break;
        const INT64 remaining64 = fileSize_ - position_;
        const size_t remainingFile = remaining64 > static_cast<INT64>(std::numeric_limits<size_t>::max())
            ? std::numeric_limits<size_t>::max()
            : static_cast<size_t>(remaining64);
        const size_t chunk = std::min(requested - done, remainingFile);
        if (chunk == 0) break;

        const ssize_t n = ::pread(fd_, dst + done, chunk, static_cast<off_t>(position_));
        if (n < 0) {
            if (errno == EINTR) continue;
            valid_ = false;
            break;
        }
        if (n == 0) break;
        done += static_cast<size_t>(n);
        position_ += static_cast<INT64>(n);
    }

    const size_t members = done / elementSize;
    return members > static_cast<size_t>(std::numeric_limits<int>::max())
        ? std::numeric_limits<int>::max()
        : static_cast<int>(members);
}

int BorrowedFdDatastream::seek(INT64 offset, int whence) {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    if (!valid_) return -1;

    INT64 base = 0;
    switch (whence) {
        case SEEK_SET: base = 0; break;
        case SEEK_CUR: base = position_; break;
        case SEEK_END: base = fileSize_; break;
        default: return -1;
    }

    INT64 next = 0;
    if (!checked_add(base, offset, next) || next < 0) return -1;
    position_ = next;
    return 0;
}

INT64 BorrowedFdDatastream::tell() {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    return valid_ ? position_ : -1;
}

INT64 BorrowedFdDatastream::size() {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    return valid_ ? fileSize_ : -1;
}

int BorrowedFdDatastream::get_char() {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    unsigned char c = 0;
    const int n = read(&c, 1, 1);
    return n == 1 ? static_cast<int>(c) : -1;
}

char* BorrowedFdDatastream::gets(char* s, int n) {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    if (!valid_ || s == nullptr || n <= 0) return nullptr;
    int written = 0;
    while (written < n - 1) {
        const int c = get_char();
        if (c < 0) break;
        s[written++] = static_cast<char>(c);
        if (c == '\n') break;
    }
    if (written == 0) return nullptr;
    s[written] = '\0';
    return s;
}

int BorrowedFdDatastream::scanf_one(const char* fmt, void* val) {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    if (!valid_ || fmt == nullptr || val == nullptr || position_ >= fileSize_) return EOF;

    char buffer[4096]{};
    const INT64 remaining64 = fileSize_ - position_;
    const size_t want = std::min<size_t>(sizeof(buffer) - 1,
        remaining64 > static_cast<INT64>(sizeof(buffer) - 1)
            ? sizeof(buffer) - 1
            : static_cast<size_t>(remaining64));
    ssize_t got = 0;
    do {
        got = ::pread(fd_, buffer, want, static_cast<off_t>(position_));
    } while (got < 0 && errno == EINTR);
    if (got <= 0) return EOF;
    buffer[static_cast<size_t>(got)] = '\0';

    std::string fmtWithCount{fmt};
    fmtWithCount += "%n";
    int consumed = 0;
    const int rc = std::sscanf(buffer, fmtWithCount.c_str(), val, &consumed);
    if (rc == 1 && consumed >= 0) position_ += static_cast<INT64>(consumed);
    return rc;
}

int BorrowedFdDatastream::eof() {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    return (!valid_ || position_ >= fileSize_) ? 1 : 0;
}

int BorrowedFdDatastream::lock() {
    mutex_.lock();
    return 1;
}

void BorrowedFdDatastream::unlock() {
    mutex_.unlock();
}

probe::ProbeStatus probe_fd(int borrowedFd, probe::ProbeResult& out) noexcept {
    out = {};
    BorrowedFdDatastream stream{borrowedFd};
    if (!stream.valid()) {
        return {probe::ProbeStatusCode::InvalidArgument, 0, "invalid or unstatable fd"};
    }

    LibRaw raw;
    const int rc = raw.open_datastream(&stream);
    if (rc != LIBRAW_SUCCESS) {
        return {probe::ProbeStatusCode::OpenFailed, rc, safe_string(libraw_strerror(rc))};
    }

    const auto& idata = raw.imgdata.idata;
    const auto& sizes = raw.imgdata.sizes;
    if (idata.raw_count == 0) {
        raw.recycle();
        return {probe::ProbeStatusCode::Unrecognized, 0, "LibRaw reported raw_count=0"};
    }

    probe::TopologyInput topologyInput{};
    topologyInput.rawCount = idata.raw_count;
    topologyInput.isFoveon = idata.is_foveon;
    topologyInput.dngVersion = idata.dng_version;
    topologyInput.colors = idata.colors;
    topologyInput.filters = idata.filters;

    out.librawVersion = safe_string(LibRaw::version());
    out.cameraMake = safe_string(idata.make);
    out.cameraModel = safe_string(idata.model);
    out.normalizedMake = safe_string(idata.normalized_make);
    out.normalizedModel = safe_string(idata.normalized_model);
    out.software = safe_string(idata.software);
    out.rawCount = idata.raw_count;
    out.dngVersion = idata.dng_version;
    out.filters = idata.filters;
    out.colors = idata.colors;
    out.rawWidth = sizes.raw_width;
    out.rawHeight = sizes.raw_height;
    out.visibleWidth = sizes.width;
    out.visibleHeight = sizes.height;
    out.topology = probe::classify_topology(topologyInput);
    out.container = probe::classify_container(topologyInput);
    out.requiresFrameSelection = idata.raw_count > 1;
    out.pixelsUnpacked = false;

    libraw_decoder_info_t decoder{};
    if (raw.get_decoder_info(&decoder) == LIBRAW_SUCCESS && decoder.decoder_name) {
        out.decoderName = decoder.decoder_name;
    }

    raw.recycle();
    return {};
}

} // namespace truthraw::libraw_fd_datastream::v0_1
