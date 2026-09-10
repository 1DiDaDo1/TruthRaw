from pathlib import Path
import hashlib, json
root=Path(__file__).resolve().parents[1]
manifest=root/'MANIFEST_SHA256.txt'
if not manifest.is_file(): raise SystemExit('missing MANIFEST_SHA256.txt')
for line in manifest.read_text().splitlines():
    expected, rel=line.split('  ',1)
    p=root/rel
    if not p.is_file(): raise SystemExit(f'missing {rel}')
    got=hashlib.sha256(p.read_bytes()).hexdigest()
    if got!=expected: raise SystemExit(f'hash mismatch {rel}: {got} != {expected}')
s=json.loads((root/'state/STATE_v1.json').read_text())
assert s['physicalFrameCount']==1 and s['independentEvidenceCount']==1
assert s['counterfactualObservationsAreEvidence'] is False
assert s['scientificMasterModified'] is False and s['zeroLineModified'] is False
assert s['honorPhysicalForwardAllowed'] is False
assert s['fullRelightImplemented'] is False
print(json.dumps({'status':'CICM_V1_INTEGRITY_PASS','manifest_entries':len(manifest.read_text().splitlines())},indent=2))
