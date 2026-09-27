#!/usr/bin/env python3
from __future__ import annotations
import hashlib,json,sys
from pathlib import Path

EXPECTED_MODELS={"PINHOLE_BROWN_CONRADY","FISHEYE_KANNALA_BRANDT","OMNIDIRECTIONAL_MEI"}

def fail(errors):
    print("DRAW_GEOMETRY_RELATION_CAMPAIGN_V01_FAIL")
    for e in errors: print(e)
    raise SystemExit(1)

def digest(doc):
    x=dict(doc); x.pop("state_sha256",None)
    return hashlib.sha256(json.dumps(x,sort_keys=True,separators=(",",":"),ensure_ascii=False,allow_nan=False).encode()).hexdigest()

def main():
    if len(sys.argv)!=4:
        fail(["usage: validate_draw_geometry_relation_campaign_v01.py MAIN_ADMISSION.json ULTRAWIDE_ADMISSION.json CAMPAIGN.json"])
    try:
        main_adm=json.loads(Path(sys.argv[1]).read_text())
        wide_adm=json.loads(Path(sys.argv[2]).read_text())
        c=json.loads(Path(sys.argv[3]).read_text())
    except Exception as exc:
        fail(["json:"+str(exc)])

    errors=[]
    if c.get("schema")!="D.RAW/GeometryRelationCampaign/0.1": errors.append("schema")
    if c.get("status")!="PENDING_MATCHED_SCENE_CAPTURE": errors.append("status")
    if c.get("relation_axis")!="GEOMETRY": errors.append("axis")

    for label,adm in (("main",main_adm),("ultra_wide",wide_adm)):
        if adm.get("status")!="ADMITTED_SOURCE_LOCAL": errors.append(label+".admission")
        if adm.get("graph_relations")!=[]: errors.append(label+".existing_relations")
        if adm.get("fusion_admissions")!=[]: errors.append(label+".existing_fusion")

    refs=c.get("reference_observations") or {}
    ma=refs.get("main") or {}; wa=refs.get("ultra_wide") or {}
    if ma.get("physical_observation_id")!=main_adm.get("physical_observation_id"): errors.append("main.endpoint")
    if wa.get("physical_observation_id")!=wide_adm.get("physical_observation_id"): errors.append("wide.endpoint")
    if ma.get("source_local_admission_state_sha256")!=main_adm.get("state_sha256"): errors.append("main.admission_hash")
    if wa.get("source_local_admission_state_sha256")!=wide_adm.get("state_sha256"): errors.append("wide.admission_hash")
    if ma.get("lens_role")!="MAIN" or wa.get("lens_role")!="ULTRA_WIDE": errors.append("lens_roles")
    if ma.get("physical_observation_id")==wa.get("physical_observation_id"): errors.append("same_endpoint")

    rel=c.get("current_relation_record") or {}
    if rel.get("relation_id")!="DRAW_REL_GEOMETRY_MAIN_ULTRAWIDE_V0_1": errors.append("relation.id")
    if rel.get("observation_a")!=ma.get("physical_observation_id"): errors.append("relation.a")
    if rel.get("observation_b")!=wa.get("physical_observation_id"): errors.append("relation.b")
    if rel.get("axis")!="GEOMETRY" or rel.get("status")!="UNKNOWN": errors.append("relation.state")
    if rel.get("certificate_sha256") is not None: errors.append("relation.premature_certificate")
    cap=rel.get("capabilities") or {}
    for k in ("coordinate_transform_allowed","equality_allowed","fusion_allowed","calibration_transfer_allowed"):
        if cap.get(k) is not False: errors.append("relation.capability."+k)

    ev=c.get("current_evidence_assessment") or {}
    for k in ("existing_reference_observations_same_scene","existing_reference_observations_are_geometry_measurement_pair","reuse_existing_reference_images_for_geometry_fit_allowed"):
        if ev.get(k) is not False: errors.append("current_evidence."+k)
    if ev.get("matched_scene_capture_required") is not True: errors.append("current_evidence.capture_required")

    cp=c.get("capture_protocol") or {}
    for k in ("camera_system_rigidly_mounted","move_calibration_target_not_camera_system","static_target_between_main_ultrawide_pair","camera_system_motion_between_pair_forbidden","target_motion_between_pair_forbidden","source_local_admission_required_for_every_capture","geometry_altering_crop_rotate_rescale_before_measurement_forbidden","source_raster_coordinates_required","training_holdout_disjoint"):
        if cp.get(k) is not True: errors.append("capture."+k)
    if cp.get("training_pose_minimum",0)<12: errors.append("capture.training_count")
    if cp.get("holdout_pose_minimum",0)<4: errors.append("capture.holdout_count")
    needed={"CENTER","LEFT_EDGE","RIGHT_EDGE","TOP_EDGE","BOTTOM_EDGE","CORNERS","NEAR","MID","FAR","TILTED"}
    if not needed.issubset(set(cp.get("pose_coverage_required") or [])): errors.append("capture.coverage")
    fp=cp.get("focus_policy") or {}
    if fp.get("focus_state_must_be_recorded_or_explicit_unknown") is not True: errors.append("capture.focus_record")
    if fp.get("silent_focus_transfer_allowed") is not False: errors.append("capture.focus_transfer")
    if fp.get("certificate_validity_domain_must_cover_focus_state") is not True: errors.append("capture.focus_domain")

    target=c.get("metric_target") or {}
    if target.get("kind")!="INDEXED_PLANAR_METRIC_TARGET": errors.append("target.kind")
    for k in ("physical_spacing_measurement_required","target_geometry_identity_sha256_required","target_must_overlap_both_fovs_per_pair","feature_ids_must_be_unambiguous"):
        if target.get(k) is not True: errors.append("target."+k)

    mp=c.get("model_policy") or {}
    if set(mp.get("candidate_projection_models") or [])!=EXPECTED_MODELS: errors.append("models.candidates")
    for k in ("intrinsics_are_per_lens","distortion_is_per_lens","extrinsic_transform_is_between_lens_coordinate_frames","metric_translation_requires_metric_target","projection_model_frozen_before_final_holdout_scoring","model_selection_uses_training_only","final_admission_uses_disjoint_holdout","model_complexity_may_not_be_increased_after_holdout"):
        if mp.get(k) is not True: errors.append("models."+k)
    if mp.get("single_model_family_for_both_lenses_required") is not False: errors.append("models.same_family")
    if mp.get("transform_direction")!="MAIN_TO_ULTRA_WIDE": errors.append("models.direction")

    cert=c.get("certificate_requirements") or {}
    for k in ("target_geometry_sha256","training_capture_set_sha256","holdout_capture_set_sha256","intrinsics_main_sha256","intrinsics_ultrawide_sha256","distortion_main_sha256","distortion_ultrawide_sha256","extrinsics_main_to_ultrawide_sha256","residual_report_sha256","validity_domain_sha256","certificate_sha256"):
        if cert.get(k) is not None: errors.append("certificate.premature."+k)
    if cert.get("thresholds_frozen_before_final_holdout") is not False: errors.append("certificate.thresholds")
    if cert.get("all_required_hashes_must_be_known_before_admission") is not True: errors.append("certificate.all_hashes")

    gate=c.get("admission_gate") or {}
    expected_gate={"calibration_capture_success_alone_admits_relation":False,"exact_endpoint_applicability_required":True,"held_out_validation_pass_required":True,"geometry_relation_status_after_success":"ADMITTED","coordinate_transform_allowed_after_admission":True,"equality_allowed_after_admission":False,"fusion_allowed_by_relation":False,"calibration_transfer_allowed_by_relation":False,"other_relation_axes_unchanged":True}
    for k,v in expected_gate.items():
        if gate.get(k)!=v: errors.append("gate."+k)

    for k,v in (c.get("separate_axes_preserved") or {}).items():
        if v is not False: errors.append("separate_axis."+k)

    inv=c.get("invariants") or {}
    expected_inv={"source_evidence_mutated":False,"observation_identity_mutated":False,"creates_new_evidence":False,"scientific_writeback_allowed":False,"appearance_writeback_allowed":False,"relation_promotes_source_evidence":False,"geometry_relation_implies_other_axes":False,"direct_fusion_granted":False}
    for k,v in expected_inv.items():
        if inv.get(k)!=v: errors.append("invariant."+k)

    if c.get("state_sha256")!=digest(c): errors.append("state_sha256:"+digest(c))
    if errors: fail(errors)

    print("DRAW_GEOMETRY_RELATION_CAMPAIGN_V01_PASS")
    print("status=PENDING_MATCHED_SCENE_CAPTURE")
    print("relation_axis=GEOMETRY")
    print("relation_status=UNKNOWN")
    print("coordinate_transform_allowed=false")
    print("fusion_allowed=false")
    print("creates_new_evidence=false")

if __name__=="__main__": main()
