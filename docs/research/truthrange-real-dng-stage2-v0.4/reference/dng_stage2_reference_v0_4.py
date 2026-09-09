#!/usr/bin/env python3
"""TruthRaw v0.4 portable classic-TIFF DNG -> Stage-2 reference.

Scope is deliberately narrow and fail-closed:
- classic TIFF (magic 42), not BigTIFF;
- uncompressed 16-bit, single-sample CFA;
- strip storage;
- DNG BlackLevel / WhiteLevel / CFA pattern;
- OpcodeList2 containing GainMap opcodes;
- GainMap applied exactly once after black subtraction and source-white normalization.

No appearance/color/rendering is performed here.
"""
from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path
from array import array
import hashlib
import math
import struct
import sys
from typing import Dict, Iterable, List, Sequence, Tuple

TYPE_SIZES = {1:1, 2:1, 3:2, 4:4, 5:8, 7:1, 9:4, 10:8, 11:4, 12:8}

class DngStage2Error(ValueError):
    pass

@dataclass(frozen=True)
class IfdEntry:
    tag: int
    typ: int
    count: int
    raw: bytes

@dataclass(frozen=True)
class GainMapOpcode:
    opcode_id: int
    min_version: int
    flags: int
    area: Tuple[int,int,int,int]
    plane: int
    planes: int
    row_pitch: int
    col_pitch: int
    points_v: int
    points_h: int
    spacing_v: float
    spacing_h: float
    origin_v: float
    origin_h: float
    map_planes: int
    values: Tuple[float,...]
    payload_sha256: str

    def entry(self, r: int, c: int, p: int=0) -> float:
        return self.values[(r*self.points_h+c)*self.map_planes+p]

    def applies(self, y: int, x: int) -> bool:
        t,l,b,r = self.area
        return (t <= y < b and l <= x < r and
                (y-t) % self.row_pitch == 0 and
                (x-l) % self.col_pitch == 0)

    def interpolate(self, y: int, x: int, image_h: int, image_w: int, plane: int=0) -> float:
        if not (0 <= plane < self.map_planes):
            raise DngStage2Error('GainMap plane out of range')
        if image_h <= 0 or image_w <= 0:
            raise DngStage2Error('invalid image bounds')
        if not (self.spacing_v > 0.0 and self.spacing_h > 0.0):
            raise DngStage2Error('invalid GainMap spacing')
        # Adobe DNG SDK dng_gain_map_interpolator: image-bounds scale and +0.5 pixel-center offset.
        row_f = (((y + 0.5) / image_h) - self.origin_v) / self.spacing_v
        col_f = (((x + 0.5) / image_w) - self.origin_h) / self.spacing_h
        if not math.isfinite(row_f): row_f = 0.0
        if not math.isfinite(col_f): col_f = 0.0
        row_f = max(0.0, min(float(self.points_v-1), row_f))
        col_f = max(0.0, min(float(self.points_h-1), col_f))
        r0 = int(math.floor(row_f)); r1 = min(r0+1, self.points_v-1); rf = row_f-r0
        c0 = int(math.floor(col_f)); c1 = min(c0+1, self.points_h-1); cf = col_f-c0
        a = self.entry(r0,c0,plane)*(1.0-rf) + self.entry(r1,c0,plane)*rf
        b = self.entry(r0,c1,plane)*(1.0-rf) + self.entry(r1,c1,plane)*rf
        return a*(1.0-cf) + b*cf

@dataclass
class ClassicDng:
    path: Path
    endian: str
    width: int
    height: int
    bits_per_sample: int
    compression: int
    photometric: int
    samples_per_pixel: int
    rows_per_strip: int
    strip_offsets: Tuple[int,...]
    strip_byte_counts: Tuple[int,...]
    cfa_repeat: Tuple[int,int]
    cfa_pattern: bytes
    black_phase: Tuple[float,float,float,float]
    white_level: float
    opcode_list2: bytes
    gain_maps: Tuple[GainMapOpcode,...]

    @property
    def source_sha256(self) -> str:
        h=hashlib.sha256()
        with self.path.open('rb') as f:
            for chunk in iter(lambda:f.read(4*1024*1024), b''):
                h.update(chunk)
        return h.hexdigest()

    @property
    def opcode_list2_sha256(self) -> str:
        return hashlib.sha256(self.opcode_list2).hexdigest()

    def phase_index(self, y:int, x:int) -> int:
        return (y & 1)*2 + (x & 1)

    def gain_at(self, y:int, x:int) -> float:
        matches=[g for g in self.gain_maps if g.applies(y,x)]
        if len(matches) != 1:
            raise DngStage2Error(f'expected exactly one GainMap at ({y},{x}), found {len(matches)}')
        return matches[0].interpolate(y,x,self.height,self.width,0)

    def raw_row(self, y:int) -> array:
        if not (0 <= y < self.height): raise DngStage2Error('row out of range')
        if self.rows_per_strip != 1 or len(self.strip_offsets) != self.height:
            raise DngStage2Error('v0.4 reference currently requires one strip per row')
        nbytes=self.strip_byte_counts[y]
        if nbytes != self.width*2:
            raise DngStage2Error('unexpected row byte count')
        with self.path.open('rb') as f:
            f.seek(self.strip_offsets[y]); b=f.read(nbytes)
        if len(b) != nbytes: raise DngStage2Error('truncated strip')
        out=array('H'); out.frombytes(b)
        host_little=(sys.byteorder=='little')
        file_little=(self.endian=='<')
        if host_little != file_little: out.byteswap()
        if len(out) != self.width: raise DngStage2Error('row decode mismatch')
        return out

    def stage2_sample(self, y:int, x:int, raw_value:int|None=None) -> float:
        if raw_value is None: raw_value=int(self.raw_row(y)[x])
        b=self.black_phase[self.phase_index(y,x)]
        denom=max(self.white_level-b,1.0)
        signal=(float(raw_value)-b)/denom
        return signal*self.gain_at(y,x)


def _u16(b:bytes,o:int,e:str)->int: return struct.unpack_from(e+'H',b,o)[0]
def _u32(b:bytes,o:int,e:str)->int: return struct.unpack_from(e+'I',b,o)[0]

def _parse_ifd0(blob:bytes,endian:str,offset:int)->Dict[int,IfdEntry]:
    if offset+2 > len(blob): raise DngStage2Error('IFD outside file')
    n=_u16(blob,offset,endian); base=offset+2
    if base+n*12+4 > len(blob): raise DngStage2Error('truncated IFD')
    out={}
    for i in range(n):
        p=base+i*12
        tag=_u16(blob,p,endian); typ=_u16(blob,p+2,endian); count=_u32(blob,p+4,endian)
        if typ not in TYPE_SIZES: continue
        size=TYPE_SIZES[typ]*count
        if size <= 4:
            raw=blob[p+8:p+8+size]
        else:
            off=_u32(blob,p+8,endian)
            if off+size > len(blob): raise DngStage2Error(f'tag {tag} outside file')
            raw=blob[off:off+size]
        out[tag]=IfdEntry(tag,typ,count,raw)
    return out

def _values(ent:IfdEntry,endian:str):
    t,c,b=ent.typ,ent.count,ent.raw
    if t in (1,7): return bytes(b) if c!=1 else b[0]
    if t==3:
        vals=struct.unpack(endian+('H'*c),b); return vals[0] if c==1 else vals
    if t==4:
        vals=struct.unpack(endian+('I'*c),b); return vals[0] if c==1 else vals
    if t==5:
        vals=[]
        for i in range(c):
            n,d=struct.unpack_from(endian+'II',b,8*i)
            if d==0: raise DngStage2Error('zero rational denominator')
            vals.append(n/d)
        return vals[0] if c==1 else tuple(vals)
    raise DngStage2Error(f'unsupported value type {t}')

def parse_opcode_list2(blob:bytes)->Tuple[GainMapOpcode,...]:
    if len(blob) < 4: raise DngStage2Error('OpcodeList2 too short')
    pos=0; count=struct.unpack_from('>I',blob,pos)[0]; pos+=4
    if count > (len(blob)-4)//16: raise DngStage2Error('invalid opcode count')
    out=[]
    for _ in range(count):
        if pos+16 > len(blob): raise DngStage2Error('truncated opcode header')
        oid,minv,flags,size=struct.unpack_from('>IIII',blob,pos); pos+=16
        if size > len(blob)-pos: raise DngStage2Error('invalid opcode data size')
        payload=blob[pos:pos+size]; pos+=size
        if oid != 9:
            raise DngStage2Error(f'v0.4 source path rejects non-GainMap OpcodeList2 entry id={oid}')
        if len(payload) < 32+44: raise DngStage2Error('truncated GainMap payload')
        t,l,b,r,plane,planes,rowpitch,colpitch=struct.unpack_from('>iiiiIIII',payload,0)
        if b<=t or r<=l or planes<1 or rowpitch<1 or colpitch<1:
            raise DngStage2Error('invalid area spec')
        q=32
        pv,ph=struct.unpack_from('>II',payload,q); q+=8
        sv,sh=struct.unpack_from('>dd',payload,q); q+=16
        ov,oh=struct.unpack_from('>dd',payload,q); q+=16
        mp=struct.unpack_from('>I',payload,q)[0]; q+=4
        if pv<1 or ph<1 or mp<1 or not all(math.isfinite(v) for v in (sv,sh,ov,oh)) or sv<=0 or sh<=0:
            raise DngStage2Error('invalid GainMap geometry')
        n=pv*ph*mp
        if q+n*4 != len(payload): raise DngStage2Error('GainMap payload size mismatch')
        vals=struct.unpack_from('>'+('f'*n),payload,q)
        if not all(math.isfinite(v) for v in vals): raise DngStage2Error('non-finite GainMap entry')
        out.append(GainMapOpcode(oid,minv,flags,(t,l,b,r),plane,planes,rowpitch,colpitch,
                                 pv,ph,sv,sh,ov,oh,mp,tuple(vals),hashlib.sha256(payload).hexdigest()))
    if pos != len(blob): raise DngStage2Error('OpcodeList2 not fully consumed')
    return tuple(out)

def read_classic_dng(path:Path|str)->ClassicDng:
    path=Path(path); blob=path.read_bytes()
    if len(blob)<8: raise DngStage2Error('file too small')
    bo=blob[:2]
    if bo==b'II': endian='<'
    elif bo==b'MM': endian='>'
    else: raise DngStage2Error('invalid TIFF byte order')
    if _u16(blob,2,endian) != 42: raise DngStage2Error('v0.4 rejects BigTIFF/non-classic TIFF')
    ifd=_parse_ifd0(blob,endian,_u32(blob,4,endian))
    def req(tag):
        if tag not in ifd: raise DngStage2Error(f'missing required tag {tag}')
        return _values(ifd[tag],endian)
    width=int(req(256)); height=int(req(257)); bits=int(req(258)); compression=int(req(259)); phot=int(req(262))
    strips=req(273); strips=(int(strips),) if isinstance(strips,int) else tuple(map(int,strips))
    spp=int(req(277)); rps=int(req(278)); counts=req(279); counts=(int(counts),) if isinstance(counts,int) else tuple(map(int,counts))
    repeat=req(33421); repeat=tuple(map(int,repeat))
    pattern=req(33422); pattern=bytes(pattern) if not isinstance(pattern,int) else bytes([pattern])
    black=req(50714); black=(float(black),) if isinstance(black,(int,float)) else tuple(map(float,black))
    white=float(req(50717)); op=req(51009); op=bytes(op) if not isinstance(op,int) else bytes([op])
    if width<=0 or height<=0 or bits!=16 or compression!=1 or spp!=1 or phot!=32803:
        raise DngStage2Error('unsupported source DNG layout')
    if repeat!=(2,2) or len(pattern)!=4 or len(black)!=4:
        raise DngStage2Error('v0.4 requires 2x2 CFA and four phase black levels')
    if len(strips)!=len(counts): raise DngStage2Error('strip arrays mismatch')
    gains=parse_opcode_list2(op)
    # exact-one-map coverage audit on all 4 CFA phases at representative interior pixel.
    for py in range(2):
        for px in range(2):
            y=py; x=px
            n=sum(g.applies(y,x) for g in gains)
            if n!=1: raise DngStage2Error(f'GainMap coverage invalid for CFA phase {py},{px}')
    return ClassicDng(path,endian,width,height,bits,compression,phot,spp,rps,strips,counts,repeat,pattern,
                      tuple(black),white,op,gains)

if __name__=='__main__':
    import argparse, json
    ap=argparse.ArgumentParser(); ap.add_argument('dng'); args=ap.parse_args()
    d=read_classic_dng(args.dng)
    samples=[]
    for y,x in [(0,0),(0,d.width-1),(d.height//2,d.width//2),(d.height-1,d.width-1)]:
        raw=int(d.raw_row(y)[x]); samples.append({'y':y,'x':x,'raw':raw,'gain':d.gain_at(y,x),'stage2':d.stage2_sample(y,x,raw)})
    print(json.dumps({'file':d.path.name,'sha256':d.source_sha256,'width':d.width,'height':d.height,
                      'black_phase':d.black_phase,'white_level':d.white_level,'cfa_pattern':list(d.cfa_pattern),
                      'opcode_list2_bytes':len(d.opcode_list2),'opcode_list2_sha256':d.opcode_list2_sha256,
                      'gain_maps':len(d.gain_maps),'samples':samples},indent=2))
