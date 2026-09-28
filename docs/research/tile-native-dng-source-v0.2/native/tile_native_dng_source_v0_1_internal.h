#pragma once

#include "tile_native_dng_source_v0_1.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace truthraw::tile_dng_v0_1::detail {

inline constexpr std::uint16_t TIFF_BYTE=1, TIFF_SHORT=3, TIFF_LONG=4, TIFF_RATIONAL=5, TIFF_UNDEFINED=7, TIFF_DOUBLE=12;
inline constexpr std::uint16_t TAG_ImageWidth=256, TAG_ImageLength=257, TAG_BitsPerSample=258, TAG_Compression=259;
inline constexpr std::uint16_t TAG_Photometric=262, TAG_FillOrder=266, TAG_StripOffsets=273, TAG_Orientation=274;
inline constexpr std::uint16_t TAG_SamplesPerPixel=277, TAG_RowsPerStrip=278, TAG_StripByteCounts=279;
inline constexpr std::uint16_t TAG_PlanarConfiguration=284, TAG_TileWidth=322, TAG_TileLength=323;
inline constexpr std::uint16_t TAG_TileOffsets=324, TAG_TileByteCounts=325, TAG_SubIFDs=330, TAG_SampleFormat=339;
inline constexpr std::uint16_t TAG_CFARepeatPatternDim=33421, TAG_CFAPattern=33422;
inline constexpr std::uint16_t TAG_CFAPlaneColor=50710, TAG_BlackLevelRepeatDim=50713, TAG_BlackLevel=50714, TAG_WhiteLevel=50717;
inline constexpr std::uint16_t TAG_OpcodeList2=51009, TAG_NoiseProfile=51041;
inline constexpr std::uint16_t PHOTO_CFA=32803;

inline std::size_t type_size(std::uint16_t t) {
    switch (t) {
        case TIFF_BYTE:
        case TIFF_UNDEFINED: return 1;
        case TIFF_SHORT: return 2;
        case TIFF_LONG: return 4;
        case TIFF_RATIONAL:
        case TIFF_DOUBLE: return 8;
        default: return 0;
    }
}
inline bool host_little() {
    const std::uint16_t v=1;
    return *reinterpret_cast<const std::uint8_t*>(&v)==1;
}
inline std::uint16_t dec16(const std::uint8_t* p,bool le) {
    return le ? std::uint16_t(p[0]|(std::uint16_t(p[1])<<8))
              : std::uint16_t((std::uint16_t(p[0])<<8)|p[1]);
}
inline std::uint32_t dec32(const std::uint8_t* p,bool le) {
    if (le) return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
    return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|std::uint32_t(p[3]);
}
inline std::uint32_t be32(const std::uint8_t* p) { return dec32(p,false); }
inline std::int32_t bei32(const std::uint8_t* p) { return static_cast<std::int32_t>(be32(p)); }
inline float befloat(const std::uint8_t* p) { auto u=be32(p); float f; std::memcpy(&f,&u,4); return f; }
inline double bedouble(const std::uint8_t* p) { std::uint64_t u=0; for(int i=0;i<8;++i)u=(u<<8)|p[i]; double d; std::memcpy(&d,&u,8); return d; }
inline bool mul_ok(std::uint64_t a,std::uint64_t b,std::uint64_t& out) {
    if(a && b>std::numeric_limits<std::uint64_t>::max()/a) return false;
    out=a*b; return true;
}
inline bool add_ok(std::uint64_t a,std::uint64_t b,std::uint64_t& out) {
    if(b>std::numeric_limits<std::uint64_t>::max()-a) return false;
    out=a+b; return true;
}
inline StreamStatus source_error(const std::string& s) {
    return StreamStatus::error(StreamStatusCode::SourceFailed,s);
}

} // namespace truthraw::tile_dng_v0_1::detail
