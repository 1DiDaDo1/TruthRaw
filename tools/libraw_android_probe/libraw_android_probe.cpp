#include <libraw/libraw.h>

#include <cstdint>

extern "C" std::uint32_t truthraw_libraw_probe_version_number() {
    return static_cast<std::uint32_t>(LIBRAW_VERSION);
}

extern "C" const char* truthraw_libraw_probe_version_string() {
    return LibRaw::version();
}

extern "C" int truthraw_libraw_probe_construct_and_recycle() {
    LibRaw processor;
    processor.recycle();
    return 0;
}
