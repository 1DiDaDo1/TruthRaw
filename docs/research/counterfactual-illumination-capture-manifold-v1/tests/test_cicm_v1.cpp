#include "cicm_v1.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace truthraw::counterfactual::v1;
static void req(bool x,const char* m){if(!x){std::cerr<<"FAIL: "<<m<<"\n";std::exit(2);}}
static bool near(double a,double b,double e=1e-11){return std::abs(a-b)<=e*std::max({1.0,std::abs(a),std::abs(b)});}
static SceneBinding bind(){return {"EVIDENCE","SCENE_SCALE","TRUTHRANGE_L0",std::string(64,'a')};}
static SensorModeCalibration mode(double iso,double k,CalibrationAuthority auth=CalibrationAuthority::IndependentMeasurement){SensorModeCalibration m;m.authority=auth;m.calibrationId="fixture-"+std::to_string((int)iso);m.calibrationProtocolId="EMVA1288_LIKE_FIXTURE";m.calibrationEvidenceSha256=std::string(64,'b');m.sourceClassId="SOURCE";m.opticalModeId="TELE_FIXED";m.sensorModeId="ISO"+std::to_string((int)iso);m.sceneScaleId="SCENE_SCALE";m.illuminationReferenceId="REF_NEUTRAL";m.nominalIso=iso;m.sceneUnitToElectronsPerSecond=1000;m.readNoiseElectronsRms=2;m.darkCurrentElectronsPerSecond=10;m.fullWellElectrons=1000;m.systemGainDnPerElectron=k;m.blackOffsetDn=64;m.adcWhiteDn=1023;return m;}
static PhysicalCaptureSpec cap(double t){return {"SOURCE","TELE_FIXED",t};}
int main(){
 CounterfactualWorldSpec rel{"REL",WorldSemantics::RelativeRadianceScaleOnly,4.0,"",bind()}; RelativeWorldPrediction rp;
 req(simulate_relative_world(.25,rel,{.5},rp)==Status::Ok,"relative"); const double relativeEv=rp.counterfactualDeltaEv;
 req(near(rp.counterfactualSceneSignal,1)&&near(rp.relativeExposureSignal,.5)&&near(relativeEv,1),"relative math");
 req(!rp.physicalSnrAvailable&&rp.ledger.physicalFrameCount==1&&rp.ledger.independentEvidenceCount==1&&!rp.ledger.counterfactualObservationsAreEvidence&&!rp.ledger.scientificMasterModified&&!rp.ledger.zeroLineModified,"relative ledger");
 req(rp.sceneBinding.zeroLineId=="TRUTHRANGE_L0","zero line retained"); RelativeWorldPrediction badout;auto bad=rel;bad.illuminationScale=-1;req(simulate_relative_world(.2,bad,{1},badout)==Status::InvalidInput,"negative light");
 auto badbind=rel;badbind.sceneBinding.scientificMasterSha256="not-a-hash";req(simulate_relative_world(.2,badbind,{1},badout)==Status::InvalidInput,"master hash binding");
 CounterfactualWorldSpec phy{"P",WorldSemantics::CalibratedNeutralIlluminationForward,4.0,"REF_NEUTRAL",bind()};auto m100=mode(100,1);SensorPrediction p1;
 req(predict_calibrated_capture(.25,phy,cap(.01),m100,p1)==Status::Ok,"physical fixture");req(near(p1.signalElectrons,10)&&near(p1.darkElectrons,.1)&&near(p1.temporalVarianceElectrons2,14.1),"noise math");req(near(p1.electronDomainSnr,10/std::sqrt(14.1)),"snr");req(p1.physicalForwardClaimAllowed,"independent authority");req(p1.calibrationEvidenceSha256==std::string(64,'b'),"calibration evidence hash retained");
 auto phy2=phy;phy2.illuminationScale=8;SensorPrediction p2;req(predict_calibrated_capture(.25,phy2,cap(.01),m100,p2)==Status::Ok,"double light");req(near(p2.signalElectrons,2*p1.signalElectrons)&&p2.electronDomainSnr>p1.electronDomainSnr,"light changes hypothetical SNR");SensorPrediction ps;req(predict_calibrated_capture(.25,phy,cap(.02),m100,ps)==Status::Ok&&near(ps.signalElectrons,2*p1.signalElectrons)&&ps.electronDomainSnr>p1.electronDomainSnr,"shutter changes hypothetical SNR");
 auto m400=mode(400,4);auto unit=phy;unit.illuminationScale=1;SensorPrediction g1,g4;req(predict_calibrated_capture(.1,unit,cap(.01),m100,g1)==Status::Ok&&predict_calibrated_capture(.1,unit,cap(.01),m400,g4)==Status::Ok,"gain pair");req(near(g1.electronDomainSnr,g4.electronDomainSnr),"gain alone no e-SNR gain");req(near(g4.expectedDnBeforeClip-m400.blackOffsetDn,4*(g1.expectedDnBeforeClip-m100.blackOffsetDn)),"gain DN scale");req(g4.saturationChargeElectrons<g1.saturationChargeElectrons,"ADC headroom");
 SensorPrediction z;req(predict_calibrated_capture(0,unit,cap(.01),m100,z)==Status::Ok&&z.electronDomainSnr==0,"zero signal SNR");
 auto unresolved=m100;unresolved.authority=CalibrationAuthority::Unresolved;SensorPrediction pu;req(predict_calibrated_capture(.1,unit,cap(.01),unresolved,pu)==Status::CalibrationMissing,"missing calibration");auto nohash=m100;nohash.calibrationEvidenceSha256.clear();req(predict_calibrated_capture(.1,unit,cap(.01),nohash,pu)==Status::CalibrationMissing,"missing independent evidence hash");
 auto research=mode(100,1,CalibrationAuthority::ExplicitResearchFixture);research.calibrationEvidenceSha256.clear();req(predict_calibrated_capture(.1,unit,cap(.01),research,pu)==Status::Ok&&!pu.physicalForwardClaimAllowed,"research fixture not physical");auto wrong=cap(.01);wrong.expectedOpticalModeId="MAIN";req(predict_calibrated_capture(.1,unit,wrong,m100,pu)==Status::BindingMismatch,"optical binding");auto wrongScale=m100;wrongScale.sceneScaleId="OTHER";req(predict_calibrated_capture(.1,unit,cap(.01),wrongScale,pu)==Status::BindingMismatch,"scene scale binding");
 SensorPrediction sat;req(predict_calibrated_capture(100,unit,cap(1),m100,sat)==Status::Ok&&sat.highCensored,"saturation censor");
 std::vector<CaptureCandidate> cands{{cap(.001),m100},{cap(.004),m100},{cap(.02),m100}};BestCaptureResult br;req(choose_best_calibrated_capture(.2,unit,cands,{.25},br)==Status::Ok&&br.candidateIndex==2,"best capture");std::vector<CaptureCandidate> onlyResearch{{cap(.01),research}};req(choose_best_calibrated_capture(.2,unit,onlyResearch,{0},br)==Status::NoAdmissibleCapture,"research best blocked");
 double prev=-1;int unc=0,cens=0;for(int i=0;i<1000;i++){auto w=unit;w.illuminationScale=std::exp2(-10.0+30.0*i/999.0);SensorPrediction p;req(predict_calibrated_capture(.02,w,cap(.01),m100,p)==Status::Ok,"sweep");if(!p.highCensored){req(p.electronDomainSnr+1e-14>=prev,"SNR monotone pre-censor");prev=p.electronDomainSnr;unc++;}else cens++;}req(unc>100&&cens>100,"sweep crosses censor");
 FullRelightStateAvailability fr;req(assess_full_relight_readiness(fr)==Status::RelightStateIncomplete,"relight missing");fr={true,true,true,true,true,true};req(assess_full_relight_readiness(fr)==Status::FullRelightNotImplemented,"relight not faked");
 std::cout<<"relative_delta_ev="<<relativeEv<<"\nfixture_snr="<<p1.electronDomainSnr<<"\ndouble_light_snr="<<p2.electronDomainSnr<<"\nuncensored_sweep_points="<<unc<<"\ncensored_sweep_points="<<cens<<"\nCICM_V1_PASS\n";
}
