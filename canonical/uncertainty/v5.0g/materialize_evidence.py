#!/usr/bin/env python3
from pathlib import Path
import argparse, hashlib, json, zlib
HERE=Path(__file__).resolve().parent
P=json.loads((HERE/"EVIDENCE_PROVENANCE_v5_0g.json").read_text())
def h(b): return hashlib.sha256(b).hexdigest()
def build(e):
 c=[]
 for p in e["parts"]:
  b=(HERE/p["file"]).read_bytes()
  if len(b)!=p["bytes"] or h(b)!=p["sha256"]: raise SystemExit(f"FAIL part {p['file']}")
  c.append(b)
 b=b"".join(c)
 if e["encoding"]=="zlib_concat":
  if len(b)!=e["compressed_bytes"] or h(b)!=e["compressed_sha256"]: raise SystemExit("FAIL compressed payload")
  b=zlib.decompress(b)
 elif e["encoding"]!="raw_concat": raise SystemExit("FAIL unknown encoding")
 if len(b)!=e["bytes"] or h(b)!=e["sha256"]: raise SystemExit(f"FAIL {e['canonical_filename']}")
 return b
def main():
 ap=argparse.ArgumentParser(); ap.add_argument("names",nargs="*"); ap.add_argument("--output-dir",default="materialized-evidence"); a=ap.parse_args()
 d={e["canonical_filename"]:e for e in P["reconstructed_files"]}; names=a.names or sorted(d); out=HERE/a.output_dir; out.mkdir(parents=True,exist_ok=True)
 for n in names:
  if n not in d: raise SystemExit(f"Unknown evidence: {n}")
  b=build(d[n]); (out/n).write_bytes(b); print(f"PASS {n} bytes={len(b)} sha256={h(b)}")
if __name__=="__main__": main()
