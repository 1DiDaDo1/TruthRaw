#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, zlib
ROOT=Path(__file__).resolve().parents[1]
def gitblob(p):
    b=p.read_bytes(); return hashlib.sha1(b'blob '+str(len(b)).encode()+b'\0'+b).hexdigest()
EXPECTED_BLOBS={
 'README.md':'cbc083c14eee370d3e2466e86c53296ae34c197f',
 'state/CURRENT_CANONICAL_STATE_2026-09-08.json':'13571f07246a12401d590757f54bcfe0c980d38b',
 'state/REPOSITORY_MIGRATION_STATUS.json':'b7d9a2ef63bcf1e824e90ddcf56d560126d87011',
 'docs/PROJECT_STATE_AUDIT_2026-09-08.md':'069220b82de4f50c42e214b4d9213e66e9423cc8',
 'canonical/reconstruction/v4.7i/native/include/truthraw/core.h':'cfb9fd42bc310ddb4fd16ee8f26c3ed554a0f92a',
 'canonical/reconstruction/v4.7i/native/src/core.cpp':'f79b951ba54cff08db400023e528e4eb91909ba6',
 'canonical/detail/v4.7j/native/include/truthraw/core.h':'ff5a6c976be8228d990aa4ee577db8613d8c9bd1',
 'canonical/detail/v4.7j/native/src/core.cpp':'4c818fed9644605693e93e35c88bedeee5360cef',
 'canonical/detail/v4.7j/VALIDATION_v4_7j.json':'73593dcf546cb4ec8da3209963ea4717c331f7fe',
 'canonical/output-acutance/v4.7k/STATUS_v4_7k.json':'4b1af55b6dd1e754c434838974b47084c94e6cf4',
 'canonical/output-acutance/v4.7k/VALIDATION_v4_7k.json':'1c67dfc25beca27b4fe1de4079b2a9e0d1e64310',
 'canonical/output-acutance/v4.7k/native/CMakeLists.txt':'ffb243278c361b5b71bef6ee3e548a6f2b518dfa',
 'canonical/output-acutance/v4.7k/native/output_acutance.cpp':'e0465dadd453427c904d5292abd763397075331d',
 'canonical/output-acutance/v4.7k/native/output_acutance.h':'e6982fb702ddb8b0f60c451a3b3315671afb823e',
 'canonical/ptc/v1.1/python/truthraw_ptc.py':'92f177f2292a4c194b698916c951d3c751766699',
 'canonical/uncertainty/v5.0g/source/uncertainty_core_v5_0g.py':'3a833147f892a970c60175a7ebe4ab1e8cf0221c',
 'canonical/uncertainty/v5.0g/source/prospective_holdout_v5_0g.py':'8f1f57f8d965b3a7067aed51ec60fe8586f196df',
 'canonical/uncertainty/v5.0g/source/finalize_uncertainty_v5_0g.py':'7832f3da0cb960f74f025f1c2c9fbd9f2c772a62',
 'canonical/uncertainty/v5.0g/source/fit_uncertainty_v5_0g.py':'0e3e8be37e890f5a3cf6b2e25ae9374e62aaa2b6',
 'canonical/uncertainty/v5.0g/source/train_uncertainty_v5_0g.py':'e9a9cf08c54637111bcfd0b6b8a29f0566e46075',
 'canonical/uncertainty/v5.0g/runtime/runtime_parity_test.cpp':'923f581612cb1632cc352659acd68d90dc9cec01',
}
for rel,exp in EXPECTED_BLOBS.items():
    p=ROOT/rel
    if not p.is_file() or gitblob(p)!=exp: raise SystemExit(f'FAIL blob {rel}')
    print('PASS blob',rel)
v5e=ROOT/'canonical/unified-material/v5.0e'
data=b''.join((v5e/'source-parts'/f'unified_material_v5_0e.py.part0{i}').read_bytes() for i in range(1,5))
if len(data)!=26771 or hashlib.sha256(data).hexdigest()!='7a60703c47a981da8aa4f9613545e1615a6a25cdbb3a8347ed1c1a4df076b1e0': raise SystemExit('FAIL v5.0e source')
print('PASS v5.0e source reconstruction')
u=ROOT/'canonical/uncertainty/v5.0g'; prov=json.loads((u/'EVIDENCE_PROVENANCE_v5_0g.json').read_text())
for rec in prov['files']:
    b=b''.join((u/x).read_bytes() for x in rec['parts'])
    if rec['encoding']=='zlib-concat':
        if len(b)!=rec['compressed_bytes'] or hashlib.sha256(b).hexdigest()!=rec['compressed_sha256']: raise SystemExit('FAIL compressed '+rec['target'])
        b=zlib.decompress(b)
    if len(b)!=rec['bytes'] or hashlib.sha256(b).hexdigest()!=rec['sha256']: raise SystemExit('FAIL evidence '+rec['target'])
    print('PASS evidence',rec['target'])
for p in ROOT.rglob('*'):
    if p.is_file() and p.suffix.lower() in {'.dng','.rawsensor','.raw10','.raw12','.apk','.npz'}:
        raise SystemExit('FAIL forbidden payload '+str(p.relative_to(ROOT)))
print('TruthRaw canonical integrity: PASS')
