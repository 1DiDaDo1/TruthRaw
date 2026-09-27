#!/usr/bin/env python3
from __future__ import annotations
import hashlib,json,subprocess,sys
from pathlib import Path

def fail(errors):
    print("DRAW_SOURCE_ADMISSION_PACKAGE_V02_FAIL")
    for e in errors: print(e)
    raise SystemExit(1)

def digest(doc):
    x=dict(doc); x.pop("state_sha256",None)
    return hashlib.sha256(json.dumps(x,sort_keys=True,separators=(",",":"),ensure_ascii=False,allow_nan=False).encode()).hexdigest()

def main():
    if len(sys.argv)!=6:
        fail(["usage: validate_draw_source_admission_package_v02.py RECORD_V04.json PRE_V02.json LINEAGE.json ENVELOPE_V02.json PACKAGE_V02.json"])
    try:
        record=json.loads(Path(sys.argv[1]).read_text())
        pre=json.loads(Path(sys.argv[2]).read_text())
        lineage=json.loads(Path(sys.argv[3]).read_text())
        env=json.loads(Path(sys.argv[4]).read_text())
        pkg=json.loads(Path(sys.argv[5]).read_text())
    except Exception as exc: fail(["json:"+str(exc)])
    errors=[]

    cp=subprocess.run([
      sys.executable,str(Path(__file__).with_name("validate_draw_source_capability_envelope_v02.py")),
      sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4]
    ],capture_output=True,text=True)
    if cp.returncode!=0: errors.append("envelope_v02_gate:"+(cp.stdout.strip().replace("\n","|") or cp.stderr.strip()))

    if pkg.get("schema")!="D.RAW/SourceAdmissionPackage/0.2": errors.append("schema")
    if pkg.get("status")!="ADMITTED_SOURCE_LOCAL": errors.append("status")
    if pkg.get("physical_observation_id")!=record.get("physical_observation_id") or pkg.get("physical_observation_id")!=env.get("physical_observation_id"): errors.append("physical_observation_id")
    if pkg.get("physical_source_evidence_sha256")!=(record.get("physical_source_evidence") or {}).get("sha256") or pkg.get("physical_source_evidence_sha256")!=env.get("physical_source_evidence_sha256"): errors.append("physical_source")

    pr=pkg.get("observation_record") or {}
    if pr.get("schema")!="D.RAW/DRAWObservationRecord/0.4": errors.append("record.schema")
    if pr.get("record_state_sha256")!=record.get("record_state_sha256"): errors.append("record.state")
    if pr.get("physical_graph_node_sha256")!=(record.get("physical_graph_node") or {}).get("state_sha256"): errors.append("record.graph")

    pe=pkg.get("capability_envelope") or {}
    if pe.get("schema")!="D.RAW/SourceCapabilityEnvelope/0.2": errors.append("envelope.schema")
    if pe.get("state_sha256")!=env.get("state_sha256"): errors.append("envelope.state")

    plb=pkg.get("scientific_ingress_lineage") or {}
    if plb.get("schema")!="D.RAW/ScientificIngressLineageBinding/0.1": errors.append("lineage.schema")
    if plb.get("binding_state_sha256")!=lineage.get("binding_state_sha256") or plb.get("binding_state_sha256")!=env.get("scientific_ingress_lineage_binding_state_sha256"): errors.append("lineage.state")

    pipeline=record.get("pipeline_record_v0_3") or {}
    ppl=pkg.get("pipeline_lineage") or {}
    if ppl.get("pipeline_observation_id")!=pipeline.get("observation_id"): errors.append("pipeline.obs")
    if ppl.get("pipeline_source_sha256")!=(pipeline.get("source_evidence") or {}).get("sha256"): errors.append("pipeline.source")
    if ppl.get("drawnegative_state_sha256")!=(pipeline.get("drawnegative") or {}).get("state_sha256"): errors.append("pipeline.drawnegative")

    g=pkg.get("gauge") or {}; rg=pipeline.get("gauge") or {}; eg=env.get("gauge") or {}
    if g.get("relation")!="SOURCE_LOCAL_ONLY" or rg.get("gauge_relation")!="SOURCE_LOCAL_ONLY" or eg.get("relation")!="SOURCE_LOCAL_ONLY": errors.append("gauge.local")
    if g.get("scale_gauge_id")!=rg.get("scale_gauge_id") or g.get("scale_gauge_id")!=eg.get("scale_gauge_id"): errors.append("gauge.id")
    if g.get("shared_free_world_gauge_id") is not None or rg.get("shared_free_world_gauge_id") is not None or eg.get("shared_free_world_gauge_id") is not None: errors.append("gauge.shared")

    counts=pkg.get("evidence_counts") or {}
    if counts!={"physical_frame_count":1,"independent_evidence_count":1}: errors.append("counts")
    if counts!=record.get("evidence_counts"): errors.append("record_counts")

    domains=pkg.get("source_domain_status") or {}; esd=env.get("source_domains") or {}
    for name in ("capture_sample_domain","readout_domain","sensor_pixel_mode"):
        if domains.get(name)!=esd.get(name): errors.append("source_domain."+name)
        b=domains.get(name) or {}
        if b.get("authority")!="UNKNOWN" or b.get("value") is not None: errors.append("source_domain."+name+".must_remain_unknown")

    if pkg.get("calibration_binding_count")!=0 or env.get("calibration_bindings")!=[]: errors.append("calibration_count")
    if pkg.get("graph_relations")!=[]: errors.append("graph_relations")
    if pkg.get("fusion_admissions")!=[]: errors.append("fusion_admissions")

    scope=pkg.get("admission_scope") or {}
    for k,v in {
      "physical_source_admitted":True,
      "scientific_pipeline_lineage_admitted":True,
      "free_world_physical_node_admitted":True,
      "cross_observation_authority_admitted":False
    }.items():
        if scope.get(k)!=v: errors.append("scope."+k)

    inv=pkg.get("invariants") or {}
    for k,v in {
      "physical_source_mutated":False,
      "compatibility_ingress_is_new_observation":False,
      "unknown_source_domains_resolved_by_admission":False,
      "drawnegative_v0_1_mutated":False,
      "calibration_transfer_implied":False,
      "cross_observation_relation_granted":False,
      "cross_observation_fusion_granted":False,
      "creates_new_evidence":False,
      "scientific_writeback_allowed":False,
      "appearance_writeback_allowed":False
    }.items():
        if inv.get(k)!=v: errors.append("invariant."+k)

    # Physical source remains the original pre-admitted source.
    if pkg.get("physical_source_evidence_sha256")!=(pre.get("source_evidence") or {}).get("sha256"): errors.append("pre_source")
    if pkg.get("physical_observation_id")!="DRAW_PHYSICAL_OBS_"+str(pkg.get("physical_source_evidence_sha256")): errors.append("physical_id_derivation")

    if pkg.get("state_sha256")!=digest(pkg): errors.append("state_sha256:"+digest(pkg))
    if errors: fail(errors)
    print("DRAW_SOURCE_ADMISSION_PACKAGE_V02_PASS")
    print("status=ADMITTED_SOURCE_LOCAL")
    print("physical_observation_id="+pkg["physical_observation_id"])
    print("physical_graph_node_sha256="+pkg["observation_record"]["physical_graph_node_sha256"])
    print("capture_sample_domain=UNKNOWN")
    print("readout_domain=UNKNOWN")
    print("sensor_pixel_mode=UNKNOWN")
    print("graph_relations=0")
    print("fusion_admissions=0")
    print("creates_new_evidence=false")

if __name__=="__main__": main()
