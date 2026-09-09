#!/usr/bin/env python3
from pathlib import Path
import importlib.util, math, struct, tempfile, sys
MOD=Path(__file__).resolve().parents[1]/'reference'/'dng_stage2_reference_v0_4.py'
spec=importlib.util.spec_from_file_location('ref',MOD); ref=importlib.util.module_from_spec(spec);sys.modules['ref']=ref;spec.loader.exec_module(ref)

def be_u32(v): return struct.pack('>I',v)
def be_i32(v): return struct.pack('>i',v)
def be_f64(v): return struct.pack('>d',v)
def be_f32(v): return struct.pack('>f',v)
def opcode_list(h=4,w=4):
    pays=[]
    for k,(t,l) in enumerate([(1,1),(0,1),(1,0),(0,0)]):
        p=be_i32(t)+be_i32(l)+be_i32(h)+be_i32(w)+be_u32(0)+be_u32(1)+be_u32(2)+be_u32(2)
        p+=be_u32(2)+be_u32(2)+be_f64(1.0)+be_f64(1.0)+be_f64(0.0)+be_f64(0.0)+be_u32(1)
        for z in [1+k,2+k,3+k,4+k]: p+=be_f32(float(z))
        pays.append(p)
    return be_u32(4)+b''.join(be_u32(9)+be_u32(0x01030000)+be_u32(1)+be_u32(len(p))+p for p in pays)

def make_tiff(path):
    W=H=4; op=opcode_list(); raw_rows=[struct.pack('<4H',*r) for r in ([10,20,30,40],[50,60,70,80],[90,91,92,93],[94,95,96,97])]
    # tag,type,count,data-bytes. Type semantics are TIFF little-endian.
    blocks=[]
    def short(*v): return struct.pack('<'+'H'*len(v),*v)
    def long(*v): return struct.pack('<'+'I'*len(v),*v)
    rats=b''.join(struct.pack('<II',v,1) for v in (10,20,30,40))
    tags=[(256,4,1,long(W)),(257,4,1,long(H)),(258,3,1,short(16)),(259,3,1,short(1)),(262,3,1,short(32803)),
          (273,4,H,b'__STRIPOFFSETS__'),(277,3,1,short(1)),(278,4,1,long(1)),(279,4,H,long(*([W*2]*H))),
          (33421,3,2,short(2,2)),(33422,1,4,bytes([2,1,1,0])),(50714,5,4,rats),(50717,4,1,long(100)),(51009,7,len(op),op)]
    n=len(tags); ifd_size=2+12*n+4; data_start=8+ifd_size
    extra=bytearray(); entry_bytes=[]; strip_placeholder_offset=None
    for tag,typ,count,data in tags:
        if data==b'__STRIPOFFSETS__': data=bytes(4*H); is_strip=True
        else: is_strip=False
        if len(data)<=4: val=data+b'\0'*(4-len(data))
        else:
            off=data_start+len(extra);val=long(off)
            if is_strip: strip_placeholder_offset=len(extra)
            extra+=data
            if len(extra)&1: extra+=b'\0'
        entry_bytes.append(struct.pack('<HHI',tag,typ,count)+val)
    raw_start=data_start+len(extra); offs=[raw_start+i*W*2 for i in range(H)]
    assert strip_placeholder_offset is not None
    extra[strip_placeholder_offset:strip_placeholder_offset+4*H]=long(*offs)
    blob=b'II'+short(42)+long(8)+short(n)+b''.join(entry_bytes)+long(0)+bytes(extra)+b''.join(raw_rows)
    path.write_bytes(blob)

with tempfile.TemporaryDirectory() as td:
    p=Path(td)/'tiny.dng';make_tiff(p);d=ref.read_classic_dng(p)
    assert (d.width,d.height,d.black_phase,d.white_level)==(4,4,(10.0,20.0,30.0,40.0),100.0)
    assert d.cfa_pattern==bytes([2,1,1,0]) and len(d.gain_maps)==4
    assert list(d.raw_row(1))==[50,60,70,80]
    s=d.stage2_sample(1,1,60); expected=((60-40)/(100-40))*d.gain_at(1,1); assert abs(s-expected)<1e-12
    bad=bytearray(d.opcode_list2);bad[7]=8
    try: ref.parse_opcode_list2(bytes(bad)); raise AssertionError('non-GainMap accepted')
    except ref.DngStage2Error: pass
print('TruthRaw v0.4 stdlib DNG/Stage-2 reference: PASS')
