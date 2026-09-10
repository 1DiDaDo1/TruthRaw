#!/usr/bin/env python3
from __future__ import annotations
import hashlib, json, pathlib, sys
R=pathlib.Path(__file__).resolve().parents[1]
MAN=R/'SHA256_MANIFEST_v0_9.json'

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

def fail(m): print('TRUTHRAW_VIRTUAL_OBSERVATION_V0_9_INTEGRITY FAIL:',m); sys.exit(1)

m=json.loads(MAN.read_text(encoding='utf-8'))
for rel,expected in m['files'].items():
    p=R/rel
    if not p.is_file(): fail(f'missing {rel}')
    got=sha(p)
    if got!=expected: fail(f'hash mismatch {rel}: {got} != {expected}')
for rel,expected in m.get('architecture_files',{}).items():
    p=(R/rel).resolve()
    if not p.is_file(): fail(f'missing architecture file {rel}')
    got=sha(p)
    if got!=expected: fail(f'architecture hash mismatch {rel}: {got} != {expected}')

h=(R/'native/virtual_observation_manifold_v0_9.h').read_text(encoding='utf-8')
c=(R/'native/virtual_observation_manifold_v0_9.cpp').read_text(encoding='utf-8')
readme=(R/'README.md').read_text(encoding='utf-8')
state=json.loads((R/'state/VIRTUAL_OBSERVATION_MANIFOLD_V0_9_STATE.json').read_text())
real=json.loads((R/'REAL_3RAW_VIRTUAL_MANIFOLD_VALIDATION_v0_9.json').read_text())

required=[
 ('physicalFrameCount = 1',c),('independentEvidenceCount = 1',c),
 ('viewsAreIndependentMeasurements = false',c),('evidenceConfidenceMultiplier = 1.0',c),
 ('T_view = T_scene + e',readme),('GAIN_ENCODING_ONLY',readme),
 ('CALIBRATED_FORWARD_MODEL',readme),('do not create photons',readme),
 ('scale_pixel_covariance_v0_6',c),('spec.exposureEv',c)
]
for needle,text in required:
    if needle not in text: fail(f'claim/implementation guard missing: {needle}')

if state['physical_frame_count']!=1 or state['independent_evidence_count']!=1: fail('state evidence count changed')
if state['virtual_views_are_independent_measurements'] is not False: fail('state claims virtual independence')
if state['evidence_confidence_multiplier']!=1.0: fail('state confidence multiplier changed')
if state['physical_iso_noise_without_bound_sensor_model']!='REJECT': fail('physical ISO fail-closed gate missing')
if real['aggregate']['virtual_ev_views_total']!=27 or real['aggregate']['virtual_iso_encoding_views_total']!=24: fail('real evidence view counts changed')
if real['aggregate']['physical_frames_total']!=3 or real['aggregate']['independent_evidence_roots_total']!=3: fail('real evidence roots inflated')
if real['aggregate']['max_ev_roundtrip_abs']!=0.0: fail('real EV roundtrip no longer exact')
if real['aggregate']['max_weight_sum_abs_error']>1e-12: fail('EV weights do not normalize')

for forbidden in ['sigma/sqrt(views.size())','independentEvidenceCount = views.size()','physicalFrameCount = views.size()']:
    if forbidden in c: fail(f'forbidden evidence multiplication pattern: {forbidden}')

print('TRUTHRAW_VIRTUAL_OBSERVATION_V0_9_INTEGRITY PASS')
