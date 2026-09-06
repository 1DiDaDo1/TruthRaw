#include "mobile_v47i_ptc_adapter.h"
namespace truthraw::ptc {
Evidence evidenceFromProcessResult(const truthraw::ProcessResult& pr,const ExportBinding& b){
  Evidence e; e.scope=b.scope;
  e.sourceHash=b.sourceHash;e.sourceClass=b.sourceClass;e.sourceIdentity=b.sourceIdentity;
  e.cfa=b.cfa;e.black=b.black;e.white=b.white;e.colorTransform=b.colorTransform;
  e.gainMapPresent=b.gainMapPresent;e.gainMapApplicationCount=b.gainMapPresent?(pr.provenance.gainMapAppliedExactlyOnce?1:2):0;
  e.backendName=!pr.provenance.reconstructionBackend.empty();e.backendVersion=b.backendVersion;e.backendHash=b.backendHash;e.backendBound=b.backendBound;
  e.generatedSceneContent=false;e.semanticDetailGeneration=false;
  e.measuredSamplesPreserved=(pr.provenance.reconstructionBackend.find("measured_preserving")!=std::string::npos);
  e.clippingCensoringPreserved=!pr.provenance.censoredHighlightRecoveryClaimed;e.inventedClippedDetail=pr.provenance.censoredHighlightRecoveryClaimed;
  e.masterHash=b.scientificMasterHash;e.sceneLinear=b.sceneLinear;
  e.appearanceSeparated=!pr.provenance.scientificMasterModifiedByAppearance;
  e.negativeEvidencePreserved=b.negativeEvidencePreserved;e.overrangePreserved=b.overrangePreserved;
  e.generatedDetail=false;
  e.exportFormat=b.exportFormat;e.mediaPayloadHash=b.mediaPayloadHash;e.hiddenExtraIsp=b.hiddenExtraIsp;e.metadataEmbedded=b.metadataEmbedded;e.copyrightEmbedded=b.copyrightEmbedded;
  e.noiseModelBound=b.noiseModelBound;e.backendBoundUncertainty=b.backendBoundUncertainty;e.perLensColorCalibrated=b.perLensColorCalibrated;e.illuminantCalibrated=b.illuminantCalibrated;e.electronCalibrationBound=b.electronCalibrationBound;e.opticsCalibrationBound=b.opticsCalibrationBound;
  return e;
}
}
