#include "truthraw/ptc.h"
namespace truthraw::ptc {
const char* statusName(Status s){switch(s){case Status::PureTruthCertified:return "PURE_TRUTH_CERTIFIED";case Status::PureTruthDerived:return "PURE_TRUTH_DERIVED";default:return "NOT_CERTIFIED";}}
Result evaluate(const Evidence& e){
  Result r; auto req=[&](bool v,const char*n){if(!v)r.hardFailures.emplace_back(n);}; auto full=[&](bool v,const char*n){if(!v)r.completionBlockers.emplace_back(n);};
  req(e.sourceHash,"source.sha256"); req(e.sourceClass,"source.source_class"); req(e.singleFrame,"capture.single_frame"); req(!e.multiFrameEvidence,"capture.multi_frame_evidence_used");
  req(e.cfa,"admission.cfa_validated");req(e.black,"admission.blacklevel_validated");req(e.white,"admission.whitelevel_validated");req(e.colorTransform,"admission.color_transform_bound");req(e.sourceIdentity,"admission.source_identity_bound");
  req(e.gainMapPresent?e.gainMapApplicationCount==1:e.gainMapApplicationCount==0,"admission.gainmap_application_count");
  req(e.backendName,"reconstruction.backend");req(e.backendVersion,"reconstruction.backend_version");req(e.backendHash,"reconstruction.backend_sha256");req(e.backendBound,"reconstruction.backend_bound_to_output");
  req(!e.generatedSceneContent,"reconstruction.generated_scene_content");req(!e.semanticDetailGeneration,"reconstruction.semantic_detail_generation");req(e.measuredSamplesPreserved,"reconstruction.measured_samples_preserved_by_policy");
  req(e.clippingCensoringPreserved,"clipping.whitelevel_censoring_preserved");req(!e.inventedClippedDetail,"clipping.invented_clipped_detail");
  req(e.masterHash,"scientific_master.sha256");req(e.sceneLinear,"scientific_master.scene_linear");req(e.appearanceSeparated,"scientific_master.appearance_separated");req(e.negativeEvidencePreserved,"scientific_master.negative_evidence_preserved");req(e.overrangePreserved,"scientific_master.overrange_preserved");
  req(!e.generatedDetail,"appearance.generated_detail");req(e.exportFormat,"export.format");req(e.mediaPayloadHash,"export.media_payload_sha256");req(!e.hiddenExtraIsp,"export.hidden_extra_isp");req(e.metadataEmbedded,"export.ptc_metadata_embedded");req(e.copyrightEmbedded,"export.copyright_embedded");
  if(e.claimColorimetric)req(e.perLensColorCalibrated&&e.illuminantCalibrated,"claim.colorimetric_accuracy_requires_calibration");
  if(e.claimElectron)req(e.electronCalibrationBound,"claim.electron_truth_requires_calibration");
  if(e.claimOptics)req(e.opticsCalibrationBound,"claim.optical_restoration_requires_calibration");
  if(e.claimCalibratedUncertainty)req(e.backendBoundUncertainty,"claim.calibrated_uncertainty_requires_backend_binding");
  if(e.scope==Scope::FullPhysical){full(e.noiseModelBound,"full.noise_model_bound");full(e.backendBoundUncertainty,"full.backend_bound_uncertainty");full(e.perLensColorCalibrated,"full.per_lens_color_calibrated");full(e.illuminantCalibrated,"full.illuminant_calibrated");full(e.electronCalibrationBound,"full.electron_calibration_bound");full(e.opticsCalibrationBound,"full.optics_calibration_bound");}
  r.status=!r.hardFailures.empty()?Status::NotCertified:(!r.completionBlockers.empty()?Status::PureTruthDerived:Status::PureTruthCertified);return r;
}
}
