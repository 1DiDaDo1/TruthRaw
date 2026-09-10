#!/usr/bin/env python3
from pathlib import Path
import json, subprocess
ROOT=Path(__file__).resolve().parents[1]
REPO=ROOT.parents[2]
bound={
'docs/research/manifold-conditioning-v1/native/manifold_conditioning_v1.cpp':'e41e8c6adbfdcd10c4afac6c4148a2bbfcb66ba0',
'docs/research/manifold-conditioning-v1/native/manifold_conditioning_v1.h':'da5def4b1f40300548782a123420e671349f5b6b',
'docs/research/uncertainty-aware-scurve-color-v1/native/uncertainty_aware_scurve_color_v1.cpp':'e23aa036343f0d205c4748bc3f70a4bf144430be',
'docs/research/uncertainty-aware-scurve-color-v1/native/uncertainty_aware_scurve_color_v1.h':'0a32d45f63e8de6d7df9b8a843ffcc3dd14e9439'}
for rel,expected in bound.items():
 p=REPO/rel
 got=subprocess.check_output(['git','-C',str(REPO),'hash-object',str(p)],text=True).strip()
 if got!=expected: raise SystemExit(f'upstream blob mismatch {rel}: {got} != {expected}')
state=json.loads((ROOT/'state'/'STATE_v1.json').read_text())
assert state['scientific_master_modified'] is False
assert state['best_conditioning_ev_used_as_appearance_control'] is False
assert state['physical_frame_count']==1 and state['independent_evidence_count']==1
assert state['legacy_multi_ev_reconstruction_candidate']=='REJECTED_NOT_PROMOTABLE'
syn=json.loads((ROOT/'evidence'/'SYNTHETIC_FALSIFICATION_METRICS_v1.json').read_text())
assert syn['flat_confidence_step_max_luma_gradient']==0.0
assert syn['ramp_monotonic'] is True
assert syn['edge_contrast_ratio_python_reference']>0.95
assert syn['gauge_invariance']['max_abs_snr_delta']==0.0
real=json.loads((ROOT/'evidence'/'REAL_094423_BRIDGE_APPEARANCE_PROBE_v1.json').read_text())
assert real['source']['sha256']=='ade9d84542916678d1a198c6baff219806d44fa8f050ef8d7545dfae90644f65'
assert real['scientific_master_modified'] is False
assert real['physical_noise_uncertainty_reduced_claim'] is False
assert real['virtual_ev_adds_evidence_claim'] is False
print('BEST_CONDITIONING_SCURVE_BRIDGE_V1_INTEGRITY_PASS')
