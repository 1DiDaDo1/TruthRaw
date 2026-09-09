#!/usr/bin/env python3
from pathlib import Path
import hashlib,json,zlib
r=Path(__file__).resolve().parent
m=json.loads((r/'SHA256_MANIFEST_v0_8.json').read_text())
for n,v in m.items():
    b=(r/n).read_bytes(); h=hashlib.sha256(b).hexdigest()
    if h!=v['sha256'] or len(b)!=v['bytes']:
        raise SystemExit(f'MANIFEST_FAIL {n}')
s=json.loads((r/'STATE_v0_8.json').read_text())
if s.get('topologyCertified') is not False:
    raise SystemExit('OVERCLAIM topologyCertified')
rep=(r/'REPORT_v0_8.md').read_text()
for needle in ['necessary-condition topology proxy','topologyCertified` remains `false`','co-sited topology validation']:
    if needle not in rep:
        raise SystemExit('CLAIM_GUARD_FAIL '+needle)
res=json.loads(zlib.decompress((r/'TOPOLOGY_HELDOUT_RESULTS_v0_8.json.zlib').read_bytes()))
if 'cannot set topologyCertified=true' not in res.get('claim_boundary',''):
    raise SystemExit('RESULT_CLAIM_BOUNDARY_FAIL')
if sum(x['samples'] for x in res['summary'].values())!=2160000:
    raise SystemExit('SAMPLE_COUNT_FAIL')
q=json.loads((r/'DOG_3RAW_PHASE_DIVERSITY_QUALIFICATION_v0_8.json').read_text())
if q.get('status')!='FAIL_NOT_SUITABLE_FOR_CO_SITED_TOPOLOGY_CERTIFICATION':
    raise SystemExit('PHASE_DIVERSITY_QUALIFICATION_GUARD_FAIL')
if 'topologyCertified remains false' not in q.get('claim_boundary',''):
    raise SystemExit('PHASE_DIVERSITY_CLAIM_BOUNDARY_FAIL')
print('TRUTHRAW_TOPOLOGY_V0_8_INTEGRITY PASS')
