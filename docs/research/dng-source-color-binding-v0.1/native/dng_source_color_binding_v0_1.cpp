#include "dng_source_color_binding_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace truthraw::dng_source_color_v0_1 {
namespace {

constexpr std::uint16_t kTiffMagic = 42;
constexpr std::uint32_t kMaxRootIfdEntries = 512;
constexpr std::size_t kMaxTagPayloadBytes = 512;

constexpr std::uint16_t kTagDngVersion = 50706;
constexpr std::uint16_t kTagColorMatrix1 = 50721;
constexpr std::uint16_t kTagColorMatrix2 = 50722;
constexpr std::uint16_t kTagCameraCalibration1 = 50723;
constexpr std::uint16_t kTagCameraCalibration2 = 50724;
constexpr std::uint16_t kTagAnalogBalance = 50727;
constexpr std::uint16_t kTagAsShotNeutral = 50728;
constexpr std::uint16_t kTagAsShotWhiteXY = 50729;
constexpr std::uint16_t kTagCalibrationIlluminant1 = 50778;
constexpr std::uint16_t kTagCalibrationIlluminant2 = 50779;
constexpr std::uint16_t kTagColorimetricReference = 50879;
constexpr std::uint16_t kTagCameraCalibrationSignature = 50931;
constexpr std::uint16_t kTagProfileCalibrationSignature = 50932;
constexpr std::uint16_t kTagExtraCameraProfiles = 50933;
constexpr std::uint16_t kTagAsShotProfileName = 50934;
constexpr std::uint16_t kTagForwardMatrix1 = 50964;
constexpr std::uint16_t kTagForwardMatrix2 = 50965;
constexpr std::uint16_t kTagCalibrationIlluminant3 = 52529;
constexpr std::uint16_t kTagCameraCalibration3 = 52530;
constexpr std::uint16_t kTagColorMatrix3 = 52531;
constexpr std::uint16_t kTagForwardMatrix3 = 52532;
constexpr std::uint16_t kTagReductionMatrix3 = 52538;

constexpr std::uint16_t kTypeByte = 1;
constexpr std::uint16_t kTypeAscii = 2;
constexpr std::uint16_t kTypeShort = 3;
constexpr std::uint16_t kTypeLong = 4;
constexpr std::uint16_t kTypeRational = 5;
constexpr std::uint16_t kTypeSRational = 10;

using Mat3 = std::array<double, 9>;
using Vec3 = std::array<double, 3>;

constexpr Vec3 kPcsD50 = {
    0.3457 / 0.3585,
    1.0,
    (1.0 - 0.3457 - 0.3585) / 0.3585
};

struct Entry {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::array<std::uint8_t, 4> value{};
};

struct Parsed {
    std::array<std::uint8_t, 4> dngVersion{};
    bool hasDngVersion = false;
    std::optional<Mat3> colorMatrix1;
    std::optional<Mat3> forwardMatrix1;
    std::optional<Mat3> cameraCalibration1;
    std::optional<Vec3> analogBalance;
    std::optional<Vec3> asShotNeutral;
    std::optional<std::array<double, 2>> asShotWhiteXY;
    std::optional<std::uint16_t> calibrationIlluminant1;
    std::optional<std::uint16_t> colorimetricReference;
    std::optional<std::string> cameraCalibrationSignature;
    std::optional<std::string> profileCalibrationSignature;
    bool unsupportedProfileTopology = false;
};

std::uint16_t u16(const std::uint8_t* p, bool little) noexcept {
    return little ? std::uint16_t(p[0] | (std::uint16_t(p[1]) << 8))
                  : std::uint16_t((std::uint16_t(p[0]) << 8) | p[1]);
}

std::uint32_t u32(const std::uint8_t* p, bool little) noexcept {
    if (little) {
        return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
               (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
    }
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
           (std::uint32_t(p[2]) << 8) | std::uint32_t(p[3]);
}

std::int32_t s32(const std::uint8_t* p, bool little) noexcept {
    return static_cast<std::int32_t>(u32(p, little));
}

std::size_t type_width(std::uint16_t type) noexcept {
    switch (type) {
        case kTypeByte: case kTypeAscii: return 1;
        case kTypeShort: return 2;
        case kTypeLong: return 4;
        case kTypeRational: case kTypeSRational: return 8;
        default: return 0;
    }
}

bool relevant(std::uint16_t tag) noexcept {
    switch (tag) {
        case kTagDngVersion: case kTagColorMatrix1: case kTagColorMatrix2:
        case kTagCameraCalibration1: case kTagCameraCalibration2: case kTagAnalogBalance:
        case kTagAsShotNeutral: case kTagAsShotWhiteXY: case kTagCalibrationIlluminant1:
        case kTagCalibrationIlluminant2: case kTagColorimetricReference:
        case kTagCameraCalibrationSignature: case kTagProfileCalibrationSignature:
        case kTagExtraCameraProfiles: case kTagAsShotProfileName:
        case kTagForwardMatrix1: case kTagForwardMatrix2:
        case kTagCalibrationIlluminant3: case kTagCameraCalibration3:
        case kTagColorMatrix3: case kTagForwardMatrix3: case kTagReductionMatrix3:
            return true;
        default: return false;
    }
}

bool unsupported_profile_tag(std::uint16_t tag) noexcept {
    switch (tag) {
        case kTagColorMatrix2: case kTagCameraCalibration2: case kTagCalibrationIlluminant2:
        case kTagForwardMatrix2: case kTagExtraCameraProfiles: case kTagAsShotProfileName:
        case kTagCalibrationIlluminant3: case kTagCameraCalibration3: case kTagColorMatrix3:
        case kTagForwardMatrix3: case kTagReductionMatrix3:
            return true;
        default: return false;
    }
}

class Reader {
public:
    Reader(tile_dng_v0_1::IRandomAccessByteSource& source, Metrics& metrics)
        : source_(source), metrics_(metrics) {}

    bool read(std::uint64_t offset, void* dst, std::size_t n) {
        if (offset > source_.sizeBytes() || n > source_.sizeBytes() - offset) return false;
        if (!source_.readExact(offset, dst, n)) return false;
        metrics_.metadataBytesRead += n;
        return true;
    }

    void setEndian(bool little) noexcept { little_ = little; metrics_.littleEndian = little; }
    bool little() const noexcept { return little_; }
    std::uint64_t size() const noexcept { return source_.sizeBytes(); }

    Status payload(const Entry& e, std::vector<std::uint8_t>& out) {
        const std::size_t width = type_width(e.type);
        if (width == 0) return Status::error(StatusCode::UnsupportedTagType, "unsupported TIFF type for required DNG color tag");
        if (e.count > std::numeric_limits<std::size_t>::max() / width)
            return Status::error(StatusCode::TagPayloadOutOfBounds, "tag payload size overflow");
        const std::size_t n = std::size_t(e.count) * width;
        if (n == 0 || n > kMaxTagPayloadBytes)
            return Status::error(StatusCode::TagPayloadOutOfBounds, "required DNG color tag payload exceeds bounded parser contract");
        out.resize(n);
        metrics_.temporaryBytesUpperBound = std::max(metrics_.temporaryBytesUpperBound, out.capacity() + std::size_t(32));
        if (n <= 4) {
            std::copy_n(e.value.begin(), n, out.begin());
            return Status::ok();
        }
        const std::uint32_t offset = u32(e.value.data(), little_);
        if (offset > size() || n > size() - offset)
            return Status::error(StatusCode::TagPayloadOutOfBounds, "DNG color tag points outside source bytes");
        if (!read(offset, out.data(), n))
            return Status::error(StatusCode::TagPayloadOutOfBounds, "failed reading DNG color tag payload");
        return Status::ok();
    }

private:
    tile_dng_v0_1::IRandomAccessByteSource& source_;
    Metrics& metrics_;
    bool little_ = true;
};

Status require_type_count(const Entry& e, std::uint16_t type, std::uint32_t count) {
    if (e.type != type) return Status::error(StatusCode::UnsupportedTagType, "DNG color tag has unsupported TIFF type");
    if (e.count != count) return Status::error(StatusCode::InvalidTagCount, "DNG color tag has unexpected element count");
    return Status::ok();
}

Status decode_srational_matrix(Reader& reader, const Entry& e, Mat3& out) {
    auto st = require_type_count(e, kTypeSRational, 9); if (!st) return st;
    std::vector<std::uint8_t> p; st = reader.payload(e, p); if (!st) return st;
    for (std::size_t i = 0; i < 9; ++i) {
        const std::int32_t n = s32(p.data() + 8*i, reader.little());
        const std::int32_t d = s32(p.data() + 8*i + 4, reader.little());
        if (d == 0) return Status::error(StatusCode::InvalidRational, "SRATIONAL denominator is zero");
        out[i] = double(n) / double(d);
        if (!std::isfinite(out[i])) return Status::error(StatusCode::InvalidRational, "non-finite SRATIONAL matrix value");
    }
    return Status::ok();
}

Status decode_rational_vec3(Reader& reader, const Entry& e, Vec3& out) {
    auto st = require_type_count(e, kTypeRational, 3); if (!st) return st;
    std::vector<std::uint8_t> p; st = reader.payload(e, p); if (!st) return st;
    for (std::size_t i = 0; i < 3; ++i) {
        const std::uint32_t n = u32(p.data() + 8*i, reader.little());
        const std::uint32_t d = u32(p.data() + 8*i + 4, reader.little());
        if (d == 0) return Status::error(StatusCode::InvalidRational, "RATIONAL denominator is zero");
        out[i] = double(n) / double(d);
        if (!(out[i] > 0.0) || !std::isfinite(out[i])) return Status::error(StatusCode::InvalidRational, "neutral/balance values must be positive finite");
    }
    return Status::ok();
}

Status decode_rational_xy(Reader& reader, const Entry& e, std::array<double,2>& out) {
    auto st = require_type_count(e, kTypeRational, 2); if (!st) return st;
    std::vector<std::uint8_t> p; st = reader.payload(e, p); if (!st) return st;
    for (std::size_t i = 0; i < 2; ++i) {
        const std::uint32_t n = u32(p.data() + 8*i, reader.little());
        const std::uint32_t d = u32(p.data() + 8*i + 4, reader.little());
        if (d == 0) return Status::error(StatusCode::InvalidRational, "AsShotWhiteXY denominator is zero");
        out[i] = double(n) / double(d);
    }
    if (!(out[0] > 0.0 && out[1] > 0.0 && out[0] + out[1] < 1.0))
        return Status::error(StatusCode::InvalidWhitePoint, "AsShotWhiteXY is not a valid chromaticity");
    return Status::ok();
}

Status decode_short(Reader& reader, const Entry& e, std::uint16_t& out) {
    auto st = require_type_count(e, kTypeShort, 1); if (!st) return st;
    std::vector<std::uint8_t> p; st = reader.payload(e, p); if (!st) return st;
    out = u16(p.data(), reader.little());
    return Status::ok();
}

Status decode_string(Reader& reader, const Entry& e, std::string& out) {
    if (e.type != kTypeAscii && e.type != kTypeByte)
        return Status::error(StatusCode::UnsupportedTagType, "calibration signature must be ASCII/BYTE");
    if (e.count == 0 || e.count > 256)
        return Status::error(StatusCode::InvalidTagCount, "calibration signature length outside bounded contract");
    std::vector<std::uint8_t> p; auto st = reader.payload(e, p); if (!st) return st;
    auto end = std::find(p.begin(), p.end(), std::uint8_t{0});
    out.assign(reinterpret_cast<const char*>(p.data()), static_cast<std::size_t>(end - p.begin()));
    return Status::ok();
}

Status parse_root_ifd(tile_dng_v0_1::IRandomAccessByteSource& source, Parsed& parsed, Metrics& metrics) {
    Reader reader(source, metrics);
    std::array<std::uint8_t,8> header{};
    if (!reader.read(0, header.data(), header.size())) return Status::error(StatusCode::InvalidTiff, "TIFF header unavailable");
    bool little = false;
    if (header[0]=='I' && header[1]=='I') little = true;
    else if (header[0]=='M' && header[1]=='M') little = false;
    else return Status::error(StatusCode::InvalidTiff, "invalid TIFF byte order marker");
    reader.setEndian(little);
    const auto magic = u16(header.data()+2, little);
    if (magic == 43) return Status::error(StatusCode::BigTiffUnsupported, "BigTIFF is outside DNG source-color v0.1");
    if (magic != kTiffMagic) return Status::error(StatusCode::InvalidTiff, "classic TIFF magic 42 required");
    const std::uint32_t ifdOffset = u32(header.data()+4, little);
    std::array<std::uint8_t,2> countBytes{};
    if (!reader.read(ifdOffset, countBytes.data(), countBytes.size())) return Status::error(StatusCode::RootIfdOutOfBounds, "root IFD count outside source");
    const std::uint32_t count = u16(countBytes.data(), little);
    metrics.rootIfdEntries = count;
    if (count > kMaxRootIfdEntries) return Status::error(StatusCode::TooManyIfdEntries, "root IFD entry count exceeds bounded color parser contract");
    const std::uint64_t entriesStart = std::uint64_t(ifdOffset) + 2u;
    const std::uint64_t entriesBytes = std::uint64_t(count) * 12u;
    if (entriesStart > reader.size() || entriesBytes > reader.size() - entriesStart ||
        entriesStart + entriesBytes > reader.size() - std::min<std::uint64_t>(4u, reader.size()))
        return Status::error(StatusCode::RootIfdOutOfBounds, "root IFD table outside source");

    std::set<std::uint16_t> seen;
    for (std::uint32_t i=0; i<count; ++i) {
        std::array<std::uint8_t,12> raw{};
        if (!reader.read(entriesStart + std::uint64_t(i)*12u, raw.data(), raw.size()))
            return Status::error(StatusCode::RootIfdOutOfBounds, "root IFD entry unavailable");
        Entry e;
        e.tag = u16(raw.data(), little); e.type = u16(raw.data()+2, little); e.count = u32(raw.data()+4, little);
        std::copy_n(raw.data()+8, 4, e.value.begin());
        if (!relevant(e.tag)) continue;
        if (!seen.insert(e.tag).second) return Status::error(StatusCode::DuplicateColorTag, "duplicate relevant DNG color tag in IFD0");
        if (unsupported_profile_tag(e.tag)) { parsed.unsupportedProfileTopology = true; continue; }

        Status st;
        switch (e.tag) {
            case kTagDngVersion: {
                st = require_type_count(e, kTypeByte, 4); if (!st) return st;
                std::vector<std::uint8_t> p; st = reader.payload(e,p); if (!st) return st;
                std::copy_n(p.begin(), 4, parsed.dngVersion.begin()); parsed.hasDngVersion = true; break;
            }
            case kTagColorMatrix1: {
                Mat3 m{}; st = decode_srational_matrix(reader,e,m); if (!st) return st; parsed.colorMatrix1=m; break;
            }
            case kTagForwardMatrix1: {
                Mat3 m{}; st = decode_srational_matrix(reader,e,m); if (!st) return st; parsed.forwardMatrix1=m; break;
            }
            case kTagCameraCalibration1: {
                Mat3 m{}; st = decode_srational_matrix(reader,e,m); if (!st) return st; parsed.cameraCalibration1=m; break;
            }
            case kTagAnalogBalance: {
                Vec3 v{}; st = decode_rational_vec3(reader,e,v); if (!st) return st; parsed.analogBalance=v; metrics.usedAnalogBalance=true; break;
            }
            case kTagAsShotNeutral: {
                Vec3 v{}; st = decode_rational_vec3(reader,e,v); if (!st) return st; parsed.asShotNeutral=v; metrics.usedAsShotNeutral=true; break;
            }
            case kTagAsShotWhiteXY: {
                std::array<double,2> xy{}; st=decode_rational_xy(reader,e,xy); if(!st)return st; parsed.asShotWhiteXY=xy; metrics.usedAsShotWhiteXY=true; break;
            }
            case kTagCalibrationIlluminant1: {
                std::uint16_t v=0; st=decode_short(reader,e,v); if(!st)return st; parsed.calibrationIlluminant1=v; metrics.calibrationIlluminant1=v; break;
            }
            case kTagColorimetricReference: {
                std::uint16_t v=0; st=decode_short(reader,e,v); if(!st)return st; parsed.colorimetricReference=v; break;
            }
            case kTagCameraCalibrationSignature: {
                std::string s; st=decode_string(reader,e,s); if(!st)return st; parsed.cameraCalibrationSignature=s; break;
            }
            case kTagProfileCalibrationSignature: {
                std::string s; st=decode_string(reader,e,s); if(!st)return st; parsed.profileCalibrationSignature=s; break;
            }
            default: break;
        }
    }
    return Status::ok();
}

Vec3 mul(const Mat3& m, const Vec3& v) noexcept {
    return {m[0]*v[0]+m[1]*v[1]+m[2]*v[2], m[3]*v[0]+m[4]*v[1]+m[5]*v[2], m[6]*v[0]+m[7]*v[1]+m[8]*v[2]};
}

Mat3 mul(const Mat3& a, const Mat3& b) noexcept {
    Mat3 r{};
    for (int y=0;y<3;++y) for(int x=0;x<3;++x)
        for(int k=0;k<3;++k) r[3*y+x]+=a[3*y+k]*b[3*k+x];
    return r;
}

Mat3 diag(const Vec3& v) noexcept { return {v[0],0,0, 0,v[1],0, 0,0,v[2]}; }

bool finite(const Mat3& m) noexcept { for(double x:m) if(!std::isfinite(x)) return false; return true; }
bool finite(const Vec3& v) noexcept { for(double x:v) if(!std::isfinite(x)) return false; return true; }

bool invert(const Mat3& m, Mat3& out) noexcept {
    const double c00=m[4]*m[8]-m[5]*m[7], c01=m[5]*m[6]-m[3]*m[8], c02=m[3]*m[7]-m[4]*m[6];
    const double c10=m[2]*m[7]-m[1]*m[8], c11=m[0]*m[8]-m[2]*m[6], c12=m[1]*m[6]-m[0]*m[7];
    const double c20=m[1]*m[5]-m[2]*m[4], c21=m[2]*m[3]-m[0]*m[5], c22=m[0]*m[4]-m[1]*m[3];
    const double det=m[0]*c00+m[1]*c01+m[2]*c02;
    if (!std::isfinite(det) || std::abs(det)<1e-12) return false;
    const double s=1.0/det;
    out={c00*s,c10*s,c20*s, c01*s,c11*s,c21*s, c02*s,c12*s,c22*s};
    return finite(out);
}

void round_4(Mat3& m) noexcept { for(double& x:m) x=std::round(x*10000.0)/10000.0; }

Status normalize_color_matrix(Mat3& m) {
    if (!finite(m)) return Status::error(StatusCode::InvalidColorMatrix,"ColorMatrix1 contains non-finite values");
    const Vec3 coord=mul(m,kPcsD50);
    const double maxCoord=std::max({coord[0],coord[1],coord[2]});
    if (!(maxCoord>0.0) || !std::isfinite(maxCoord)) return Status::error(StatusCode::InvalidColorMatrix,"ColorMatrix1 cannot be normalized");
    if (maxCoord<0.99 || maxCoord>1.01) for(double& x:m) x/=maxCoord;
    round_4(m);
    Mat3 inv{}; if(!invert(m,inv)) return Status::error(StatusCode::SingularMatrix,"normalized ColorMatrix1 is singular");
    return Status::ok();
}

Status normalize_forward_matrix(Mat3& m) {
    if (!finite(m)) return Status::error(StatusCode::InvalidForwardMatrix,"ForwardMatrix1 contains non-finite values");
    round_4(m);
    const Vec3 one{1.0,1.0,1.0};
    const Vec3 mapped=mul(m,one);
    for(int i=0;i<3;++i) {
        if (!std::isfinite(mapped[std::size_t(i)]) || std::abs(mapped[std::size_t(i)]-kPcsD50[std::size_t(i)])>0.01)
            return Status::error(StatusCode::InvalidForwardMatrix,"ForwardMatrix1 does not map equal camera values to XYZ D50 within DNG tolerance");
        if (std::abs(mapped[std::size_t(i)])<1e-12)
            return Status::error(StatusCode::InvalidForwardMatrix,"ForwardMatrix1 row sum is zero");
    }
    for(int row=0;row<3;++row) {
        const double scale=kPcsD50[std::size_t(row)]/mapped[std::size_t(row)];
        for(int col=0;col<3;++col) m[std::size_t(3*row+col)]*=scale;
    }
    return Status::ok();
}

Status xy_from_neutral(const Mat3& xyzToCamera, const Vec3& neutral, std::array<double,2>& xy) {
    Mat3 inv{}; if(!invert(xyzToCamera,inv)) return Status::error(StatusCode::SingularMatrix,"XYZ-to-camera matrix is singular while solving AsShotNeutral");
    const Vec3 xyz=mul(inv,neutral);
    const double sum=xyz[0]+xyz[1]+xyz[2];
    if (!(sum>0.0) || !finite(xyz)) return Status::error(StatusCode::InvalidWhitePoint,"AsShotNeutral maps to invalid XYZ");
    xy={xyz[0]/sum,xyz[1]/sum};
    if (!(xy[0]>0.0 && xy[1]>0.0 && xy[0]+xy[1]<1.0))
        return Status::error(StatusCode::InvalidWhitePoint,"AsShotNeutral resolves to invalid white xy");
    return Status::ok();
}

Vec3 xyz_from_xy(const std::array<double,2>& xy) noexcept {
    return {xy[0]/xy[1],1.0,(1.0-xy[0]-xy[1])/xy[1]};
}

Status build_camera_to_pcs(const Parsed& p, Mat3& out, Metrics& metrics) {
    if (p.unsupportedProfileTopology)
        return Status::error(StatusCode::UnsupportedProfileTopology,"v0.1 accepts only the embedded single-illuminant profile; dual/triple/alternate profile tags are present");
    if (!p.hasDngVersion) return Status::error(StatusCode::MissingDngVersion,"DNGVersion is required");
    if (p.dngVersion[0]!=1 || p.dngVersion[1]>7)
        return Status::error(StatusCode::UnsupportedDngVersion,"only DNG 1.0 through 1.7 are admitted by this parser subset");
    if (p.colorimetricReference && *p.colorimetricReference!=0)
        return Status::error(StatusCode::UnsupportedColorimetricReference,"output-referred ColorimetricReference is outside v0.1");
    if (!p.colorMatrix1) return Status::error(StatusCode::MissingColorMatrix1,"ColorMatrix1 is required");
    if (!p.forwardMatrix1) return Status::error(StatusCode::MissingForwardMatrix1,"ForwardMatrix1 is required by v0.1");
    if (!p.calibrationIlluminant1) return Status::error(StatusCode::MissingCalibrationIlluminant1,"CalibrationIlluminant1 is required");
    if (*p.calibrationIlluminant1==0 || *p.calibrationIlluminant1==255)
        return Status::error(StatusCode::UnsupportedProfileTopology,"unknown/custom CalibrationIlluminant1 is outside v0.1");
    if (bool(p.asShotNeutral)==bool(p.asShotWhiteXY))
        return Status::error(StatusCode::MissingWhitePoint,"v0.1 requires exactly one of AsShotNeutral or AsShotWhiteXY");

    Mat3 cm=*p.colorMatrix1; auto st=normalize_color_matrix(cm); if(!st)return st;
    Mat3 fm=*p.forwardMatrix1; st=normalize_forward_matrix(fm); if(!st)return st;
    const Vec3 analog=p.analogBalance.value_or(Vec3{1.0,1.0,1.0});
    const Mat3 analogM=diag(analog);
    Mat3 calibration{1,0,0,0,1,0,0,0,1};
    if (p.cameraCalibration1) {
        if (!p.cameraCalibrationSignature || !p.profileCalibrationSignature || p.cameraCalibrationSignature->empty() ||
            *p.cameraCalibrationSignature!=*p.profileCalibrationSignature)
            return Status::error(StatusCode::InvalidCalibrationSignature,"CameraCalibration1 requires matching non-empty camera/profile calibration signatures in v0.1");
        calibration=*p.cameraCalibration1;
        Mat3 tmp{}; if(!finite(calibration)||!invert(calibration,tmp)) return Status::error(StatusCode::SingularMatrix,"CameraCalibration1 is singular/non-finite");
        metrics.usedCameraCalibration1=true;
    }
    const Mat3 individual=mul(analogM,calibration);
    Mat3 individualToReference{}; if(!invert(individual,individualToReference)) return Status::error(StatusCode::SingularMatrix,"AnalogBalance*CameraCalibration1 is singular");
    const Mat3 xyzToCamera=mul(individual,cm);
    Mat3 invXyzToCamera{}; if(!invert(xyzToCamera,invXyzToCamera)) return Status::error(StatusCode::SingularMatrix,"effective ColorMatrix1 is singular");

    std::array<double,2> whiteXY{};
    if (p.asShotNeutral) { st=xy_from_neutral(xyzToCamera,*p.asShotNeutral,whiteXY); if(!st)return st; }
    else whiteXY=*p.asShotWhiteXY;
    Vec3 cameraWhite=mul(xyzToCamera,xyz_from_xy(whiteXY));
    if(!finite(cameraWhite)) return Status::error(StatusCode::InvalidWhitePoint,"camera white is non-finite");
    const double maxWhite=std::max({cameraWhite[0],cameraWhite[1],cameraWhite[2]});
    if(!(maxWhite>0.0)) return Status::error(StatusCode::InvalidWhitePoint,"camera white has no positive channel");
    for(double& x:cameraWhite) x=std::clamp(x/maxWhite,0.001,1.0);
    const Vec3 refWhite=mul(individualToReference,cameraWhite);
    for(double x:refWhite) if(!(x>0.0)||!std::isfinite(x)) return Status::error(StatusCode::InvalidWhitePoint,"reference camera white is invalid");
    const Mat3 invRefWhite=diag(Vec3{1.0/refWhite[0],1.0/refWhite[1],1.0/refWhite[2]});
    out=mul(mul(fm,invRefWhite),individualToReference);
    if(!finite(out)) return Status::error(StatusCode::NonFiniteResult,"CameraToXYZ(D50) result is non-finite");
    return Status::ok();
}

} // namespace

Status produce_source_bound_color_binding(
    tile_dng_v0_1::IRandomAccessByteSource& source,
    const scientific_preview_binding_v0_1::SourceSeal& sourceSeal,
    Result& out) {
    if (!scientific_preview_binding_v0_1::is_canonical_source_evidence_id(sourceSeal.sourceEvidenceId) || sourceSeal.byteLength==0)
        return Status::error(StatusCode::SourceSealRejected,"canonical non-empty source seal required");
    Parsed parsed; Metrics metrics;
    auto st=parse_root_ifd(source,parsed,metrics); if(!st)return st;
    Mat3 cameraToPcs{}; st=build_camera_to_pcs(parsed,cameraToPcs,metrics); if(!st)return st;
    const auto restatus=scientific_preview_binding_v0_1::reverify_source_sha256(source,sourceSeal);
    if(!restatus) return Status::error(StatusCode::SourceChanged,"source bytes changed or no longer match the sealed SHA-256");

    Result result;
    result.metrics=metrics;
    result.binding.authority=scientific_preview_binding_v0_1::ColorBindingAuthority::SourceMetadataBound;
    result.binding.sourceEvidenceId=sourceSeal.sourceEvidenceId;
    result.binding.bindingId="dng-source-color-v0.1:single-forward@"+sourceSeal.sourceEvidenceId;
    result.binding.normalized=true;
    result.binding.validated=true;
    result.binding.physicalFrameCount=1;
    result.binding.independentEvidenceCount=1;
    for(std::size_t i=0;i<9;++i) {
        const float v=static_cast<float>(cameraToPcs[i]);
        if(!std::isfinite(v)) return Status::error(StatusCode::NonFiniteResult,"CameraToXYZ(D50) cannot be represented as finite float");
        result.binding.cameraToXyzD50[i]=v;
    }
    out=std::move(result);
    return Status::ok();
}

const char* status_name(StatusCode code) noexcept {
    switch(code) {
        case StatusCode::Ok:return "OK"; case StatusCode::InvalidArgument:return "INVALID_ARGUMENT";
        case StatusCode::SourceSealRejected:return "SOURCE_SEAL_REJECTED"; case StatusCode::SourceChanged:return "SOURCE_CHANGED";
        case StatusCode::InvalidTiff:return "INVALID_TIFF"; case StatusCode::BigTiffUnsupported:return "BIGTIFF_UNSUPPORTED";
        case StatusCode::RootIfdOutOfBounds:return "ROOT_IFD_OUT_OF_BOUNDS"; case StatusCode::TooManyIfdEntries:return "TOO_MANY_IFD_ENTRIES";
        case StatusCode::DuplicateColorTag:return "DUPLICATE_COLOR_TAG"; case StatusCode::MissingDngVersion:return "MISSING_DNG_VERSION";
        case StatusCode::UnsupportedDngVersion:return "UNSUPPORTED_DNG_VERSION"; case StatusCode::UnsupportedColorimetricReference:return "UNSUPPORTED_COLORIMETRIC_REFERENCE";
        case StatusCode::UnsupportedProfileTopology:return "UNSUPPORTED_PROFILE_TOPOLOGY"; case StatusCode::MissingColorMatrix1:return "MISSING_COLOR_MATRIX1";
        case StatusCode::MissingForwardMatrix1:return "MISSING_FORWARD_MATRIX1"; case StatusCode::MissingWhitePoint:return "MISSING_WHITE_POINT";
        case StatusCode::MissingCalibrationIlluminant1:return "MISSING_CALIBRATION_ILLUMINANT1"; case StatusCode::UnsupportedTagType:return "UNSUPPORTED_TAG_TYPE";
        case StatusCode::InvalidTagCount:return "INVALID_TAG_COUNT"; case StatusCode::TagPayloadOutOfBounds:return "TAG_PAYLOAD_OUT_OF_BOUNDS";
        case StatusCode::InvalidRational:return "INVALID_RATIONAL"; case StatusCode::InvalidCalibrationSignature:return "INVALID_CALIBRATION_SIGNATURE";
        case StatusCode::InvalidColorMatrix:return "INVALID_COLOR_MATRIX"; case StatusCode::InvalidForwardMatrix:return "INVALID_FORWARD_MATRIX";
        case StatusCode::InvalidWhitePoint:return "INVALID_WHITE_POINT"; case StatusCode::SingularMatrix:return "SINGULAR_MATRIX";
        case StatusCode::NonFiniteResult:return "NON_FINITE_RESULT";
    }
    return "UNKNOWN";
}

} // namespace truthraw::dng_source_color_v0_1
