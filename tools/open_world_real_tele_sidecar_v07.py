#!/usr/bin/env python3
"""TruthRaw v0.7: deterministic real 4080x3072 tele source-bound authority sidecar.

The sidecar only emits what the admitted DNG itself can support. The sampled
CFA role becomes a Stage-2 CALIBRATED_ESTIMATE (or explicit highlight CENSORED
at WhiteLevel); the two absent RGB roles remain UNKNOWN until an exact persisted
Scientific Master plus v5.0g-p1 missing-channel runtime artifact is bound.
"""
from __future__ import annotations
import hashlib, json, math, struct
from dataclasses import dataclass
from pathlib import Path
from typing import Optional, Sequence, Tuple

MAGIC=b"TRDAF07\0"; VERSION=7
PREFIX=struct.Struct("<8sHHIQII"); RECORD=struct.Struct("<ffB")
REGION_CORE=128
AUTH_CALIBRATED=0; AUTH_CENSORED_HIGHLIGHT=1
EXPECTED_SOURCE_SHA256="7f64a628ff431272fc76f17b63179c93af8e496c40cdbd3b45d7dd67fd58b344"
EXPECTED_GAIN_MIN=1.000986099243164; EXPECTED_GAIN_MAX=2.365234375
EXPECTED_STAGE2_MIN=-0.007270084228366613; EXPECTED_STAGE2_MAX=0.6392968893051147
UNCERTAINTY_BINDING_SHA256="61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0"

class SidecarError(RuntimeError): pass

def sha256_file(path: Path)->str:
    h=hashlib.sha256()
    with Path(path).open("rb") as f:
        for c in iter(lambda:f.read(1<<20),b""): h.update(c)
    return h.hexdigest()

def canonical_json_bytes(x:dict)->bytes:
    return json.dumps(x,sort_keys=True,separators=(",",":"),ensure_ascii=True).encode()

def canonical_authority_region(x:int,y:int,width:int,height:int,core:int=REGION_CORE)->Tuple[int,Tuple[int,int,int,int]]:
    if width<=0 or height<=0 or core<=0 or not(0<=x<width and 0<=y<height): raise SidecarError("invalid authority-region coordinate")
    nx=(width+core-1)//core; rx=x//core; ry=y//core; x0=rx*core; y0=ry*core
    return ry*nx+rx,(x0,y0,min(x0+core,width),min(y0+core,height))

def gainmap_axis_coordinate(ordinal:int,count:int,origin:float,spacing:float,points:int)->Tuple[int,int,float]:
    if count<=0 or points<=0 or spacing<=0: raise SidecarError("invalid GainMap axis")
    n=0.0 if count==1 else ordinal/float(count-1)
    u=min(max((n-origin)/spacing,0.0),float(points-1)); i0=int(math.floor(u)); i1=min(i0+1,points-1)
    return i0,i1,u-i0

def bilinear_gain_scalar(grid:Sequence[Sequence[float]],ro:int,rc:int,co:int,cc:int,ov:float,oh:float,sv:float,sh:float)->float:
    v0,v1,fv=gainmap_axis_coordinate(ro,rc,ov,sv,len(grid)); h0,h1,fh=gainmap_axis_coordinate(co,cc,oh,sh,len(grid[0]))
    a=float(grid[v0][h0])*(1-fh)+float(grid[v0][h1])*fh; b=float(grid[v1][h0])*(1-fh)+float(grid[v1][h1])*fh
    return a*(1-fv)+b*fv

def source_noise_sigma_stage2(pre:float,gain:float,s:float,o:float)->float:
    if not all(math.isfinite(v) for v in (pre,gain,s,o)) or gain<=0 or s<0 or o<0: raise SidecarError("invalid NoiseProfile input")
    return gain*math.sqrt(max(s*max(pre,0.0)+o,0.0))

def encode_flags(phase:int,authority:int)->int:
    if phase not in range(4) or authority not in (AUTH_CALIBRATED,AUTH_CENSORED_HIGHLIGHT): raise SidecarError("invalid flags")
    return phase|(authority<<2)

@dataclass(frozen=True)
class GainMap:
    top:int; left:int; bottom:int; right:int; row_pitch:int; col_pitch:int
    mv:int; mh:int; sv:float; sh:float; ov:float; oh:float; values:object

def _black(v)->Tuple[float,float,float,float]:
    t=tuple(v)
    if len(t)==8: return tuple(float(t[i])/float(t[i+1]) for i in range(0,8,2)) # type: ignore
    if len(t)==4: return tuple(float(x) for x in t) # type: ignore
    raise SidecarError("unsupported BlackLevel")

def _gainmaps(data:bytes,np)->list[GainMap]:
    off=0; n=struct.unpack_from(">I",data,off)[0]; off+=4; out=[]
    for _ in range(n):
        oid,ver,flags,size=struct.unpack_from(">IIII",data,off); off+=16; p=data[off:off+size]; off+=size
        if oid!=9 or len(p)<76: raise SidecarError("v0.7 accepts GainMap OpcodeList2 only")
        top,left,bottom,right,plane,planes,rp,cp,mv,mh,sv,sh,ov,oh,mp=struct.unpack_from(">10I4dI",p,0)
        if plane!=0 or planes!=1 or mp!=1 or len(p)!=76+mv*mh*4: raise SidecarError("unsupported GainMap topology")
        g=np.frombuffer(p,dtype=">f4",offset=76,count=mv*mh).astype(np.float32).reshape(mv,mh)
        out.append(GainMap(top,left,bottom,right,rp,cp,mv,mh,sv,sh,ov,oh,g))
    if off!=len(data) or len(out)!=4: raise SidecarError("expected exactly four GainMaps")
    return out

def _interp(m:GainMap,rows,cols,np):
    nr=len(range(m.top,m.bottom,m.row_pitch)); nc=len(range(m.left,m.right,m.col_pitch))
    ro=(rows-m.top)//m.row_pitch; co=(cols-m.left)//m.col_pitch
    rv=np.zeros(ro.shape) if nr==1 else ro.astype(np.float64)/(nr-1); cv=np.zeros(co.shape) if nc==1 else co.astype(np.float64)/(nc-1)
    v=np.clip((rv-m.ov)/m.sv,0,m.mv-1); u=np.clip((cv-m.oh)/m.sh,0,m.mh-1)
    v0=np.floor(v).astype(np.int64); v1=np.minimum(v0+1,m.mv-1); fv=v-v0
    u0=np.floor(u).astype(np.int64); u1=np.minimum(u0+1,m.mh-1); fu=u-u0; g=m.values
    g00=g[v0[:,None],u0[None,:]].astype(np.float64); g01=g[v0[:,None],u1[None,:]].astype(np.float64)
    g10=g[v1[:,None],u0[None,:]].astype(np.float64); g11=g[v1[:,None],u1[None,:]].astype(np.float64)
    return ((1-fv[:,None])*((1-fu[None,:])*g00+fu[None,:]*g01)+fv[:,None]*((1-fu[None,:])*g10+fu[None,:]*g11)).astype(np.float32)

def _gain_chunk(y0:int,y1:int,w:int,h:int,maps:Sequence[GainMap],np):
    out=np.ones((y1-y0,w),np.float32); cover=np.zeros((y1-y0,w),np.uint8)
    for m in maps:
        rows=np.arange(max(y0,m.top),min(y1,m.bottom),dtype=np.int64); rows=rows[((rows-m.top)%m.row_pitch)==0]
        cols=np.arange(m.left,m.right,dtype=np.int64); cols=cols[((cols-m.left)%m.col_pitch)==0]
        if rows.size:
            rr=rows-y0; out[np.ix_(rr,cols)]=_interp(m,rows,cols,np); cover[np.ix_(rr,cols)]+=1
    if np.any(cover!=1): raise SidecarError("GainMaps must cover each CFA sample exactly once")
    return out

def _span(raw,y0,y1,w,h,black,white,noise,maps,np):
    r=raw[y0:y1].astype(np.float32,copy=False); gain=_gain_chunk(y0,y1,w,h,maps,np)
    yy=np.arange(y0,y1,dtype=np.int64)[:,None]; xx=np.arange(w,dtype=np.int64)[None,:]; phase=((yy&1)*2+(xx&1)).astype(np.uint8)
    b=np.asarray(black,np.float32)[phase]; pre=(r-b)/np.maximum(np.float32(white)-b,np.float32(1)); stage=(pre*gain).astype(np.float32)
    color=np.asarray([2,1,1,0],np.uint8)[phase]; s=np.asarray(noise[0::2],np.float32)[color]; o=np.asarray(noise[1::2],np.float32)[color]
    sigma=(gain*np.sqrt(np.maximum(s*np.maximum(pre,np.float32(0))+o,np.float32(0)))).astype(np.float32)
    high=r>=np.float32(white); value=stage.copy(); value[high]=gain[high]; sigma[high]=0
    flags=(phase|(np.where(high,AUTH_CENSORED_HIGHLIGHT,AUTH_CALIBRATED).astype(np.uint8)<<2)).astype(np.uint8)
    return value,sigma,flags,stage,gain,high

def generate_real_tele_sidecar(dng:Path,out:Path,*,span_rows:int=64,expected_sha256:Optional[str]=EXPECTED_SOURCE_SHA256)->dict:
    try: import numpy as np; import tifffile
    except Exception as e: raise SidecarError("real generation requires numpy+tifffile") from e
    if span_rows<=0: raise SidecarError("span_rows must be positive")
    source=sha256_file(dng)
    if expected_sha256 and source.lower()!=expected_sha256.lower(): raise SidecarError("source SHA mismatch")
    with tifffile.TiffFile(str(dng)) as tf:
        if len(tf.pages)!=1: raise SidecarError("exactly one CFA IFD required")
        p=tf.pages[0]; tags=p.tags
        if tuple(p.shape)!=(3072,4080) or str(p.dtype)!="uint16" or str(tags["Make"].value)!="HONOR" or str(tags["Model"].value)!="BKQ-N49": raise SidecarError("source class mismatch")
        if bytes(tags["CFAPattern"].value)!=b"\x02\x01\x01\x00": raise SidecarError("BGGR required")
        fv=tags["FocalLength"].value; focal=float(fv[0])/fv[1] if isinstance(fv,tuple) else float(fv)
        if abs(focal-22.48)>1e-6: raise SidecarError("focal mismatch")
        white=float(tags["WhiteLevel"].value); black=_black(tags["BlackLevel"].value); noise=tuple(float(x) for x in tags["NoiseProfile"].value)
        op=bytes(tags["OpcodeList2"].value); maps=_gainmaps(op,np); raw=p.asarray(out="memmap"); h,w=raw.shape
        st={"gain_min":math.inf,"gain_max":-math.inf,"stage2_min":math.inf,"stage2_max":-math.inf,"sigma_min":math.inf,"sigma_max":-math.inf,"censored":0}
        for y0 in range(0,h,span_rows):
            v,s,f,stage,gain,hi=_span(raw,y0,min(h,y0+span_rows),w,h,black,white,noise,maps,np)
            st["gain_min"]=min(st["gain_min"],float(gain.min())); st["gain_max"]=max(st["gain_max"],float(gain.max()))
            st["stage2_min"]=min(st["stage2_min"],float(stage.min())); st["stage2_max"]=max(st["stage2_max"],float(stage.max()))
            st["sigma_min"]=min(st["sigma_min"],float(s.min())); st["sigma_max"]=max(st["sigma_max"],float(s.max())); st["censored"]+=int(hi.sum())
        ref={"gain_min":st["gain_min"]==EXPECTED_GAIN_MIN,"gain_max":st["gain_max"]==EXPECTED_GAIN_MAX,"stage2_min":st["stage2_min"]==EXPECTED_STAGE2_MIN,"stage2_max":st["stage2_max"]==EXPECTED_STAGE2_MAX}
        if source==EXPECTED_SOURCE_SHA256 and not all(ref.values()): raise SidecarError("frozen Stage-2 reference mismatch")
        header={"schema":"TruthRawRealTeleDynamicAuthoritySidecar/0.7","classification":"REAL_TELE_SOURCE_BOUND_PARTIAL_DYNAMIC_AUTHORITY_SIDECAR_V07",
          "source":{"sha256":source,"make":"HONOR","model":"BKQ-N49","physical_camera_id":"5","focal_length_mm":focal,"width":w,"height":h,"cfa":"BGGR","white_level":white,"black_phase":list(black),"noise_profile":list(noise),"opcode_list2_sha256":hashlib.sha256(op).hexdigest()},
          "stage2":{"formula":"(raw-black)/(white-black)*GainMap_once","float32":True,"no_clamp":True,"gain_min":st["gain_min"],"gain_max":st["gain_max"],"minimum":st["stage2_min"],"maximum":st["stage2_max"],"frozen_reference_exact":ref},
          "authority":{"direct":"CALIBRATED_ESTIMATE_UNLESS_WHITELEVEL_CENSORED","missing_rgb":"UNKNOWN","missing_channel_reconstruction_emitted":False},
          "uncertainty":{"direct_kind":"DNG_NOISEPROFILE_PROPAGATED_1SIGMA_STAGE2","is_p95":False,"sigma_min":st["sigma_min"],"sigma_max":st["sigma_max"],"missing_channel_p95":"NOT_EMITTED","v5_0g_p1_binding_sha256":UNCERTAINTY_BINDING_SHA256},
          "censoring":{"highlight_rule":"raw>=WhiteLevel => scene>=Stage2_bound","highlight_count":st["censored"],"shadow_floor_claim":"NOT_EMITTED"},
          "authority_regions":{"core":[REGION_CORE,REGION_CORE],"grid":[(w+REGION_CORE-1)//REGION_CORE,(h+REGION_CORE-1)//REGION_CORE],"origin":[0,0],"administrative_only":True,"compute_partition_affects_authority":False},
          "record":{"struct":"<ffB","bytes":RECORD.size,"order":"global_raster","fields":["value_or_bound","source_sigma","flags"]},
          "lineage":{"scientific_master_sha256":None,"technical_backplane":"BLOCKED_NO_PERSISTED_REAL_SCIENTIFIC_MASTER_BACKPLANE_ARTIFACT","physical_frame_count":1,"independent_evidence_count":1,"writeback":False},
          "fixed_dynamic_range_limit_ev":None,"execution_partition_in_scientific_content":False}
        hb=canonical_json_bytes(header); pref=PREFIX.pack(MAGIC,VERSION,PREFIX.size,len(hb),w*h,RECORD.size,0); Path(out).parent.mkdir(parents=True,exist_ok=True)
        with Path(out).open("wb") as f:
            f.write(pref); f.write(hb)
            dt=np.dtype([("v","<f4"),("s","<f4"),("f","u1")],align=False)
            for y0 in range(0,h,span_rows):
                v,s,flags,*_=_span(raw,y0,min(h,y0+span_rows),w,h,black,white,noise,maps,np); rec=np.empty(v.size,dtype=dt); rec["v"]=v.ravel(); rec["s"]=s.ravel(); rec["f"]=flags.ravel(); f.write(rec.tobytes())
    size=Path(out).stat().st_size; expected=PREFIX.size+len(hb)+w*h*RECORD.size
    if size!=expected: raise SidecarError("artifact length mismatch")
    return {"schema":"TruthRawRealTeleDynamicAuthoritySidecarReport/0.7","classification":header["classification"],"artifact_sha256":sha256_file(out),"artifact_bytes":size,"source_sha256":source,"record_count":w*h,"header":header,"claim_boundary":"Real source-bound direct-CFA Stage-2 authority only; missing RGB remains UNKNOWN and no Scientific Master/backplane binding is claimed."}
