#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, sys

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[2]
errors=[]
manifest=HERE/'MANIFEST_SHA256.txt'
if not manifest.exists(): errors.append('missing manifest')
else:
    for line in manifest.read_text().splitlines():
        if not line.strip(): continue
        want, rel = line.split('  ',1)
        p=HERE/rel
        if not p.exists(): errors.append(f'missing:{rel}'); continue
        got=hashlib.sha256(p.read_bytes()).hexdigest()
        if got!=want: errors.append(f'hash:{rel}:{got}:{want}')

for forbidden in ('local_stub','__pycache__'):
    if (HERE/forbidden).exists(): errors.append(f'forbidden:{forbidden}')

state=json.loads((HERE/'state/STATE_v0_1.json').read_text())
if state['scientific']['physicalFrameCount']!=1: errors.append('physicalFrameCount')
if state['scientific']['independentEvidenceCount']!=1: errors.append('independentEvidenceCount')
if state['canonical_v4_7i_modified']: errors.append('canonical modified')
if state['full_frame_streaming_v0_1_modified']: errors.append('streaming modified')
if state['memory_contract']['full_file_materialized']: errors.append('full file materialized')
if state['memory_contract']['full_raw_materialized']: errors.append('full raw materialized')
if state['memory_contract']['strile_arrays_materialized']: errors.append('strile arrays materialized')

# Upstream Git blob bindings. These are the exact files the adapter ABI/CI uses.
expected={
 'canonical/reconstruction/v4.7i/native/include/truthraw/core.h':'cfb9fd42bc310ddb4fd16ee8f26c3ed554a0f92a',
 'canonical/reconstruction/v4.7i/native/src/core.cpp':'f79b951ba54cff08db400023e528e4eb91909ba6',
 'docs/research/full-frame-streaming-v0.1/native/full_frame_streaming_v0_1.h':'79bd42c18918ea524220d94646d46bbe4e4040b1',
 'docs/research/full-frame-streaming-v0.1/native/full_frame_streaming_v0_1.cpp':'3335eb85a48c930e5a7dcb08c73975c1ed0ae50e',
 'docs/research/full-frame-streaming-v0.1/native/full_frame_streaming_v0_1_plan.cpp':'e6be34cac89512ff4c9551daeed440363137c83f',
 'docs/research/full-frame-streaming-v0.1/native/full_frame_streaming_v0_1_pass1.cpp':'17dc2daaad47cf7b9eaf5feadeaba452b0669ace',
 'docs/research/full-frame-streaming-v0.1/native/full_frame_streaming_v0_1_pass2.cpp':'1c5a3a819bcabb5c9427265695573f7255b42a05'
}
# Verify git blob SHA directly from bytes, avoiding dependence on git CLI history depth.
def git_blob_sha(p):
    b=p.read_bytes(); return hashlib.sha1(f'blob {len(b)}\0'.encode()+b).hexdigest()
for rel,want in expected.items():
    p=ROOT/rel
    if not p.exists(): errors.append(f'upstream_missing:{rel}')
    elif git_blob_sha(p)!=want: errors.append(f'upstream_drift:{rel}:{git_blob_sha(p)}:{want}')

# Root workflow is outside module manifest and is independently SHA-bound.
workflow=ROOT/'.github/workflows/tile-native-dng-source-v0-1-integrity.yml'
EXPECTED_WORKFLOW_SHA256='9ddb5270fa4d1ad6d8dcefda52ebfc45cf20be5b4d68f42dbbaa5c3be757dda7'
if not workflow.exists(): errors.append('workflow_missing')
elif hashlib.sha256(workflow.read_bytes()).hexdigest()!=EXPECTED_WORKFLOW_SHA256: errors.append('workflow_hash')

if errors:
    print('TILE_NATIVE_DNG_SOURCE_V0_1_INTEGRITY_FAIL')
    for e in errors: print(e)
    sys.exit(1)
print('TILE_NATIVE_DNG_SOURCE_V0_1_INTEGRITY_PASS')
print('canonical_v4_7i_bytes_bound=true')
print('full_frame_streaming_v0_1_bytes_bound=true')
print('full_file_materialized=false')
print('full_raw_materialized=false')
print('strile_arrays_materialized=false')
