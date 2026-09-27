#!/usr/bin/env python3
from __future__ import annotations
import hashlib,json,sys,subprocess
from pathlib import Path

CAP_KEYS={"sampling_geometry","geometry_pose","radiometry","colorimetry","spectral","optical_support","noise_uncertainty","temporal","provenance"}

def fail(errors):
    print("DRAW_SOURCE_CAPABILITY_ENVELOPE_V02_FAIL")
    for e in errors: print(e)
    raise SystemExit(1)

def digest(doc):
    x=dict(doc); x.pop("state_sha256",None)
    return hashlib.sha256(json.dumps(x,sort_keys=True,separators=(",",":"),ensure_ascii=False,allow_nan=False).encode()).hexdigest()

def main():
    if len(sys.argv)!=5:
        fail(["usage: validate_draw_source_capability_envelope_v02.py RECORD_V04.json PRE_V02.json LINEAGE.json ENVELOPE_V02.json"])
    try:
        record=json.loads(Path(sys.argv[1]).read_text())
        pre=json.loads(Path(sys.argv[2]).read_text())
        lineage=json.loads(Path(sys.argv[3]).read_text())
        env=json.loads(Path(sys.argv[4]).read_text())
    except Exception as exc: fail(["json:"+str(exc)])
    errors=[]

    cp=subprocess.run([sys.executable,str(Path(__file__).with_name("validate_draw_observation_record_v04.py")),sys.argv[1],sys.argv[3]],capture_output=True,text=True)
    if cp.returncode!=0: errors.append("record_v04_gate:"+(cp.stdout.strip().replace("\n","|") or cp.stderr.strip()))

    if env.get("schema")!="D.RAW/SourceCapabilityEnvelope/0.2": errors.append("schema")
    if env.get("status")!="PHYSICAL_SOURCE_KNOWLEDGE_MAP": errors.append("status")
    if env.get("observation_record_schema")!="D.RAW/DRAWObservationRecord/0.4": errors.append("record_schema")
    if env.get("observation_record_state_sha256")!=record.get("record_state_sha256"): errors.append("record_state")
    if env.get("physical_observation_id")!=record.get("physical_observation_id"): errors.append("physical_observation_id")
    if env.get("physical_source_evidence_sha256")!=(record.get("physical_source_evidence") or {}).get("sha256"): errors.append("physical_source")
    if env.get("physical_graph_node_sha256")!=(record.get("physical_graph_node") or {}).get("state_sha256"): errors.append("physical_graph_node")
    if env.get("scientific_ingress_lineage_binding_state_sha256")!=lineage.get("binding_state_sha256"): errors.append("lineage_state")

    p=record.get("pipeline_record_v0_3") or {}
    pl=env.get("pipeline_lineage") or {}
    if pl.get("pipeline_observation_id")!=p.get("observation_id"): errors.append("pipeline.observation_id")
    if pl.get("pipeline_source_sha256")!=(p.get("source_evidence") or {}).get("sha256"): errors.append("pipeline.source")
    if pl.get("drawnegative_state_sha256")!=(p.get("drawnegative") or {}).get("state_sha256"): errors.append("pipeline.drawnegative")
    if pl.get("scientific_master_sha256")!=(p.get("scientific_master") or {}).get("sha256"): errors.append("pipeline.master")

    inst=env.get("instrument") or {}; proc=p.get("procedure") or {}; pi=pre.get("instrument") or {}
    if inst.get("device_id")!=proc.get("device_id"): errors.append("instrument.device")
    if inst.get("camera_id")!=proc.get("camera_id"): errors.append("instrument.camera")
    if inst.get("lens_role")!=proc.get("lens_role"): errors.append("instrument.lens")
    if inst.get("camera_id")!="PHYSICAL_CAMERA_"+str(pi.get("active_physical_camera_id")): errors.append("instrument.pre_camera")

    sd=env.get("source_domains") or {}
    route=sd.get("physical_capture_route") or {}; pr=pre.get("source_route") or {}
    if route.get("route_id")!=pr.get("route_id") or route.get("authority")!=pr.get("authority"): errors.append("source_domain.route")
    if route.get("logical_camera_id")!=(pre.get("instrument") or {}).get("logical_camera_id"): errors.append("source_domain.logical")
    if route.get("active_physical_camera_id")!=(pre.get("instrument") or {}).get("active_physical_camera_id"): errors.append("source_domain.active_physical")
    if route.get("requested_physical_camera_id")!=pr.get("requested_physical_camera_id"): errors.append("source_domain.requested")

    storage=sd.get("serialized_storage_domain") or {}; ps=pre.get("serialized_storage_domain") or {}
    for k in ("authority","container","source_format","width","height","bits_per_sample","compression","cfa_pattern"):
        if storage.get(k)!=ps.get(k): errors.append("storage."+k)
    if storage.get("serialized_cfa_payload_sha256")!=(pre.get("serialized_cfa_payload") or {}).get("sha256"): errors.append("storage.payload")

    compat=sd.get("compatibility_ingress") or {}; li=lineage.get("ingress_derivation") or {}
    if compat.get("sha256")!=li.get("derived_ingress_sha256"): errors.append("compat.sha")
    if compat.get("container_role")!=li.get("derived_container_role"): errors.append("compat.role")
    if compat.get("lineage_binding_sha256")!=lineage.get("binding_state_sha256"): errors.append("compat.lineage")
    if compat.get("authority")!="DERIVED_LINEAGE_BOUND": errors.append("compat.authority")

    for name in ("capture_sample_domain","readout_domain","sensor_pixel_mode"):
        if sd.get(name)!=(pre.get(name) or {} if name!="sensor_pixel_mode" else pre.get(name) or {}):
            # v0.2 envelope intentionally copies only value+authority, not legacy blocker.
            eb=sd.get(name) or {}; pb=pre.get(name) or {}
            if eb.get("value")!=pb.get("value") or eb.get("authority")!=pb.get("authority"):
                errors.append("source_domain."+name)
        b=sd.get(name) or {}
        if b.get("authority")=="UNKNOWN" and b.get("value") is not None: errors.append("source_domain."+name+".invented")

    caps=env.get("capabilities") or {}
    if set(caps)!=CAP_KEYS: errors.append("capabilities.complete")
    dims=p.get("authority_dimensions") or {}
    for key in CAP_KEYS:
        c=caps.get(key) or {}
        if c.get("cross_observation_relation_admitted") is not False: errors.append("capability."+key+".relation")
        if c.get("implicit_transfer_allowed") is not False: errors.append("capability."+key+".transfer")
        if c.get("calibration_binding_sha256") is not None or c.get("validity_domain_sha256") is not None:
            errors.append("capability."+key+".unexpected_calibration")

    # Scene/scientific capability authority comes from the validated pipeline view.
    for key in ("sampling_geometry","geometry_pose","radiometry","colorimetry","spectral","optical_support","noise_uncertainty","temporal"):
        if (caps.get(key) or {}).get("authority")!=(dims.get(key) or {}).get("authority"):
            errors.append("capability."+key+".authority")

    if (caps.get("radiometry") or {}).get("identity_sha256")!=(p.get("scientific_master") or {}).get("sha256"): errors.append("capability.radiometry.identity")

    for key in ("spectral","optical_support","temporal"):
        c=caps.get(key) or {}
        if str(c.get("authority","")).startswith("UNKNOWN"):
            if c.get("identity_sha256") is not None or c.get("lineage_binding_sha256") is not None or c.get("knowledge_origin")!="UNKNOWN":
                errors.append("capability."+key+".unknown_claim")

    if (caps.get("sampling_geometry") or {}).get("knowledge_origin")!="PHYSICAL_SOURCE": errors.append("capability.sampling.origin")
    for key in ("geometry_pose","radiometry","noise_uncertainty"):
        if (caps.get(key) or {}).get("lineage_binding_sha256")!=lineage.get("binding_state_sha256"): errors.append("capability."+key+".lineage")
    if (caps.get("colorimetry") or {}).get("knowledge_origin")!="SOURCE_METADATA_LINEAGE_BOUND": errors.append("capability.color.origin")
    prov=caps.get("provenance") or {}
    if prov.get("authority")!="PHYSICAL_SOURCE_AND_INGRESS_LINEAGE_BOUND" or prov.get("identity_sha256")!=lineage.get("binding_state_sha256"): errors.append("capability.provenance")

    g=env.get("gauge") or {}; pg=p.get("gauge") or {}
    if g.get("truthrange_formula")!=pg.get("truthrange_formula") or g.get("scale_gauge_id")!=pg.get("scale_gauge_id"): errors.append("gauge.binding")
    if g.get("relation")!="SOURCE_LOCAL_ONLY" or pg.get("gauge_relation")!="SOURCE_LOCAL_ONLY": errors.append("gauge.local")
    if g.get("shared_free_world_gauge_id") is not None: errors.append("gauge.shared")

    if env.get("temporal_footprint")!=p.get("temporal_footprint"): errors.append("temporal_footprint")
    eo=env.get("optical_support") or {}; po=p.get("optical_support") or {}
    for k in ("authority","model_kind","support_identity_sha256"):
        if eo.get(k)!=po.get(k): errors.append("optical."+k)
    if env.get("calibration_bindings")!=[]: errors.append("calibrations")

    inv=env.get("invariants") or {}
    for k,v in {
      "unknown_is_valid_state":True,"missing_capability_placement_allowed":False,
      "physical_source_identity_replaced_by_pipeline_identity":False,
      "compatibility_ingress_is_new_observation":False,
      "lens_identity_upgrades_authority":False,"file_format_upgrades_authority":False,
      "implicit_calibration_transfer_allowed":False,"cross_observation_relation_granted":False,
      "cross_observation_fusion_granted":False,"creates_new_evidence":False,
      "scientific_writeback_allowed":False,"appearance_writeback_allowed":False
    }.items():
        if inv.get(k)!=v: errors.append("invariant."+k)

    if env.get("state_sha256")!=digest(env): errors.append("state_sha256:"+digest(env))
    if errors: fail(errors)
    print("DRAW_SOURCE_CAPABILITY_ENVELOPE_V02_PASS")
    print("physical_observation_id="+env["physical_observation_id"])
    print("state_sha256="+env["state_sha256"])
    print("capture_sample_domain=UNKNOWN")
    print("readout_domain=UNKNOWN")
    print("sensor_pixel_mode=UNKNOWN")
    print("creates_new_evidence=false")

if __name__=="__main__": main()
