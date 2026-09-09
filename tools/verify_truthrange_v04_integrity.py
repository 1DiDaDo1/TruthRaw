#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, zlib
ROOT=Path(__file__).resolve().parents[1]
MOD=ROOT/'docs/research/truthrange-real-dng-stage2-v0.4'
MAN=MOD/'SHA256_MANIFEST_v0_4.json'
EXPECTED_MANIFEST_SHA256='c589c3bbf662a792254dc0238e5d8c122ca112a4c011258414945080ea1be943'
EXPECTED_STATE_SHA256='ce5d6833b77c15ac3755372bfed841b8a7adae85ba8afb49d96d93ee009682cf'
STATE=ROOT/'state/TRUTHRANGE_V0_4_RESEARCH_STATE.json'

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
if sha(MAN)!=EXPECTED_MANIFEST_SHA256: raise SystemExit('FAIL v0.4 manifest sha256')
manifest=json.loads(MAN.read_text())
for rec in manifest['files']:
    p=MOD/rec['file']
    if not p.is_file(): raise SystemExit('FAIL missing '+rec['file'])
    if p.stat().st_size!=rec['bytes'] or sha(p)!=rec['sha256']: raise SystemExit('FAIL module '+rec['file'])
if sha(STATE)!=EXPECTED_STATE_SHA256: raise SystemExit('FAIL v0.4 research state sha256')
z=(MOD/'REAL_BNCAM_STAGE2_RESULTS_v0_4.json.zlib').read_bytes()
raw=zlib.decompress(z)
if len(raw)!=52945 or hashlib.sha256(raw).hexdigest()!='3add20ca29045778ffedfdc9254f7dff88afd5ae6025bfa812c26023339caf0f':
    raise SystemExit('FAIL v0.4 real evidence roundtrip')
print('TruthRaw TruthRange v0.4 dedicated integrity: PASS')
