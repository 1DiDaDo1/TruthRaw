#!/usr/bin/env python3
from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,zlib
ROOT=Path(__file__).resolve().parents[1]; V5G=ROOT/"canonical/uncertainty/v5.0g"; V5E=ROOT/"canonical/unified-material/v5.0e"
def h(b): return hashlib.sha256(b).hexdigest()
def gh(b): return hashlib.sha1(b"blob "+str(len(b)).encode()+b"\0"+b).hexdigest()
def chk(p,n,s,g=None):
 if not p.is_file(): raise AssertionError(f"missing {p.relative_to(ROOT)}")
 b=p.read_bytes()
 if len(b)!=n or h(b)!=s or (g and gh(b)!=g): raise AssertionError(f"integrity mismatch {p.relative_to(ROOT)}")
 return b
def v5g():
 P=json.loads((V5G/"EVIDENCE_PROVENANCE_v5_0g.json").read_text()); M=json.loads((V5G/"MANIFEST_v5_0g.json").read_text()); D={e["file"]:e for e in M["files"]}
 for name,rp in P["direct_repo_path_map"].items():
  e=D[name]; chk(V5G/rp,e["bytes"],e["sha256"])
 for e in P["reconstructed_files"]:
  b=b"".join(chk(V5G/p["file"],p["bytes"],p["sha256"],p["git_blob_sha1"]) for p in e["parts"])
  if e["encoding"]=="zlib_concat":
   if len(b)!=e["compressed_bytes"] or h(b)!=e["compressed_sha256"]: raise AssertionError("compressed evidence mismatch")
   b=zlib.decompress(b)
  elif e["encoding"]!="raw_concat": raise AssertionError("unknown evidence encoding")
  if len(b)!=e["bytes"] or h(b)!=e["sha256"]: raise AssertionError(f"reconstruction mismatch {e['canonical_filename']}")
  d=D[e["canonical_filename"]]
  if d["bytes"]!=e["bytes"] or d["sha256"]!=e["sha256"]: raise AssertionError("manifest/reconstruction mismatch")
 for name in P["external_package_assets"]:
  if name not in D: raise AssertionError(f"external declaration missing {name}")
 x=P["known_historical_exception"]; mb=(V5G/"MANIFEST_v5_0g.json").read_bytes(); d=D["MANIFEST_v5_0g.json"]
 if len(mb)!=x["actual_bytes"] or h(mb)!=x["actual_sha256"] or d["bytes"]!=x["declared_bytes"] or d["sha256"]!=x["declared_sha256"]: raise AssertionError("v5.0g manifest historical exception changed")
 if x["actual_bytes"]==x["declared_bytes"] and x["actual_sha256"]==x["declared_sha256"]: raise AssertionError("historical exception unexpectedly absent")
 print("PASS v5.0g repository evidence integrity")
def v5e():
 P=json.loads((V5E/"SOURCE_PROVENANCE.json").read_text()); b=b""
 for p in P["parts"]:
  q=V5E/p["file"]; d=q.read_bytes()
  if len(d)!=p["bytes"] or gh(d)!=p["git_blob_sha1"]: raise AssertionError(f"v5.0e part mismatch {p['file']}")
  b+=d
 if len(b)!=P["canonical_bytes"] or h(b)!=P["canonical_sha256"] or gh(b)!=P["canonical_git_blob_sha1"]: raise AssertionError("v5.0e source mismatch")
 print("PASS v5.0e byte-exact source integrity")
def runtime():
 r=V5G/"runtime"
 with tempfile.TemporaryDirectory() as td:
  x=Path(td)/"p"; subprocess.run(["g++","-std=c++17","-O2","-Wall","-Wextra","-Werror",str(r/"uncertainty_runtime_v5_0g.cpp"),str(r/"runtime_parity_test.cpp"),"-I",str(r),"-o",str(x)],check=True); subprocess.run([str(x)],check=True)
 print("PASS v5.0g native runtime parity")
def main(): v5g(); v5e(); runtime(); print("PASS canonical integrity")
if __name__=="__main__":
 try: main()
 except Exception as e: print(f"FAIL canonical integrity: {e}",file=sys.stderr); raise
