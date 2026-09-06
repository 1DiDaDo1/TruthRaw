#pragma once
#include "truthraw/ptc.h"
#include "truthraw/core.h"
namespace truthraw::ptc {
struct ExportBinding {
  Scope scope=Scope::FullPhysical;
  bool sourceHash=false, sourceClass=false, sourceIdentity=false;
  bool cfa=false, black=false, white=false, colorTransform=false;
  bool gainMapPresent=false;
  bool backendVersion=false, backendHash=false, backendBound=false;
  bool scientificMasterHash=false, sceneLinear=true, negativeEvidencePreserved=true, overrangePreserved=true;
  bool exportFormat=false, mediaPayloadHash=false, metadataEmbedded=false, copyrightEmbedded=false, hiddenExtraIsp=false;
  bool noiseModelBound=false, backendBoundUncertainty=false;
  bool perLensColorCalibrated=false, illuminantCalibrated=false, electronCalibrationBound=false, opticsCalibrationBound=false;
};
Evidence evidenceFromProcessResult(const truthraw::ProcessResult& pr,const ExportBinding& b);
}
