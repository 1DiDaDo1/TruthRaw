from pathlib import Path
import hashlib,json,sys
root=Path(__file__).resolve().parents[1]
man=json.loads((root/'SHA256_MANIFEST_v1.json').read_text())
for rel,want in man['files'].items():
    p=root/rel
    if not p.exists(): raise SystemExit(f'MISSING {rel}')
    got=hashlib.sha256(p.read_bytes()).hexdigest()
    if got!=want: raise SystemExit(f'HASH_MISMATCH {rel} {got} != {want}')

for rel,want in man.get('architecture_files',{}).items():
    p=root/rel
    if not p.exists(): raise SystemExit(f'MISSING_ARCH {rel}')
    got=hashlib.sha256(p.read_bytes()).hexdigest()
    if got!=want: raise SystemExit(f'ARCH_HASH_MISMATCH {rel} {got} != {want}')
readme=(root/'README.md').read_text()
header=(root/'native/uncertainty_aware_scurve_color_v1.h').read_text()
cpp=(root/'native/uncertainty_aware_scurve_color_v1.cpp').read_text()
evidence=json.loads((root/'evidence/REAL_DOG_094423_v1.json').read_text())
state=json.loads((root/'state/STATE_v1.json').read_text())
assert 'APPEARANCE ONLY' in readme
assert 'scientificMasterModified' in header
assert 'out.status = AppearanceStatusV1::Applied' in cpp
assert evidence['scientific_master_modified'] is False
assert state['physical_color_claimed'] is False
assert state['spectral_truth_claimed'] is False
assert state['source_censored_positive_chroma_enhancement']=='FORBIDDEN'
assert 'uncertain chroma is restrained' in (root/'REPORT_v1.md').read_text()
print('TRUTHRAW_UNCERTAINTY_AWARE_SCURVE_COLOR_V1_INTEGRITY_PASS')
