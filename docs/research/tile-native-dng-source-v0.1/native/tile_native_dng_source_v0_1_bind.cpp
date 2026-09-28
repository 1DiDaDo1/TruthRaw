#include "tile_native_dng_source_v0_1_internal.h"

#include <cmath>
#include <limits>

namespace truthraw::tile_dng_v0_1 {
using namespace detail;
DngSourceStatus TileNativeDngSource::parseGainMaps(const TagRef&tag,const OpenOptions&options){
    if (tag.type != TIFF_UNDEFINED) return DngSourceStatus::error(DngSourceCode::InvalidTag, "OpcodeList2 must be UNDEFINED");
    if (tag.dataBytes > options.maxOpcodeListBytes) return DngSourceStatus::error(DngSourceCode::BudgetExceeded, "OpcodeList2 exceeds configured cap");
    std::vector<std::uint8_t>b(static_cast<std::size_t>(tag.dataBytes));if(!readTagBytes(tag,0,b.data(),b.size()))return DngSourceStatus::error(DngSourceCode::IoError,"cannot read OpcodeList2");if(b.size()<4)return DngSourceStatus::error(DngSourceCode::InvalidTag,"OpcodeList2 too short");
    std::size_t p=0;auto count=be32(b.data());p=4;if(count>(b.size()-4)/16)return DngSourceStatus::error(DngSourceCode::InvalidTag,"invalid opcode count");gainMaps_.clear();gainMaps_.reserve(count);
    for(std::uint32_t i=0;i<count;++i){if(p+16>b.size())return DngSourceStatus::error(DngSourceCode::InvalidTag,"truncated opcode header");auto id=be32(b.data()+p),sz=be32(b.data()+p+12);p+=16;if(sz>b.size()-p)return DngSourceStatus::error(DngSourceCode::InvalidTag,"opcode payload out of range");auto end=p+sz;if(id!=9)return DngSourceStatus::error(DngSourceCode::InvalidTag,"v0.1 rejects non-GainMap OpcodeList2 entries");if(sz<76)return DngSourceStatus::error(DngSourceCode::InvalidTag,"truncated GainMap");GainMap g;g.area={bei32(b.data()+p),bei32(b.data()+p+4),bei32(b.data()+p+8),bei32(b.data()+p+12)};g.plane=be32(b.data()+p+16);g.planes=be32(b.data()+p+20);g.rowPitch=be32(b.data()+p+24);g.colPitch=be32(b.data()+p+28);p+=32;g.pointsV=be32(b.data()+p);g.pointsH=be32(b.data()+p+4);p+=8;g.spacingV=bedouble(b.data()+p);g.spacingH=bedouble(b.data()+p+8);p+=16;g.originV=bedouble(b.data()+p);g.originH=bedouble(b.data()+p+8);p+=16;g.mapPlanes=be32(b.data()+p);p+=4;
        if (g.area[2] <= g.area[0] || g.area[3] <= g.area[1] || g.planes < 1 || g.rowPitch < 1 || g.colPitch < 1 ||
            g.pointsV < 1 || g.pointsH < 1 || g.mapPlanes != 1 || !std::isfinite(g.spacingV) || !std::isfinite(g.spacingH) ||
            g.spacingV <= 0 || g.spacingH <= 0) {
            return DngSourceStatus::error(DngSourceCode::InvalidTag, "unsupported GainMap geometry");
        }
        std::uint64_t n64 = 0;
        if (!mul_ok(g.pointsV, g.pointsH, n64) || !mul_ok(n64, g.mapPlanes, n64) || n64 > (end-p)/4 || p+n64*4 != end) {
            return DngSourceStatus::error(DngSourceCode::InvalidTag, "GainMap payload mismatch");
        }
        g.values.resize(static_cast<std::size_t>(n64));
        for (std::size_t j=0; j<g.values.size(); ++j) {
            g.values[j]=befloat(b.data()+p+4*j);
            if (!std::isfinite(g.values[j]) || g.values[j] <= 0) return DngSourceStatus::error(DngSourceCode::InvalidTag, "invalid GainMap value");
        }
        p=end;
        gainMaps_.push_back(std::move(g));
    }
    if(p!=b.size())return DngSourceStatus::error(DngSourceCode::InvalidTag,"OpcodeList2 trailing bytes");
    for(int py=0;py<2;++py)for(int px=0;px<2;++px){int c=0;for(const auto&g:gainMaps_)if(g.applies(py,px))++c;if(c!=1)return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"GainMap must cover each CFA phase exactly once");}
    metadata_.hasGainField=!gainMaps_.empty();audit_.gainMapPresent=metadata_.hasGainField;return DngSourceStatus::ok();
}

DngSourceStatus TileNativeDngSource::selectAndBindRawIfd(const OpenOptions&options,const std::vector<Ifd>&ifds){
    std::vector<const Ifd*> candidates;
    for(const auto&ifd:ifds){const auto*p=findTag(ifd,TAG_Photometric);if(!p)continue;std::uint32_t v=0;if(readUnsigned(*p,0,v)&&p->count==1&&v==PHOTO_CFA)candidates.push_back(&ifd);}
    const Ifd* chosen=nullptr;if(options.explicitRawIfdOffset){for(auto*c:candidates)if(c->offset==options.explicitRawIfdOffset)chosen=c;if(!chosen)return DngSourceStatus::error(DngSourceCode::UnsupportedPhotometric,"explicit raw IFD is not a supported CFA IFD");}else{if(candidates.empty())return DngSourceStatus::error(DngSourceCode::UnsupportedPhotometric,"no CFA IFD found");if(candidates.size()!=1)return DngSourceStatus::error(DngSourceCode::AmbiguousRawIfd,"multiple CFA IFDs require explicitRawIfdOffset");chosen=candidates.front();}
    rawIfd_=*chosen;audit_.rawIfdOffset=chosen->offset;
    auto req=[&](std::uint16_t tag)->const TagRef*{return findTag(rawIfd_,tag);};
    const TagRef*w=req(TAG_ImageWidth),*h=req(TAG_ImageLength),*bps=req(TAG_BitsPerSample),*comp=req(TAG_Compression),*photo=req(TAG_Photometric),*spp=req(TAG_SamplesPerPixel),*cr=req(TAG_CFARepeatPatternDim),*cp=req(TAG_CFAPattern),*bl=req(TAG_BlackLevel),*wl=req(TAG_WhiteLevel);
    if(!w||!h||!bps||!comp||!photo||!spp||!cr||!cp||!bl||!wl)return DngSourceStatus::error(DngSourceCode::MissingRequiredTag,"required DNG/CFA tag missing");
    std::uint32_t W=0,H=0,B=0,C=0,P=0,S=0;if(!readUnsigned(*w,0,W)||!readUnsigned(*h,0,H)||!readUnsigned(*bps,0,B)||!readUnsigned(*comp,0,C)||!readUnsigned(*photo,0,P)||!readUnsigned(*spp,0,S))return DngSourceStatus::error(DngSourceCode::InvalidTag,"invalid scalar TIFF tag");
    if (W<2 || H<2 || W>std::uint32_t(std::numeric_limits<int>::max()) || H>std::uint32_t(std::numeric_limits<int>::max())) return DngSourceStatus::error(DngSourceCode::InvalidTag,"invalid dimensions");
    if (B != 16) return DngSourceStatus::error(DngSourceCode::UnsupportedBitsPerSample,"v0.1 supports 16-bit TIFF sample storage only");
    if (C != 1) return DngSourceStatus::error(DngSourceCode::UnsupportedCompression,"v0.1 supports Compression=1 only");
    if (P != PHOTO_CFA) return DngSourceStatus::error(DngSourceCode::UnsupportedPhotometric,"CFA photometric required");
    if (S != 1) return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"SamplesPerPixel must be 1");
    if(const auto*f=findTag(rawIfd_,TAG_FillOrder)){std::uint32_t v=0;if(!readUnsigned(*f,0,v)||v!=1)return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"FillOrder other than 1 unsupported");}if(const auto*pconf=findTag(rawIfd_,TAG_PlanarConfiguration)){std::uint32_t v=0;if(!readUnsigned(*pconf,0,v)||v!=1)return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"PlanarConfiguration must be chunky/1");}if(const auto*sf=findTag(rawIfd_,TAG_SampleFormat)){std::uint32_t v=0;if(!readUnsigned(*sf,0,v)||v!=1)return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"SampleFormat must be unsigned integer");}
    if (cr->count != 2) return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"CFARepeatPatternDim count must be 2");
    std::uint32_t cr0=0, cr1=0;
    if (!readUnsigned(*cr,0,cr0) || !readUnsigned(*cr,1,cr1) || cr0!=2 || cr1!=2) return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"2x2 CFA required");
    if (cp->type != TIFF_BYTE || cp->count != 4) return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"CFAPattern must be 4 BYTE values");
    std::uint8_t pat[4];
    if (!readTagBytes(*cp,0,pat,4)) return DngSourceStatus::error(DngSourceCode::IoError,"cannot read CFA pattern");
    if (std::memcmp(pat,"\x02\x01\x01\x00",4)==0) metadata_.cfa=CfaPattern::BGGR;
    else if (std::memcmp(pat,"\x00\x01\x01\x02",4)==0) metadata_.cfa=CfaPattern::RGGB;
    else if (std::memcmp(pat,"\x01\x00\x02\x01",4)==0) metadata_.cfa=CfaPattern::GRBG;
    else if (std::memcmp(pat,"\x01\x02\x00\x01",4)==0) metadata_.cfa=CfaPattern::GBRG;
    else return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"unsupported 2x2 RGB CFA pattern");
    metadata_.width=static_cast<int>(W);metadata_.height=static_cast<int>(H);metadata_.orientation=Orientation::Normal;if(const auto*o=findTag(rawIfd_,TAG_Orientation)){std::uint32_t v=0;if(!readUnsigned(*o,0,v))return DngSourceStatus::error(DngSourceCode::InvalidTag,"invalid Orientation");if(v==1)metadata_.orientation=Orientation::Normal;else if(v==3)metadata_.orientation=Orientation::Rotate180;else if(v==6)metadata_.orientation=Orientation::Rotate90CW;else if(v==8)metadata_.orientation=Orientation::Rotate90CCW;else return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"unsupported Orientation");}
    double white=0;
    if (wl->count != 1 || !readFloatLike(*wl,0,white) || !std::isfinite(white) || !(white > 0)) return DngSourceStatus::error(DngSourceCode::InvalidTag,"invalid WhiteLevel");
    metadata_.whiteLevel=float(white);
    if(bl->count==1){double b=0;if(!readFloatLike(*bl,0,b)||!std::isfinite(b))return DngSourceStatus::error(DngSourceCode::InvalidTag,"invalid BlackLevel");metadata_.blackPhase.fill(float(b));}else if(bl->count==4){if(const auto*br=findTag(rawIfd_,TAG_BlackLevelRepeatDim)){std::uint32_t a=0,b=0;if(br->count!=2||!readUnsigned(*br,0,a)||!readUnsigned(*br,1,b)||a!=2||b!=2)return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"BlackLevelRepeatDim must be 2x2 for four phase values");}for(int i=0;i<4;++i){double b=0;if(!readFloatLike(*bl,i,b)||!std::isfinite(b))return DngSourceStatus::error(DngSourceCode::InvalidTag,"invalid BlackLevel phase");metadata_.blackPhase[i]=float(b);}}else return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"BlackLevel must contain 1 or 4 values");for(float b:metadata_.blackPhase)if(!(metadata_.whiteLevel>b))return DngSourceStatus::error(DngSourceCode::InvalidTag,"WhiteLevel must exceed every BlackLevel phase");
    metadata_.hasNoiseProfile=false;if(const auto*np=findTag(rawIfd_,TAG_NoiseProfile)){if(np->type!=TIFF_DOUBLE||np->count!=6)return DngSourceStatus::error(DngSourceCode::InvalidTag,"NoiseProfile must be six DOUBLE values");if(const auto*pc=findTag(rawIfd_,TAG_CFAPlaneColor)){if(pc->type!=TIFF_BYTE||pc->count!=3)return DngSourceStatus::error(DngSourceCode::InvalidTag,"CFAPlaneColor invalid");std::uint8_t rgb[3];if(!readTagBytes(*pc,0,rgb,3)||rgb[0]!=0||rgb[1]!=1||rgb[2]!=2)return DngSourceStatus::error(DngSourceCode::UnsupportedTopology,"NoiseProfile requires CFAPlaneColor RGB order 0,1,2 in v0.1");}else return DngSourceStatus::error(DngSourceCode::MissingRequiredTag,"NoiseProfile requires CFAPlaneColor binding");for(int i=0;i<6;++i){double v=0;if(!readFloatLike(*np,i,v)||v<0||!std::isfinite(v))return DngSourceStatus::error(DngSourceCode::InvalidTag,"invalid NoiseProfile");metadata_.noiseProfile[i]=float(v);}metadata_.hasNoiseProfile=true;}
    metadata_.hasGainField=false;if(const auto*op=findTag(rawIfd_,TAG_OpcodeList2)){auto s=parseGainMaps(*op,options);if(!s)return s;}
    const auto*so=findTag(rawIfd_,TAG_StripOffsets),*sb=findTag(rawIfd_,TAG_StripByteCounts),*tw=findTag(rawIfd_,TAG_TileWidth),*tl=findTag(rawIfd_,TAG_TileLength),*to=findTag(rawIfd_,TAG_TileOffsets),*tb=findTag(rawIfd_,TAG_TileByteCounts);
    bool strips=so&&sb,tiles=tw&&tl&&to&&tb;if(strips==tiles)return DngSourceStatus::error(DngSourceCode::InvalidStorage,"exactly one of strip or tile storage is required");
    std::uint64_t rowBytes=std::uint64_t(W)*2;
    if(strips){const auto*rps=findTag(rawIfd_,TAG_RowsPerStrip);if(!rps)return DngSourceStatus::error(DngSourceCode::MissingRequiredTag,"RowsPerStrip missing");std::uint32_t r=0;if(!readUnsigned(*rps,0,r)||r<1)return DngSourceStatus::error(DngSourceCode::InvalidStorage,"invalid RowsPerStrip");rowsPerStrip_=r;strileCount_=(H+r-1)/r;if(so->count!=strileCount_||sb->count!=strileCount_)return DngSourceStatus::error(DngSourceCode::InvalidStorage,"strip offset/count cardinality mismatch");offsets_=*so;byteCounts_=*sb;tiled_=false;
        (void)rowBytes; // Per-strip offset/count values are validated lazily on each requested row.
    }else{std::uint32_t x=0,y=0;if(!readUnsigned(*tw,0,x)||!readUnsigned(*tl,0,y)||x<1||y<1)return DngSourceStatus::error(DngSourceCode::InvalidStorage,"invalid tile dimensions");tileWidth_=x;tileLength_=y;std::uint64_t nx=(W+x-1)/x,ny=(H+y-1)/y,n=0;if(!mul_ok(nx,ny,n)||n>std::numeric_limits<std::uint32_t>::max())return DngSourceStatus::error(DngSourceCode::InvalidStorage,"tile count overflow");strileCount_=static_cast<std::uint32_t>(n);if(to->count!=strileCount_||tb->count!=strileCount_)return DngSourceStatus::error(DngSourceCode::InvalidStorage,"tile offset/count cardinality mismatch");offsets_=*to;byteCounts_=*tb;tiled_=true;}
    audit_.strileCount=strileCount_;audit_.tiledStorage=tiled_;return DngSourceStatus::ok();
}

std::size_t TileNativeDngSource::computeResidentUpperBound()const{
    std::size_t n=sizeof(*this)+(bytes_?bytes_->residentBytesUpperBound():0)+rawIfd_.tags.capacity()*sizeof(TagRef)+metadata_.sourceId.capacity()+colorBindingId_.capacity();for(const auto&g:gainMaps_)n+=sizeof(GainMap)+g.values.capacity()*sizeof(float);return n;
}
DngSourceStatus TileNativeDngSource::initialize(const OpenOptions&options){
    if (!bytes_ || bytes_->sizeBytes() < 8) return DngSourceStatus::error(DngSourceCode::InvalidTiff,"byte source missing or too small");
    if (options.sourceEvidenceId.empty()) return DngSourceStatus::error(DngSourceCode::BindingMissing,"sourceEvidenceId is required");
    if (!options.color.valid || options.color.bindingId.empty()) return DngSourceStatus::error(DngSourceCode::BindingMissing,"explicit color binding is required");
    for (float v : options.color.cameraToXyzD50) if (!std::isfinite(v)) return DngSourceStatus::error(DngSourceCode::BindingMissing,"color matrix contains non-finite value");
    metadata_.sourceId=options.sourceEvidenceId;
    metadata_.cameraToXyzD50=options.color.cameraToXyzD50;
    colorBindingId_=options.color.bindingId;
    metadata_.hasResidualBlack=false;
    std::vector<Ifd> ifds;auto s=discoverIfds(options,ifds);if(!s)return s;s=selectAndBindRawIfd(options,ifds);if(!s)return s;residentUpperBound_=computeResidentUpperBound();if(options.maxResidentBytes&&residentUpperBound_>options.maxResidentBytes)return DngSourceStatus::error(DngSourceCode::BudgetExceeded,"DNG source resident bound exceeds configured cap");return DngSourceStatus::ok();
}
DngSourceStatus TileNativeDngSource::open(std::shared_ptr<IRandomAccessByteSource> bytes,const OpenOptions&options,std::unique_ptr<TileNativeDngSource>&out){auto p=std::unique_ptr<TileNativeDngSource>(new TileNativeDngSource);p->bytes_=std::move(bytes);auto s=p->initialize(options);if(!s)return s;out=std::move(p);return DngSourceStatus::ok();}

} // namespace truthraw::tile_dng_v0_1
