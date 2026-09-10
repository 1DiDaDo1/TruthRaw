#include "cicm_v1.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace truthraw::counterfactual::v1 {
namespace {
bool finite(double x) noexcept { return std::isfinite(x); }
bool positive(double x) noexcept { return finite(x) && x > 0.0; }
bool nonnegative(double x) noexcept { return finite(x) && x >= 0.0; }
bool hex_sha256(const std::string& s) noexcept {
    if (s.size()!=64) return false;
    for(char c:s) if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'))) return false;
    return true;
}
EvidenceLedger ledger_one() noexcept { EvidenceLedger e; e.counterfactualObservationCount=1; return e; }
}

const char* status_name(Status s) noexcept { switch(s){
 case Status::Ok:return "OK"; case Status::InvalidInput:return "INVALID_INPUT";
 case Status::CalibrationMissing:return "CALIBRATION_MISSING"; case Status::BindingMismatch:return "BINDING_MISMATCH";
 case Status::RelightStateIncomplete:return "RELIGHT_STATE_INCOMPLETE"; case Status::FullRelightNotImplemented:return "FULL_RELIGHT_NOT_IMPLEMENTED";
 case Status::NoAdmissibleCapture:return "NO_ADMISSIBLE_CAPTURE";} return "UNKNOWN"; }
const char* world_semantics_name(WorldSemantics s) noexcept { switch(s){
 case WorldSemantics::RelativeRadianceScaleOnly:return "RELATIVE_RADIANCE_SCALE_ONLY";
 case WorldSemantics::CalibratedNeutralIlluminationForward:return "CALIBRATED_NEUTRAL_ILLUMINATION_FORWARD";
 case WorldSemantics::GeometryBrdfSpectralRelightReserved:return "GEOMETRY_BRDF_SPECTRAL_RELIGHT_RESERVED";} return "UNKNOWN"; }
const char* calibration_authority_name(CalibrationAuthority a) noexcept { switch(a){
 case CalibrationAuthority::Unresolved:return "UNRESOLVED"; case CalibrationAuthority::ExplicitResearchFixture:return "EXPLICIT_RESEARCH_FIXTURE";
 case CalibrationAuthority::IndependentMeasurement:return "INDEPENDENT_MEASUREMENT";} return "UNKNOWN"; }

Status validate_scene_binding(const SceneBinding& b) noexcept {
    return (!b.sourceEvidenceId.empty() && !b.sceneScaleId.empty() && !b.zeroLineId.empty() && hex_sha256(b.scientificMasterSha256)) ? Status::Ok : Status::InvalidInput;
}
Status validate_world(const CounterfactualWorldSpec& w) noexcept {
    if (w.worldId.empty() || !positive(w.illuminationScale) || validate_scene_binding(w.sceneBinding)!=Status::Ok) return Status::InvalidInput;
    if (w.semantics==WorldSemantics::CalibratedNeutralIlluminationForward && w.illuminationReferenceId.empty()) return Status::InvalidInput;
    if (w.semantics==WorldSemantics::GeometryBrdfSpectralRelightReserved) return Status::FullRelightNotImplemented;
    return Status::Ok;
}

Status simulate_relative_world(double scene,const CounterfactualWorldSpec& w,const RelativeCaptureSpec& c,RelativeWorldPrediction& out) noexcept {
    out=RelativeWorldPrediction{};
    if(w.semantics!=WorldSemantics::RelativeRadianceScaleOnly || validate_world(w)!=Status::Ok || !nonnegative(scene) || !positive(c.shutterScale)) return Status::InvalidInput;
    const double worldSignal=scene*w.illuminationScale, exposure=worldSignal*c.shutterScale, totalScale=w.illuminationScale*c.shutterScale;
    if(!nonnegative(worldSignal)||!nonnegative(exposure)||!positive(totalScale)) return Status::InvalidInput;
    out.valid=true; out.sourceSceneSignal=scene; out.counterfactualSceneSignal=worldSignal; out.relativeExposureSignal=exposure;
    out.counterfactualDeltaEv=std::log2(totalScale); out.physicalSnrAvailable=false; out.sceneBinding=w.sceneBinding; out.ledger=ledger_one();
    out.claimBoundary="Relative counterfactual radiance/exposure only. No electron, physical SNR, ISO-noise, spectral relight, new shadow, or new evidence claim.";
    return Status::Ok;
}

Status validate_sensor_mode_calibration(const SensorModeCalibration& m) noexcept {
    if(m.authority==CalibrationAuthority::Unresolved) return Status::CalibrationMissing;
    if(m.calibrationId.empty()||m.sourceClassId.empty()||m.opticalModeId.empty()||m.sensorModeId.empty()||m.sceneScaleId.empty()||m.illuminationReferenceId.empty()) return Status::CalibrationMissing;
    if(m.authority==CalibrationAuthority::IndependentMeasurement && (m.calibrationProtocolId.empty()||!hex_sha256(m.calibrationEvidenceSha256))) return Status::CalibrationMissing;
    if(!positive(m.nominalIso)||!positive(m.sceneUnitToElectronsPerSecond)||!nonnegative(m.readNoiseElectronsRms)||!nonnegative(m.darkCurrentElectronsPerSecond)||!positive(m.fullWellElectrons)||!positive(m.systemGainDnPerElectron)||!finite(m.blackOffsetDn)||!finite(m.adcWhiteDn)||!(m.adcWhiteDn>m.blackOffsetDn)) return Status::CalibrationMissing;
    return Status::Ok;
}

Status predict_calibrated_capture(double scene,const CounterfactualWorldSpec& w,const PhysicalCaptureSpec& c,const SensorModeCalibration& m,SensorPrediction& out) noexcept {
    out=SensorPrediction{};
    if(w.semantics!=WorldSemantics::CalibratedNeutralIlluminationForward) return Status::InvalidInput;
    const auto ws=validate_world(w); if(ws!=Status::Ok)return ws; const auto ms=validate_sensor_mode_calibration(m); if(ms!=Status::Ok)return ms;
    if(!nonnegative(scene)||!positive(c.shutterSeconds)||c.expectedSourceClassId.empty()||c.expectedOpticalModeId.empty()) return Status::InvalidInput;
    if(c.expectedSourceClassId!=m.sourceClassId||c.expectedOpticalModeId!=m.opticalModeId||w.illuminationReferenceId!=m.illuminationReferenceId||w.sceneBinding.sceneScaleId!=m.sceneScaleId) return Status::BindingMismatch;
    const double signalE=scene*w.illuminationScale*m.sceneUnitToElectronsPerSecond*c.shutterSeconds;
    const double darkE=m.darkCurrentElectronsPerSecond*c.shutterSeconds, chargeE=signalE+darkE;
    const double varE=signalE+darkE+m.readNoiseElectronsRms*m.readNoiseElectronsRms;
    const double adcSatE=(m.adcWhiteDn-m.blackOffsetDn)/m.systemGainDnPerElectron, saturationE=std::min(m.fullWellElectrons,adcSatE);
    const double meanDn=m.blackOffsetDn+chargeE*m.systemGainDnPerElectron, varDn=varE*m.systemGainDnPerElectron*m.systemGainDnPerElectron;
    if(!nonnegative(signalE)||!nonnegative(darkE)||!nonnegative(chargeE)||!nonnegative(varE)||!positive(saturationE)||!finite(meanDn)||!nonnegative(varDn)) return Status::InvalidInput;
    out.valid=true; out.physicalForwardClaimAllowed=m.authority==CalibrationAuthority::IndependentMeasurement;
    out.signalElectrons=signalE; out.darkElectrons=darkE; out.expectedChargeElectrons=chargeE; out.temporalVarianceElectrons2=varE;
    out.electronDomainSnr=(signalE==0.0)?0.0:signalE/std::sqrt(varE); out.expectedDnBeforeClip=meanDn; out.temporalVarianceDn2=varDn;
    out.saturationChargeElectrons=saturationE; out.highCensored=chargeE>=saturationE;
    out.headroomStops=chargeE>0.0?std::log2(saturationE/chargeE):std::numeric_limits<double>::infinity(); out.nominalIso=m.nominalIso;
    out.calibrationId=m.calibrationId; out.calibrationEvidenceSha256=m.calibrationEvidenceSha256; out.sceneBinding=w.sceneBinding; out.ledger=ledger_one();
    out.claimBoundary=out.physicalForwardClaimAllowed?"Counterfactual expected-value temporal sensor prediction under caller-declared independently measured, hash-bound calibration. Simulated, never source evidence.":"Research-fixture counterfactual sensor prediction only. Numeric physics test, not a calibrated camera claim and not source evidence.";
    return Status::Ok;
}

Status choose_best_calibrated_capture(double scene,const CounterfactualWorldSpec& w,std::span<const CaptureCandidate> cs,const BestCapturePolicy& policy,BestCaptureResult& out) noexcept {
    out=BestCaptureResult{}; if(!nonnegative(scene)||!finite(policy.minimumHeadroomStops)||policy.minimumHeadroomStops<0||cs.empty())return Status::InvalidInput;
    bool found=false; double best=-1,bestShutter=std::numeric_limits<double>::infinity();
    for(std::size_t i=0;i<cs.size();++i){ SensorPrediction p; if(predict_calibrated_capture(scene,w,cs[i].capture,cs[i].calibration,p)!=Status::Ok)continue;
      if(!p.physicalForwardClaimAllowed||p.highCensored||p.headroomStops<policy.minimumHeadroomStops) continue;
      const double t=cs[i].capture.shutterSeconds;
      if(!found||p.electronDomainSnr>best||(p.electronDomainSnr==best&&t<bestShutter)){found=true;best=p.electronDomainSnr;bestShutter=t;out.valid=true;out.candidateIndex=i;out.prediction=p;}}
    return found?Status::Ok:Status::NoAdmissibleCapture;
}

Status assess_full_relight_readiness(const FullRelightStateAvailability& s) noexcept {
    if(!(s.geometry&&s.surfaceNormals&&s.visibility&&s.materialBrdf&&s.illuminantSpatialField&&s.illuminantSpectrum))return Status::RelightStateIncomplete;
    return Status::FullRelightNotImplemented;
}
} // namespace truthraw::counterfactual::v1
