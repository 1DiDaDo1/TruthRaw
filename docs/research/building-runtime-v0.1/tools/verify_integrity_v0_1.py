#!/usr/bin/env python3
from pathlib import Path
import json, sys, hashlib
root = Path(__file__).resolve().parents[1]
required = [
    'CMakeLists.txt','README.md','FACT_CHECK_v0_1.md','REPORT_v0_1.md',
    'native/building_runtime_v0_1.h','native/building_runtime_v0_1.cpp',
    'tests/test_building_runtime_v0_1.cpp','state/STATE_v0_1.json',
    'evidence/TEST_METRICS_v0_1.txt','evidence/LOCAL_VALIDATION_v0_1.json'
]
missing=[p for p in required if not (root/p).is_file()]
if missing:
    print('MISSING', *missing, sep='\n'); sys.exit(1)
state=json.loads((root/'state/STATE_v0_1.json').read_text())
assert state['scientific_master_modified'] is False
assert state['physical_frame_count'] == 1
assert state['independent_evidence_count'] == 1
assert state['appearance_feedback_to_science'] is False
assert state['conditioning_ev_equals_appearance_ev'] is False
assert state['automatic_scientific_promotion'] is False
assert state['hot_path_heap_allocation'] is False
text='\n'.join((root/p).read_text(errors='strict') for p in required if p.endswith(('.md','.h','.cpp','.json')))
for needle in ['PhysicalCaptureEV', 'BestConditioningEV', 'AppearanceEV', 'independentEvidenceCount', 'scientificMasterModified']:
    if needle not in text:
        print('MISSING_CONTRACT', needle); sys.exit(1)
print('BUILDING_RUNTIME_V0_1_INTEGRITY_PASS')

manifest_path=root/'SHA256_MANIFEST_v0_1.json'
if manifest_path.is_file():
    manifest=json.loads(manifest_path.read_text())
    for rel, expected in manifest['files'].items():
        got=hashlib.sha256((root/rel).read_bytes()).hexdigest()
        if got != expected:
            print('HASH_MISMATCH', rel, expected, got); sys.exit(1)
    print('BUILDING_RUNTIME_V0_1_SHA256_MANIFEST_PASS')
