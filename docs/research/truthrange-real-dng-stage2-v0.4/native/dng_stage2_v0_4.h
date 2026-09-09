#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw_v04 {

struct GainMapOpcode {
    std::uint32_t opcodeId=0, minVersion=0, flags=0;
    std::array<std::int32_t,4> area{}; // t,l,b,r
    std::uint32_t plane=0, planes=0, rowPitch=0, colPitch=0;
    std::uint32_t pointsV=0, pointsH=0, mapPlanes=0;
    double spacingV=0, spacingH=0, originV=0, originH=0;
    std::vector<float> values;
    bool applies(int y,int x) const;
    float entry(std::uint32_t r,std::uint32_t c,std::uint32_t p=0) const;
    float interpolate(int y,int x,int imageH,int imageW,std::uint32_t p=0) const;
};

struct ClassicDng {
    bool littleEndian=true;
    int width=0,height=0;
    std::uint16_t bitsPerSample=0, compression=0, photometric=0, samplesPerPixel=0;
    std::uint32_t rowsPerStrip=0;
    std::vector<std::uint32_t> stripOffsets, stripByteCounts;
    std::array<std::uint16_t,2> cfaRepeat{};
    std::array<std::uint8_t,4> cfaPattern{};
    std::array<float,4> blackPhase{};
    float whiteLevel=0;
    std::vector<std::uint8_t> opcodeList2;
    std::vector<GainMapOpcode> gainMaps;
    std::vector<std::uint16_t> raw;

    int phaseIndex(int y,int x) const { return (y&1)*2+(x&1); }
    float gainAt(int y,int x) const;
    float stage2At(int y,int x) const;
};

struct Stage2Summary {
    double minValue=0,maxValue=0,mean=0;
    std::uint64_t negativeCount=0,over1Count=0,clipCount=0,total=0;
    std::uint64_t floatBitsSum=0;
    std::uint32_t floatBitsXor=0;
};

ClassicDng readClassicDng(const std::string& path);
std::vector<GainMapOpcode> parseOpcodeList2(const std::vector<std::uint8_t>& bytes);
Stage2Summary summarizeStage2(const ClassicDng& dng);

} // namespace truthraw_v04
