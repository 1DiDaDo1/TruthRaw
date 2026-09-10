#include "uncertainty_aware_scurve_v1.h"
#include "xyz_uncertainty_adapter_v1.h"
#include <cmath>
#include <iostream>

using namespace truthraw_scurve_v1;
#define CHECK(x) do { if(!(x)){ std::cerr << "CHECK failed: " #x " at " << __FILE__ << ':' << __LINE__ << '\n'; return 1; } } while(0)
static bool near(double a,double b,double e=1e-8){return std::abs(a-b)<=e;}

int main(){
    AppearanceParamsV1 p;
    AppearanceResultV1 out;
    AppearanceEvidenceV1 e;

    CHECK(apply_uncertainty_aware_scurve_v1({0.18,0.18,0.18},e,p,out));
    CHECK(out.luminanceConfidence==0.0);
    CHECK(out.requestedChromaGain <= 1.0);
    CHECK(out.appearanceOnly);

    e.luminanceSigmaKnown=true; e.luminanceSigmaUpper=0.02;
    CHECK(apply_uncertainty_aware_scurve_v1({0.02,0.02,0.02},e,p,out));
    CHECK(out.luminanceConfidence < 0.05);
    CHECK(out.localToneSlope < 1.0);
    CHECK(out.sigmaOutUpper < e.luminanceSigmaUpper);

    e.luminanceSigmaUpper=0.001;
    CHECK(apply_uncertainty_aware_scurve_v1({0.35,0.35,0.35},e,p,out));
    CHECK(out.luminanceConfidence > 0.99);
    CHECK(out.localToneSlope > 1.0);

    e.chromaConfidenceKnown=true; e.chromaConfidence=0.0;
    CHECK(apply_uncertainty_aware_scurve_v1({0.30,0.22,0.18},e,p,out));
    CHECK(out.requestedChromaGain < 1.0);
    const double lowGain=out.appliedChromaGain;
    e.chromaConfidence=1.0;
    CHECK(apply_uncertainty_aware_scurve_v1({0.30,0.22,0.18},e,p,out));
    CHECK(out.requestedChromaGain > 1.0);
    CHECK(out.appliedChromaGain >= lowGain);

    e.sourceHighCensored=true;
    CHECK(apply_uncertainty_aware_scurve_v1({0.30,0.22,0.18},e,p,out));
    CHECK(out.chromaConfidenceUsed==0.0);
    CHECK(out.requestedChromaGain < 1.0);

    truthraw_v07::PixelXyzD50UncertaintyV07 xyz;
    AppearanceEvidenceV1 ex;
    xyz.varianceKnownMask = 1u<<1; xyz.variance[1]=0.0004;
    CHECK(extract_y_sigma_upper_from_xyz_v0_7(xyz,ex));
    CHECK(ex.luminanceSigmaKnown && near(ex.luminanceSigmaUpper,0.02));
    xyz.varianceKnownMask=0; xyz.varianceBounds[1].valid=true; xyz.varianceBounds[1].upper=0.0009;
    CHECK(extract_y_sigma_upper_from_xyz_v0_7(xyz,ex));
    CHECK(ex.luminanceSigmaKnown && near(ex.luminanceSigmaUpper,0.03));
    xyz.varianceBounds[1].valid=false;
    CHECK(extract_y_sigma_upper_from_xyz_v0_7(xyz,ex));
    CHECK(!ex.luminanceSigmaKnown && std::isnan(ex.luminanceSigmaUpper));

    for(int i=0;i<2000;++i){
        const double r=(i%17)/16.0, g=(i%31)/30.0, b=(i%47)/46.0;
        AppearanceEvidenceV1 q; q.luminanceSigmaKnown=true; q.luminanceSigmaUpper=0.001+0.02*((i%13)/12.0);
        q.chromaConfidenceKnown=true; q.chromaConfidence=(i%19)/18.0;
        CHECK(apply_uncertainty_aware_scurve_v1({r,g,b},q,p,out));
        for(double v:out.rgb) CHECK(std::isfinite(v) && v>=0.0 && v<=1.0);
    }
    std::cout << "TRUTHRAW_UNCERTAINTY_AWARE_SCURVE_V1_TESTS_PASS\n";
    return 0;
}
