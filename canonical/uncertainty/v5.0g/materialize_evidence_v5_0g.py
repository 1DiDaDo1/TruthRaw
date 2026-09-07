#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, zlib, argparse
BASE=Path(__file__).resolve().parent
PROV=json.loads((BASE/'EVIDENCE_PROVENANCE_v5_0g.json').read_text())
def sha(b): return hashlib.sha256(b).hexdigest()
def build(rec):
    data=b''.join((BASE/p).read_bytes() for p in rec['parts'])
    if rec['encoding']=='zlib-concat':
        if len(data)!=rec['compressed_bytes'] or sha(data)!=rec['compressed_sha256']:
            raise SystemExit(f"compressed evidence mismatch: {rec['target']}")
        data=zlib.decompress(data)
    if len(data)!=rec['bytes'] or sha(data)!=rec['sha256']:
        raise SystemExit(f"evidence mismatch: {rec['target']}")
    return data
def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--write',action='store_true'); args=ap.parse_args()
    for rec in PROV['files']:
        data=build(rec); print(f"PASS {rec['target']} {len(data)} {sha(data)}")
        if args.write: (BASE/rec['target']).write_bytes(data)
if __name__=='__main__': main()
