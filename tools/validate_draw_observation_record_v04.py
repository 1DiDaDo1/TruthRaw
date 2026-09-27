#!/usr/bin/env python3
from __future__ import annotations
import hashlib,json,re,struct,subprocess,sys,tempfile
from pathlib import Path

HEX64=re.compile(r"^[0-9a-f]{64}$")
LENS={"MAIN":1,"ULTRA_WIDE":2,"TELEPHOTO":3,"FRONT":4,"EXTERNAL":5,"UNKNOWN":6}
TEMPORAL={"UNKNOWN":1,"CAPTURE_TIMESTAMP_ONLY":2,"EXPOSURE_INTERVAL":3,"GLOBAL_SHUTTER_INTERVAL":4,"ROLLING_SHUTTER_MODEL":5}

def fail(errors):
    print("DRAW_OBSERVATION_RECORD_V04_FAIL")
    for e in errors: print(e)
    raise SystemExit(1)

def canonical_hash(doc):
    x=dict(doc); x.pop("record_state_sha256",None)
    return hashlib.sha256(json.dumps(x,sort_keys=True,separators=(",",":"),ensure_ascii=False,allow_nan=False).encode()).hexdigest()

def hash_string(h,s):
    b=s.encode(); h.update(struct.pack("<Q",len(b))); h.update(b)

def graph_sha(obs_id,draw_sha,source_sha,lens_role,gauge_id,temporal,exposure_known,rolling_known,frames,evidence):
    h=hashlib.sha256(); h.update(b"D_RAW_FREE_WORLD_OBSERVATION_NODE_V0_1")
    hash_string(h,obs_id); h.update(bytes.fromhex(draw_sha)); h.update(bytes.fromhex(source_sha))
    h.update(bytes([LENS[lens_role]])); hash_string(h,gauge_id); h.update(bytes([TEMPORAL[temporal]]))
    h.update(bytes([1 if exposure_known else 0,1 if rolling_known else 0]))
    h.update(struct.pack("<I",frames)); h.update(struct.pack("<I",evidence))
    return h.hexdigest()

def main():
    if len(sys.argv)!=3: fail(["usage: validate_draw_observation_record_v04.py RECORD_V04.json LINEAGE.json"])
    try:
        r=json.loads(Path(sys.argv[1]).read_text())
        lineage=json.loads(Path(sys.argv[2]).read_text())
    except Exception as exc: fail(["json:"+str(exc)])
    errors=[]
    if r.get("schema")!="D.RAW/DRAWObservationRecord/0.4": errors.append("schema")
    if r.get("status")!="PHYSICAL_OBSERVATION_BOUND_TO_PIPELINE_LINEAGE": errors.append("status")

    pipeline=r.get("pipeline_record_v0_3") or {}
    validator=Path(__file__).with_name("validate_draw_observation_record_v03.py")
    with tempfile.NamedTemporaryFile("w",suffix=".json",delete=False) as f:
        json.dump(pipeline,f); tmp=f.name
    try:
        cp=subprocess.run([sys.executable,str(validator),tmp],capture_output=True,text=True)
        if cp.returncode!=0: errors.append("pipeline_v03_gate:"+(cp.stdout.strip().replace("\n","|") or cp.stderr.strip()))
    finally:
        Path(tmp).unlink(missing_ok=True)

    lp=lineage.get("physical_observation") or {}
    li=lineage.get("ingress_derivation") or {}
    ll=lineage.get("pipeline_lineage") or {}
    phys=r.get("physical_source_evidence") or {}
    prov=r.get("physical_provenance") or {}
    bind=r.get("scientific_ingress_lineage") or {}

    if r.get("physical_observation_id")!=lp.get("physical_observation_id"): errors.append("physical_observation_id")
    if phys.get("sha256")!=lp.get("parent_source_sha256") or phys.get("sealed") is not True: errors.append("physical_source")
    if prov.get("pre_admission_manifest_state_sha256")!=lp.get("pre_admission_manifest_state_sha256"): errors.append("provenance.pre")
    if prov.get("orientation_quarantine_manifest_sha256")!=li.get("manifest_sha256"): errors.append("provenance.quarantine")
    if prov.get("scientific_ingress_lineage_binding_state_sha256")!=lineage.get("binding_state_sha256"): errors.append("provenance.lineage")
    if prov.get("serialized_cfa_payload_sha256")!=li.get("serialized_cfa_payload_sha256"): errors.append("provenance.cfa")

    if bind.get("schema")!="D.RAW/ScientificIngressLineageBinding/0.1": errors.append("lineage.schema")
    if bind.get("binding_state_sha256")!=lineage.get("binding_state_sha256"): errors.append("lineage.state")
    if bind.get("pipeline_observation_id")!=ll.get("pipeline_observation_id"): errors.append("lineage.pipeline_obs")
    if bind.get("pipeline_source_sha256")!=ll.get("pipeline_input_sha256"): errors.append("lineage.pipeline_source")
    if bind.get("same_physical_observation") is not True: errors.append("lineage.same_physical")

    if pipeline.get("observation_id")!=ll.get("pipeline_observation_id"): errors.append("pipeline.obs")
    if (pipeline.get("source_evidence") or {}).get("sha256")!=ll.get("pipeline_input_sha256"): errors.append("pipeline.source")
    if (pipeline.get("scientific_master") or {}).get("sha256")!=ll.get("scientific_master_sha256"): errors.append("pipeline.master")
    if (pipeline.get("authority_field") or {}).get("sha256")!=ll.get("authority_field_sha256"): errors.append("pipeline.authority")
    if (pipeline.get("legacy_truthnegative_parent") or {}).get("state_sha256")!=ll.get("truthnegative_state_sha256"): errors.append("pipeline.truthnegative")
    if (pipeline.get("drawnegative") or {}).get("state_sha256")!=ll.get("drawnegative_state_sha256"): errors.append("pipeline.drawnegative")
    if (pipeline.get("gauge") or {}).get("scale_gauge_id")!=ll.get("scale_gauge_id"): errors.append("pipeline.gauge")

    if r.get("physical_observation_id")==pipeline.get("observation_id"): errors.append("identity_roles_collapsed")
    if phys.get("sha256")==((pipeline.get("source_evidence") or {}).get("sha256")): errors.append("derived_route_expected_distinct_source")

    counts=r.get("evidence_counts") or {}
    if counts!={"physical_frame_count":1,"independent_evidence_count":1}: errors.append("counts")
    if pipeline.get("evidence_counts")!=counts: errors.append("pipeline_counts")

    node=r.get("physical_graph_node") or {}
    tf=pipeline.get("temporal_footprint") or {}
    try:
        expected=graph_sha(
          r["physical_observation_id"],pipeline["drawnegative"]["state_sha256"],phys["sha256"],
          pipeline["procedure"]["lens_role"],pipeline["gauge"]["scale_gauge_id"],
          tf["model"],tf["exposure_interval_known"],tf["rolling_shutter_model_known"],1,1)
        if node.get("state_sha256")!=expected: errors.append("physical_graph_node.digest:"+expected)
    except Exception as exc: errors.append("physical_graph_node.compute:"+str(exc))
    if node.get("schema")!="D.RAW/FreeWorldObservationGraphNative/0.1": errors.append("physical_graph_node.schema")
    if node.get("observation_id")!=r.get("physical_observation_id"): errors.append("physical_graph_node.obs")
    if node.get("source_evidence_sha256")!=phys.get("sha256"): errors.append("physical_graph_node.source")
    if node.get("drawnegative_state_sha256")!=(pipeline.get("drawnegative") or {}).get("state_sha256"): errors.append("physical_graph_node.drawnegative")
    if node.get("scale_gauge_id")!=(pipeline.get("gauge") or {}).get("scale_gauge_id"): errors.append("physical_graph_node.gauge")
    if node.get("creates_new_evidence") is not False or node.get("scientific_writeback_allowed") is not False: errors.append("physical_graph_node.authority")
    if node.get("state_sha256")==((pipeline.get("graph_node") or {}).get("state_sha256")): errors.append("physical_and_pipeline_graph_nodes_collapsed")

    policy=r.get("identity_policy") or {}
    for k,v in {
      "physical_observation_is_free_world_identity":True,
      "pipeline_observation_id_role":"D_RAW_NEGATIVE_V0_1_COMPUTATIONAL_LINEAGE",
      "pipeline_identity_may_change_with_compatibility_representation":True,
      "physical_identity_changes_with_compatibility_representation":False,
      "derived_ingress_is_new_physical_observation":False,
      "graph_relations_use_physical_observation_id":True
    }.items():
        if policy.get(k)!=v: errors.append("policy."+k)

    inv=r.get("invariants") or {}
    for k,v in {
      "physical_source_mutated":False,"derived_ingress_promotes_source_authority":False,
      "drawnegative_v0_1_mutated":False,"physical_frame_count_increment":0,
      "independent_evidence_count_increment":0,"creates_new_evidence":False,
      "scientific_writeback_allowed":False,"calibration_transfer_implied":False,
      "cross_observation_relation_granted":False,"cross_observation_fusion_granted":False
    }.items():
        if inv.get(k)!=v: errors.append("invariant."+k)

    if r.get("record_state_sha256")!=canonical_hash(r): errors.append("record_state_sha256:"+canonical_hash(r))
    if errors: fail(errors)
    print("DRAW_OBSERVATION_RECORD_V04_PASS")
    print("physical_observation_id="+r["physical_observation_id"])
    print("pipeline_observation_id="+pipeline["observation_id"])
    print("physical_graph_node_sha256="+node["state_sha256"])
    print("pipeline_graph_node_sha256="+pipeline["graph_node"]["state_sha256"])
    print("creates_new_evidence=false")

if __name__=="__main__": main()
