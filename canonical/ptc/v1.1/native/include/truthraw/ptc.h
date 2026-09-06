#pragma once
#include <string>
#include <vector>
namespace truthraw::ptc {
enum class Scope { CoreIntegrity, FullPhysical };
enum class Status { PureTruthCertified, PureTruthDerived, NotCertified };
struct Evidence {
  Scope scope=Scope::FullPhysical;
  bool sourceHash=false, sourceClass=false, singleFrame=true, multiFrameEvidence=false;
  bool cfa=false, black=false, white=false, colorTransform=false, sourceIdentity=false;
  bool gainMapPresent=false; int gainMapApplicationCount=0;
  bool backendName=false, backendVersion=false, backendHash=false, backendBound=false;
  bool generatedSceneContent=false, semanticDetailGeneration=false, measuredSamplesPreserved=true;
  bool clippingCensoringPreserved=true, inventedClippedDetail=false;
  bool masterHash=false, sceneLinear=true, appearanceSeparated=true, negativeEvidencePreserved=true, overrangePreserved=true;
  bool generatedDetail=false;
  bool exportFormat=false, mediaPayloadHash=false, hiddenExtraIsp=false, metadataEmbedded=false, copyrightEmbedded=false;
  bool noiseModelBound=false, backendBoundUncertainty=false;
  bool perLensColorCalibrated=false, illuminantCalibrated=false, electronCalibrationBound=false, opticsCalibrationBound=false;
  bool claimColorimetric=false, claimElectron=false, claimOptics=false, claimCalibratedUncertainty=false;
};
struct Result { Status status=Status::NotCertified; std::vector<std::string> hardFailures; std::vector<std::string> completionBlockers; };
Result evaluate(const Evidence& e);
const char* statusName(Status s);
}
