#include <libraw/libraw.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

namespace {

void printEscaped(const char* s) {
    std::cout << '"';
    if (s != nullptr) {
        for (const unsigned char ch : std::string(s)) {
            switch (ch) {
                case '\\': std::cout << "\\\\"; break;
                case '"': std::cout << "\\\""; break;
                case '\n': std::cout << "\\n"; break;
                case '\r': std::cout << "\\r"; break;
                case '\t': std::cout << "\\t"; break;
                default:
                    if (ch >= 0x20u) std::cout << static_cast<char>(ch);
                    break;
            }
        }
    }
    std::cout << '"';
}

const char* rawStorageKind(const libraw_data_t& d) {
    if (d.rawdata.raw_image) return "U16_SINGLE_CHANNEL";
    if (d.rawdata.color3_image) return "U16_THREE_CHANNEL";
    if (d.rawdata.color4_image) return "U16_FOUR_CHANNEL";
    if (d.rawdata.float_image) return "F32_SINGLE_CHANNEL";
    if (d.rawdata.float3_image) return "F32_THREE_CHANNEL";
    if (d.rawdata.float4_image) return "F32_FOUR_CHANNEL";
    return "NONE";
}

int fail(const char* stage, int code) {
    std::cerr << stage << " failed: " << code << " (" << libraw_strerror(code) << ")\n";
    return 2;
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::strcmp(argv[1], "--version-json") == 0) {
        std::cout << "{\n";
        std::cout << "  \"schema\": \"TruthRawLibRawReferenceProbe/0.1\",\n";
        std::cout << "  \"role\": \"REFERENCE_DECODER_ONLY\",\n";
        std::cout << "  \"librawVersion\": ";
        printEscaped(libraw_version());
        std::cout << ",\n";
        std::cout << "  \"librawVersionNumber\": " << libraw_versionNumber() << ",\n";
        std::cout << "  \"cameraCount\": " << libraw_cameraCount() << ",\n";
        std::cout << "  \"dcrawProcessCalled\": false,\n";
        std::cout << "  \"scientificMasterAdmissionAllowed\": false\n";
        std::cout << "}\n";
        return 0;
    }

    if (argc != 2) {
        std::cerr << "usage: truthraw_libraw_reference_probe <raw-file> | --version-json\n";
        return 64;
    }

    LibRaw raw;
    int status = raw.open_file(argv[1]);
    if (status != LIBRAW_SUCCESS) return fail("open_file", status);

    std::cout << "{\n";
    std::cout << "  \"schema\": \"TruthRawLibRawReferenceProbe/0.1\",\n";
    std::cout << "  \"role\": \"REFERENCE_DECODER_ONLY\",\n";
    std::cout << "  \"librawVersion\": ";
    printEscaped(libraw_version());
    std::cout << ",\n";
    std::cout << "  \"make\": ";
    printEscaped(raw.imgdata.idata.make);
    std::cout << ",\n";
    std::cout << "  \"model\": ";
    printEscaped(raw.imgdata.idata.model);
    std::cout << ",\n";
    std::cout << "  \"normalizedMake\": ";
    printEscaped(raw.imgdata.idata.normalized_make);
    std::cout << ",\n";
    std::cout << "  \"normalizedModel\": ";
    printEscaped(raw.imgdata.idata.normalized_model);
    std::cout << ",\n";
    std::cout << "  \"rawCount\": " << raw.imgdata.idata.raw_count << ",\n";
    std::cout << "  \"filters\": " << raw.imgdata.idata.filters << ",\n";
    std::cout << "  \"colors\": " << raw.imgdata.idata.colors << ",\n";
    std::cout << "  \"rawWidth\": " << raw.imgdata.sizes.raw_width << ",\n";
    std::cout << "  \"rawHeight\": " << raw.imgdata.sizes.raw_height << ",\n";
    std::cout << "  \"width\": " << raw.imgdata.sizes.width << ",\n";
    std::cout << "  \"height\": " << raw.imgdata.sizes.height << ",\n";
    std::cout << "  \"leftMargin\": " << raw.imgdata.sizes.left_margin << ",\n";
    std::cout << "  \"topMargin\": " << raw.imgdata.sizes.top_margin << ",\n";
    std::cout << "  \"rawBitsPerSampleHint\": " << raw.imgdata.color.raw_bps << ",\n";
    std::cout << "  \"blackHint\": " << raw.imgdata.color.black << ",\n";
    std::cout << "  \"maximumHint\": " << raw.imgdata.color.maximum << ",\n";
    std::cout << "  \"unpackFunction\": ";
    printEscaped(raw.unpack_function_name());
    std::cout << ",\n";

    status = raw.unpack();
    if (status != LIBRAW_SUCCESS) {
        std::cout << "  \"unpackStatus\": " << status << ",\n";
        std::cout << "  \"unpackError\": ";
        printEscaped(libraw_strerror(status));
        std::cout << ",\n";
        std::cout << "  \"scientificMasterAdmissionAllowed\": false\n";
        std::cout << "}\n";
        return 3;
    }

    std::cout << "  \"unpackStatus\": 0,\n";
    std::cout << "  \"decodedStorageKind\": \"" << rawStorageKind(raw.imgdata) << "\",\n";
    std::cout << "  \"fullDecodedRawMaterialized\": true,\n";
    std::cout << "  \"blackSubtractionRequestedByTruthRaw\": false,\n";
    std::cout << "  \"demosaicRequestedByTruthRaw\": false,\n";
    std::cout << "  \"dcrawProcessCalled\": false,\n";
    std::cout << "  \"directSensorAdcClaimAllowed\": false,\n";
    std::cout << "  \"scientificMasterAdmissionAllowed\": false\n";
    std::cout << "}\n";
    return 0;
}
