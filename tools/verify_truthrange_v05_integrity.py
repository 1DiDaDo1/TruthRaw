#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, zlib
ROOT=Path(__file__).resolve().parents[1]
MOD=ROOT/'docs/research/truthrange-real-latent-v0.5'
MAN=MOD/'SHA256_MANIFEST_v0_5.json'
STATE=ROOT/'state/TRUTHRANGE_V0_5_RESEARCH_STATE.json'
EXPECTED_MANIFEST_SHA256='256bed131a5c576ab852f9a4ecd15972b50c728d35aa2792179217656f29fa52'
EXPECTED_STATE_SHA256='b4619bb1f48e44d9daf8cd0a9ca59a6f524745adacaeb0ac1ff88f80701f6636'
EXPECTED_EVIDENCE_RAW_SHA256='05e6b6b58e9c54b8539d6b7b01dc9b5567ae3ebddb0a0a123ccde9fb6764e361'
EXPECTED_EVIDENCE_RAW_BYTES=18882
EXPECTED_FEATURE_SCHEMA='8c8e56b762b83a5a846c5201ab3ba43c9a2549d896873cc0ceb66574e74a83e3'

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
if sha(MAN)!=EXPECTED_MANIFEST_SHA256: raise SystemExit('FAIL v0.5 manifest sha256')
manifest=json.loads(MAN.read_text())
if len(manifest.get('files',[]))!=11: raise SystemExit('FAIL v0.5 manifest entry count')
for rec in manifest['files']:
    p=MOD/rec['file']
    if not p.is_file(): raise SystemExit('FAIL missing '+rec['file'])
    if p.stat().st_size!=rec['bytes'] or sha(p)!=rec['sha256']: raise SystemExit('FAIL module '+rec['file'])
if sha(STATE)!=EXPECTED_STATE_SHA256: raise SystemExit('FAIL v0.5 research state sha256')
z=(MOD/'REAL_BNCAM_LATENT_TRUTHRANGE_RESULTS_v0_5.json.zlib').read_bytes()
raw=zlib.decompress(z)
if len(raw)!=EXPECTED_EVIDENCE_RAW_BYTES or hashlib.sha256(raw).hexdigest()!=EXPECTED_EVIDENCE_RAW_SHA256: raise SystemExit('FAIL v0.5 evidence roundtrip')
ev=json.loads(raw)
if ev.get('status')!='REAL_DNG_LATENT_DENSE_TRUTHRANGE_RESEARCH_PASS': raise SystemExit('FAIL v0.5 evidence status')
g=ev.get('gates',{})
required=['clean_werror_native_test','synthetic_anti_leak_exact','synthetic_historical_role_onehot_quirk_preserved','real_stage2_parity_exact_8_of_8','real_measured_cfa_reinjection_exact_8_of_8','independent_all_role_probe_parity_8_of_8','tile128_vs_tile256_exact_8_of_8','iso12800_fullfield8192_vs_tile256_exact']
if not all(g.get(k) is True for k in required): raise SystemExit('FAIL v0.5 evidence gate')
if g.get('independent_probe_total')!=64: raise SystemExit('FAIL v0.5 probe total')
if ev.get('bindings',{}).get('v5_0g_feature_schema_sha256')!=EXPECTED_FEATURE_SCHEMA: raise SystemExit('FAIL v0.5 feature schema')
print('TruthRaw TruthRange v0.5 dedicated integrity: PASS')
