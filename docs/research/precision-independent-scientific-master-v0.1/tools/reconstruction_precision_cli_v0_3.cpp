#include "reconstruction_precision_reference_v0_3.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

template <typename T>
bool read_exact_vector(const std::string& path, std::size_t count, std::vector<T>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.resize(count);
    f.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(count * sizeof(T)));
    if (!f) return false;
    char extra = 0;
    f.read(&extra, 1);
    return f.gcount() == 0;
}

bool parse_cfa(const std::string& s, truthraw::CfaPattern& out) {
    if (s == "BGGR") { out = truthraw::CfaPattern::BGGR; return true; }
    if (s == "RGGB") { out = truthraw::CfaPattern::RGGB; return true; }
    if (s == "GRBG") { out = truthraw::CfaPattern::GRBG; return true; }
    if (s == "GBRG") { out = truthraw::CfaPattern::GBRG; return true; }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 12) {
        std::cerr << "usage: reconstruction_precision_cli_v0_3 <f32.bin> <f64.bin> <tileW> <tileH> <globalHx0> <globalHy0> <coreX0> <coreY0> <coreW> <coreH> <CFA>\n";
        return 2;
    }

    const std::string f32Path = argv[1];
    const std::string f64Path = argv[2];
    const int tileW = std::stoi(argv[3]);
    const int tileH = std::stoi(argv[4]);
    const int globalHx0 = std::stoi(argv[5]);
    const int globalHy0 = std::stoi(argv[6]);
    const int coreX0 = std::stoi(argv[7]);
    const int coreY0 = std::stoi(argv[8]);
    const int coreW = std::stoi(argv[9]);
    const int coreH = std::stoi(argv[10]);
    truthraw::CfaPattern cfa{};
    if (!parse_cfa(argv[11], cfa) || tileW <= 0 || tileH <= 0 || coreW <= 0 || coreH <= 0) {
        std::cerr << "invalid arguments\n";
        return 2;
    }

    const std::size_t count = static_cast<std::size_t>(tileW) * static_cast<std::size_t>(tileH);
    std::vector<float> f32;
    std::vector<double> f64;
    if (!read_exact_vector(f32Path, count, f32) || !read_exact_vector(f64Path, count, f64)) {
        std::cerr << "failed to read exact tile payloads\n";
        return 3;
    }

    truthraw_precision_v03::ReconstructionTraceV03 trace32, trace64;
    std::vector<float> out32;
    std::vector<double> out64;
    if (!truthraw_precision_v03::v47i_edge_aware_reconstruct_f32_v0_3(
            f32.data(), tileW, tileH, globalHx0, globalHy0,
            coreX0, coreY0, coreW, coreH, cfa, out32, &trace32) ||
        !truthraw_precision_v03::v47i_edge_aware_reconstruct_f64_v0_3(
            f64.data(), tileW, tileH, globalHx0, globalHy0,
            coreX0, coreY0, coreW, coreH, cfa, out64, &trace64)) {
        std::cerr << "reconstruction failed\n";
        return 4;
    }

    const auto s = truthraw_precision_v03::compare_reconstruction_precision_v0_3(
        out32, trace32, out64, trace64);

    std::cout
        << "{"
        << "\"rgbSamples\":" << s.rgbSamples << ","
        << "\"maxAbsError\":" << s.maxAbsError << ","
        << "\"rmsError\":" << s.rmsError << ","
        << "\"greenDirectionDivergence\":" << s.greenDirectionDivergence << ","
        << "\"greenClampDivergence\":" << s.greenClampDivergence << ","
        << "\"colorClampDivergence\":" << s.colorClampDivergence << ","
        << "\"measuredChannelViolationsF32\":" << s.measuredChannelViolationsF32 << ","
        << "\"measuredChannelViolationsF64\":" << s.measuredChannelViolationsF64
        << "}" << std::endl;
    return 0;
}
