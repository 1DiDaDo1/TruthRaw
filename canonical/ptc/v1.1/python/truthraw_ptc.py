from __future__ import annotations
import argparse, base64, copy, hashlib, html, json, re, struct
from pathlib import Path
from typing import Any, Dict, Iterable, Optional, Tuple

SCHEMA = "TruthRawPureTruthCertificate/1.1"
VERSION = "PTC-1.1"
COPYRIGHT_NOTICE = "Copyright © TruthRaw Project. Pure Truth Certificate PTC v1.1. Reconstruction provenance embedded."
STATEMENT = "TruthRaw Pure Truth certifies the integrity of the reconstruction process, not omniscient knowledge of the photographed scene."
XMP_NS = "urn:truthraw:ptc:1.1"
XMP_APP1_ID = b"http://ns.adobe.com/xap/1.0/\x00"

class PtcError(RuntimeError):
    pass

def sha256_file(path: str | Path) -> str:
    h=hashlib.sha256()
    with open(path,"rb") as f:
        for chunk in iter(lambda:f.read(1<<20),b""):
            h.update(chunk)
    return h.hexdigest()

def canonical_json(obj: Any) -> bytes:
    return json.dumps(obj, ensure_ascii=False, sort_keys=True, separators=(",",":")).encode("utf-8")

def certificate_json_sha256(cert: Dict[str,Any]) -> str:
    c=copy.deepcopy(cert)
    c.pop("certificate_json_sha256",None)
    c.pop("signature",None)
    return hashlib.sha256(canonical_json(c)).hexdigest()

def _valid_sha(v: Any) -> bool:
    return isinstance(v,str) and bool(re.fullmatch(r"[0-9a-f]{64}",v))

def base_certificate(scope: str="FULL_PHYSICAL") -> Dict[str,Any]:
    if scope not in {"CORE_INTEGRITY","FULL_PHYSICAL"}:
        raise PtcError("scope must be CORE_INTEGRITY or FULL_PHYSICAL")
    return {
        "schema":SCHEMA,
        "certificate_version":VERSION,
        "scope":scope,
        "status":"TRUTHRAW_DEVELOPMENT",
        "canonical_principle":"Measured where measured. Reconstructed where necessary. Never invented.",
        "source":{"filename":None,"sha256":None,"source_class":None,"direct_sensor_cfa":None},
        "capture":{"single_frame":True,"multi_frame_evidence_used":False},
        "admission":{
            "cfa_validated":False,"blacklevel_validated":False,"whitelevel_validated":False,
            "gainmap_present":None,"gainmap_application_count":None,"color_transform_bound":False,
            "source_identity_bound":False
        },
        "reconstruction":{
            "backend":None,"backend_version":None,"backend_sha256":None,"backend_bound_to_output":False,
            "generated_scene_content":False,"semantic_detail_generation":False,
            "measured_samples_preserved_by_policy":True
        },
        "noise_uncertainty":{"noise_model_bound":False,"backend_bound_uncertainty":False,"uncertainty_status":"unavailable"},
        "clipping":{"whitelevel_censoring_preserved":True,"invented_clipped_detail":False},
        "scientific_master":{
            "sha256":None,"scene_linear":True,"appearance_separated":True,
            "negative_evidence_preserved":True,"overrange_preserved":True
        },
        "physical_calibration":{
            "per_lens_color_calibrated":False,"illuminant_calibrated":False,
            "electron_calibration_bound":False,"optics_calibration_bound":False
        },
        "claims":{
            "colorimetric_accuracy":False,"electron_truth":False,"optical_restoration":False,
            "calibrated_uncertainty":False
        },
        "appearance":{
            "profile":None,"material_truth_guard":None,"output_acutance":None,
            "generated_detail":False,"uniform_rgb_detail_scaling":None
        },
        "export":{
            "format":None,"media_payload_sha256":None,"hidden_extra_isp":False,
            "ptc_metadata_embedded":False,"copyright_embedded":False
        },
        "copyright_notice":COPYRIGHT_NOTICE,
        "certification_statement":STATEMENT
    }

def evaluate(cert: Dict[str,Any]) -> Dict[str,Any]:
    hard=[]; completion=[]
    def req(cond: bool, name: str):
        if not cond: hard.append(name)
    def full(cond: bool, name: str):
        if not cond: completion.append(name)
    req(cert.get("schema")==SCHEMA,"schema")
    req(cert.get("certificate_version")==VERSION,"certificate_version")
    req(cert.get("scope") in {"CORE_INTEGRITY","FULL_PHYSICAL"},"scope")
    s=cert.get("source",{})
    req(_valid_sha(s.get("sha256")),"source.sha256")
    req(bool(s.get("source_class")) and s.get("source_class")!="UNKNOWN","source.source_class")
    cap=cert.get("capture",{})
    req(cap.get("single_frame") is True,"capture.single_frame")
    req(cap.get("multi_frame_evidence_used") is False,"capture.multi_frame_evidence_used")
    a=cert.get("admission",{})
    for k in ["cfa_validated","blacklevel_validated","whitelevel_validated","color_transform_bound","source_identity_bound"]:
        req(a.get(k) is True,f"admission.{k}")
    gp=a.get("gainmap_present")
    gac=a.get("gainmap_application_count")
    req(gp in {True,False},"admission.gainmap_present")
    if gp is True: req(gac==1,"admission.gainmap_exactly_once")
    if gp is False: req(gac==0,"admission.no_gainmap_zero_applications")
    r=cert.get("reconstruction",{})
    req(bool(r.get("backend")),"reconstruction.backend")
    req(bool(r.get("backend_version")),"reconstruction.backend_version")
    req(_valid_sha(r.get("backend_sha256")),"reconstruction.backend_sha256")
    req(r.get("backend_bound_to_output") is True,"reconstruction.backend_bound_to_output")
    req(r.get("generated_scene_content") is False,"reconstruction.generated_scene_content")
    req(r.get("semantic_detail_generation") is False,"reconstruction.semantic_detail_generation")
    req(r.get("measured_samples_preserved_by_policy") is True,"reconstruction.measured_samples_preserved_by_policy")
    cl=cert.get("clipping",{})
    req(cl.get("whitelevel_censoring_preserved") is True,"clipping.whitelevel_censoring_preserved")
    req(cl.get("invented_clipped_detail") is False,"clipping.invented_clipped_detail")
    sm=cert.get("scientific_master",{})
    req(_valid_sha(sm.get("sha256")),"scientific_master.sha256")
    req(sm.get("scene_linear") is True,"scientific_master.scene_linear")
    req(sm.get("appearance_separated") is True,"scientific_master.appearance_separated")
    req(sm.get("negative_evidence_preserved") is True,"scientific_master.negative_evidence_preserved")
    req(sm.get("overrange_preserved") is True,"scientific_master.overrange_preserved")
    app=cert.get("appearance",{})
    req(app.get("generated_detail") is False,"appearance.generated_detail")
    ex=cert.get("export",{})
    req(bool(ex.get("format")),"export.format")
    req(_valid_sha(ex.get("media_payload_sha256")),"export.media_payload_sha256")
    req(ex.get("hidden_extra_isp") is False,"export.hidden_extra_isp")
    req(ex.get("ptc_metadata_embedded") is True,"export.ptc_metadata_embedded")
    req(ex.get("copyright_embedded") is True,"export.copyright_embedded")
    req(cert.get("copyright_notice")==COPYRIGHT_NOTICE,"copyright_notice")
    req(cert.get("certification_statement")==STATEMENT,"certification_statement")

    pc=cert.get("physical_calibration",{})
    claims=cert.get("claims",{})
    nu=cert.get("noise_uncertainty",{})
    # Claim-safety gates: unsupported physical claims are a hard failure.
    if claims.get("colorimetric_accuracy"):
        req(pc.get("per_lens_color_calibrated") is True and pc.get("illuminant_calibrated") is True,"claim.colorimetric_accuracy_requires_calibration")
    if claims.get("electron_truth"):
        req(pc.get("electron_calibration_bound") is True,"claim.electron_truth_requires_calibration")
    if claims.get("optical_restoration"):
        req(pc.get("optics_calibration_bound") is True,"claim.optical_restoration_requires_calibration")
    if claims.get("calibrated_uncertainty"):
        req(nu.get("backend_bound_uncertainty") is True,"claim.calibrated_uncertainty_requires_backend_binding")

    if cert.get("scope")=="FULL_PHYSICAL":
        full(nu.get("noise_model_bound") is True,"full.noise_model_bound")
        full(nu.get("backend_bound_uncertainty") is True,"full.backend_bound_uncertainty")
        full(pc.get("per_lens_color_calibrated") is True,"full.per_lens_color_calibrated")
        full(pc.get("illuminant_calibrated") is True,"full.illuminant_calibrated")
        full(pc.get("electron_calibration_bound") is True,"full.electron_calibration_bound")
        full(pc.get("optics_calibration_bound") is True,"full.optics_calibration_bound")

    if hard: status="NOT_CERTIFIED"
    elif completion: status="PURE_TRUTH_DERIVED"
    else: status="PURE_TRUTH_CERTIFIED"
    return {"status":status,"hard_failures":hard,"completion_blockers":completion,"pass_core":not hard,"pass_full":not hard and not completion}

def apply_evaluation(cert: Dict[str,Any]) -> Dict[str,Any]:
    c=copy.deepcopy(cert)
    e=evaluate(c)
    c["status"]=e["status"]
    c["evaluation"]=e
    c["certificate_json_sha256"]=certificate_json_sha256(c)
    return c

# --- JPEG payload hashing / XMP embedding ---
def _jpeg_segments(data: bytes) -> Iterable[Tuple[int,int,int]]:
    if not data.startswith(b"\xff\xd8"):
        raise PtcError("not a JPEG")
    pos=2
    yield (0xD8,0,2)
    while pos < len(data):
        if data[pos] != 0xFF:
            raise PtcError(f"invalid JPEG marker at {pos}")
        start=pos
        while pos < len(data) and data[pos]==0xFF: pos += 1
        if pos>=len(data): break
        marker=data[pos]; pos+=1
        if marker==0xD9:
            yield (marker,start,pos); break
        if marker in [0x01] or 0xD0<=marker<=0xD7:
            yield (marker,start,pos); continue
        if pos+2>len(data): raise PtcError("truncated JPEG segment")
        ln=struct.unpack(">H",data[pos:pos+2])[0]
        if ln<2 or pos+ln>len(data): raise PtcError("invalid JPEG segment length")
        end=pos+ln
        yield (marker,start,end)
        if marker==0xDA: # SOS: scan data until EOI, respecting byte stuffing/restart markers
            scan=end
            p=scan
            while p+1<len(data):
                if data[p]!=0xFF: p+=1; continue
                q=p+1
                while q<len(data) and data[q]==0xFF: q+=1
                if q>=len(data): break
                m=data[q]
                if m==0x00 or 0xD0<=m<=0xD7:
                    p=q+1; continue
                if m==0xD9:
                    yield (0x100,scan,p) # pseudo scan segment
                    yield (0xD9,p,q+1)
                    return
                # Progressive/multi-scan JPEG: yield entropy chunk, continue parsing marker.
                yield (0x100,scan,p)
                pos=p
                break
            else: return
            if pos==start: return
            continue
        pos=end

def jpeg_media_payload_bytes(data: bytes) -> bytes:
    out=bytearray()
    for marker,start,end in _jpeg_segments(data):
        # Exclude metadata-only APP0..APP15 and COM. Include coding/image semantics.
        if 0xE0<=marker<=0xEF or marker==0xFE:
            continue
        out += data[start:end]
    return bytes(out)

def jpeg_media_payload_sha256(path: str|Path) -> str:
    return hashlib.sha256(jpeg_media_payload_bytes(Path(path).read_bytes())).hexdigest()

def build_xmp(cert: Dict[str,Any], external_certificate_name: Optional[str]=None) -> bytes:
    # Embed essential provenance, not a mutable whole-file hash.
    esc=lambda x: html.escape(str(x), quote=True)
    s=cert.get("source",{}); r=cert.get("reconstruction",{}); sm=cert.get("scientific_master",{}); ex=cert.get("export",{})
    sig=cert.get("signature") or {}
    ext = f'<truthraw:ExternalCertificate>{esc(external_certificate_name)}</truthraw:ExternalCertificate>' if external_certificate_name else ''
    sigxml=''
    if sig:
        sigxml=(f'<truthraw:SignatureAlgorithm>{esc(sig.get("algorithm",""))}</truthraw:SignatureAlgorithm>'
                f'<truthraw:PublicKey>{esc(sig.get("public_key_base64",""))}</truthraw:PublicKey>'
                f'<truthraw:Signature>{esc(sig.get("signature_base64",""))}</truthraw:Signature>')
    xml=f'''<?xpacket begin="\ufeff" id="W5M0MpCehiHzreSzNTczkc9d"?>
<x:xmpmeta xmlns:x="adobe:ns:meta/">
 <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
  <rdf:Description rdf:about="" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:xmpRights="http://ns.adobe.com/xap/1.0/rights/" xmlns:truthraw="{XMP_NS}">
   <dc:rights><rdf:Alt><rdf:li xml:lang="x-default">{esc(COPYRIGHT_NOTICE)}</rdf:li></rdf:Alt></dc:rights>
   <xmpRights:Marked>True</xmpRights:Marked>
   <truthraw:Certificate>{esc(cert.get("status"))}</truthraw:Certificate>
   <truthraw:CertificateVersion>{VERSION}</truthraw:CertificateVersion>
   <truthraw:Scope>{esc(cert.get("scope"))}</truthraw:Scope>
   <truthraw:CertificateJSONSHA256>{esc(cert.get("certificate_json_sha256",""))}</truthraw:CertificateJSONSHA256>
   <truthraw:SourceSHA256>{esc(s.get("sha256",""))}</truthraw:SourceSHA256>
   <truthraw:SourceClass>{esc(s.get("source_class",""))}</truthraw:SourceClass>
   <truthraw:ReconstructionBackend>{esc(r.get("backend",""))}</truthraw:ReconstructionBackend>
   <truthraw:ReconstructionBackendSHA256>{esc(r.get("backend_sha256",""))}</truthraw:ReconstructionBackendSHA256>
   <truthraw:ScientificMasterSHA256>{esc(sm.get("sha256",""))}</truthraw:ScientificMasterSHA256>
   <truthraw:MediaPayloadSHA256>{esc(ex.get("media_payload_sha256",""))}</truthraw:MediaPayloadSHA256>
   <truthraw:GeneratedSceneContent>{str(bool(r.get("generated_scene_content"))).lower()}</truthraw:GeneratedSceneContent>
   <truthraw:MultiFrameEvidenceUsed>{str(bool(cert.get("capture",{}).get("multi_frame_evidence_used"))).lower()}</truthraw:MultiFrameEvidenceUsed>
   <truthraw:CopyrightNotice>{esc(COPYRIGHT_NOTICE)}</truthraw:CopyrightNotice>
   <truthraw:CertificationStatement>{esc(STATEMENT)}</truthraw:CertificationStatement>
   {ext}{sigxml}
  </rdf:Description>
 </rdf:RDF>
</x:xmpmeta>
<?xpacket end="w"?>'''
    return xml.encode("utf-8")

def _remove_truthraw_xmp(data: bytes) -> bytes:
    if not data.startswith(b"\xff\xd8"): raise PtcError("not JPEG")
    out=bytearray(data[:2]); pos=2
    while pos < len(data):
        if data[pos]!=0xFF: out += data[pos:]; break
        start=pos
        p=pos
        while p<len(data) and data[p]==0xFF:p+=1
        if p>=len(data):break
        marker=data[p];p+=1
        if marker==0xDA:
            out += data[start:]; break
        if marker==0xD9:
            out += data[start:p]; break
        if marker in [0x01] or 0xD0<=marker<=0xD7:
            out += data[start:p];pos=p;continue
        ln=struct.unpack(">H",data[p:p+2])[0];end=p+ln
        seg=data[start:end]
        is_ours=(marker==0xE1 and XMP_APP1_ID in seg and XMP_NS.encode() in seg)
        if not is_ours: out += seg
        pos=end
    return bytes(out)

def embed_jpeg_xmp(src: str|Path, dst: str|Path, cert: Dict[str,Any], external_certificate_name: Optional[str]=None) -> None:
    data=_remove_truthraw_xmp(Path(src).read_bytes())
    before=hashlib.sha256(jpeg_media_payload_bytes(data)).hexdigest()
    expected=cert.get("export",{}).get("media_payload_sha256")
    if expected and before!=expected:
        raise PtcError(f"JPEG media payload does not match certificate: {before} != {expected}")
    packet=XMP_APP1_ID+build_xmp(cert,external_certificate_name)
    if len(packet)+2>65535: raise PtcError("XMP packet too large for standard JPEG APP1")
    app1=b"\xff\xe1"+struct.pack(">H",len(packet)+2)+packet
    # Insert immediately after SOI. Existing EXIF/JFIF remain intact and pixels/coding data do not change.
    out=data[:2]+app1+data[2:]
    Path(dst).write_bytes(out)
    after=jpeg_media_payload_sha256(dst)
    if after!=before: raise PtcError("media payload changed while embedding XMP")

def extract_truthraw_xmp(path: str|Path) -> Optional[str]:
    data=Path(path).read_bytes(); pos=2
    if not data.startswith(b"\xff\xd8"): return None
    while pos+4<=len(data):
        if data[pos]!=0xFF: break
        p=pos
        while p<len(data) and data[p]==0xFF:p+=1
        if p>=len(data):break
        marker=data[p];p+=1
        if marker in {0xDA,0xD9}: break
        if marker in [0x01] or 0xD0<=marker<=0xD7: pos=p;continue
        if p+2>len(data):break
        ln=struct.unpack(">H",data[p:p+2])[0];end=p+ln
        if marker==0xE1:
            payload=data[p+2:end]
            if payload.startswith(XMP_APP1_ID) and XMP_NS.encode() in payload:
                return payload[len(XMP_APP1_ID):].decode("utf-8","replace")
        pos=end
    return None

# Optional Ed25519 integrity signature. Identity requires a separately trusted public key.
def sign_certificate(cert: Dict[str,Any], private_key) -> Dict[str,Any]:
    from cryptography.hazmat.primitives import serialization
    c=copy.deepcopy(cert); c.pop("signature",None)
    msg=canonical_json(c)
    sig=private_key.sign(msg)
    pub=private_key.public_key().public_bytes(serialization.Encoding.Raw,serialization.PublicFormat.Raw)
    c["signature"]={"algorithm":"Ed25519","public_key_base64":base64.b64encode(pub).decode(),"signature_base64":base64.b64encode(sig).decode()}
    return c

def verify_signature(cert: Dict[str,Any]) -> bool:
    from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PublicKey
    c=copy.deepcopy(cert); sig=c.pop("signature",None)
    if not sig or sig.get("algorithm")!="Ed25519": return False
    try:
        pub=Ed25519PublicKey.from_public_bytes(base64.b64decode(sig["public_key_base64"]))
        pub.verify(base64.b64decode(sig["signature_base64"]),canonical_json(c))
        return True
    except Exception:
        return False

def verify_against_files(cert: Dict[str,Any], source_path: Optional[str|Path]=None, master_path: Optional[str|Path]=None, jpeg_path: Optional[str|Path]=None) -> Dict[str,Any]:
    checks={}
    if source_path is not None:
        checks["source_sha256"] = sha256_file(source_path)==cert.get("source",{}).get("sha256")
    if master_path is not None:
        checks["scientific_master_sha256"] = sha256_file(master_path)==cert.get("scientific_master",{}).get("sha256")
    if jpeg_path is not None:
        checks["media_payload_sha256"] = jpeg_media_payload_sha256(jpeg_path)==cert.get("export",{}).get("media_payload_sha256")
        x=extract_truthraw_xmp(jpeg_path)
        checks["ptc_xmp_present"] = x is not None and cert.get("certificate_json_sha256","") in x and COPYRIGHT_NOTICE in x
    checks["certificate_json_sha256"] = cert.get("certificate_json_sha256")==certificate_json_sha256(cert)
    if cert.get("signature") is not None: checks["signature"] = verify_signature(cert)
    return {"pass":all(checks.values()) if checks else False,"checks":checks,"evaluation":evaluate(cert)}

def main():
    ap=argparse.ArgumentParser(description="TruthRaw Pure Truth Certificate PTC-1.1 reference tool")
    sp=ap.add_subparsers(dest="cmd",required=True)
    e=sp.add_parser("evaluate"); e.add_argument("certificate")
    v=sp.add_parser("verify"); v.add_argument("certificate");v.add_argument("--source");v.add_argument("--master");v.add_argument("--jpeg")
    j=sp.add_parser("jpeg-hash");j.add_argument("jpeg")
    x=sp.add_parser("extract-xmp");x.add_argument("jpeg")
    args=ap.parse_args()
    if args.cmd=="evaluate": print(json.dumps(evaluate(json.loads(Path(args.certificate).read_text())),indent=2))
    elif args.cmd=="verify": print(json.dumps(verify_against_files(json.loads(Path(args.certificate).read_text()),args.source,args.master,args.jpeg),indent=2))
    elif args.cmd=="jpeg-hash": print(jpeg_media_payload_sha256(args.jpeg))
    elif args.cmd=="extract-xmp": print(extract_truthraw_xmp(args.jpeg) or "")

if __name__=="__main__": main()
