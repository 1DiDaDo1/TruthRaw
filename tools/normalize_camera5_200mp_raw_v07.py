#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, os, struct
import numpy as np
from pathlib import Path

W,H=16320,12288
ACTIVE_ROW=W*2
EXPECTED=W*H*2


def sha256_file(p:Path, chunk=8<<20):
    h=hashlib.sha256()
    with p.open('rb') as f:
        while True:
            b=f.read(chunk)
            if not b: break
            h.update(b)
    return h.hexdigest()


def normalize(manifest:dict, source:Path, out:Path):
    ro=manifest.get('raw_output') or {}
    if ro.get('format')!='RAW_SENSOR': raise ValueError('v0.7 normalizer accepts RAW_SENSOR only')
    if (int(ro.get('width',0)),int(ro.get('height',0)))!=(W,H): raise ValueError('not Camera-5 16320x12288 target')
    row_stride=int(ro.get('row_stride',0)); pixel_stride=int(ro.get('pixel_stride',0))
    if pixel_stride!=2: raise ValueError(f'RAW_SENSOR pixel_stride must be 2, got {pixel_stride}')
    if row_stride<ACTIVE_ROW: raise ValueError('row_stride smaller than active row')
    src_size=source.stat().st_size
    min_access=row_stride*(H-1)+ACTIVE_ROW
    if src_size<min_access: raise ValueError(f'source truncated: {src_size} < {min_access}')
    source_hash=sha256_file(source)
    if source_hash!=ro.get('payload_sha256'): raise ValueError('source SHA-256 != manifest payload_sha256')
    if manifest.get('source_identity_sha256')!=source_hash: raise ValueError('source identity hash mismatch')

    h=hashlib.sha256(); minv=65535; maxv=0; over_white=0; below_black=0; samples=0
    white=int((manifest.get('camera_characteristics') or {}).get('white_level',1023) or 1023)
    blackvals=(manifest.get('camera_characteristics') or {}).get('black_level_pattern') or [64,64,64,64]
    blackmin=min(map(int,blackvals))
    with source.open('rb',buffering=0) as f, out.open('wb',buffering=0) as g:
        for y in range(H):
            f.seek(y*row_stride)
            row=f.read(ACTIVE_ROW)
            if len(row)!=ACTIVE_ROW: raise ValueError(f'short row {y}')
            g.write(row); h.update(row)
            # Diagnostic scan without full-frame allocation.
            vals=np.frombuffer(row,dtype='<u2')
            rvmin=int(vals.min()); rvmax=int(vals.max())
            minv=min(minv,rvmin); maxv=max(maxv,rvmax)
            over_white += int(np.count_nonzero(vals>white))
            below_black += int(np.count_nonzero(vals<blackmin))
            samples += int(vals.size)
    if out.stat().st_size!=EXPECTED: raise ValueError('canonical output size mismatch')
    return {
      'schema':'TruthRawCamera5CanonicalRawSensor/0.7',
      'classification':'FULL_SENSOR_200MP_CANONICAL_APP_VISIBLE_CFA_MIRROR',
      'source_buffer_file':source.name,'source_buffer_sha256':source_hash,'source_buffer_bytes':src_size,
      'canonical_file':out.name,'canonical_sha256':h.hexdigest(),'canonical_bytes':out.stat().st_size,
      'width':W,'height':H,'samples':samples,'storage':'uint16 little-endian','row_stride':ACTIVE_ROW,'pixel_stride':2,
      'source_row_stride':row_stride,'source_pixel_stride':pixel_stride,'padding_removed_per_row':row_stride-ACTIVE_ROW,
      'white_level':white,'black_level_pattern':blackvals,'min_code':minv,'max_code':maxv,
      'samples_above_white_level':over_white,'samples_below_min_black_level':below_black,
      'scientific_boundary':'Sample-exact canonicalization of the app-visible RAW_SENSOR plane. Row padding, if any, is removed; active sample codes are unchanged. This is not proof of untouched photodiode/ADC output.'
    }


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('manifest');ap.add_argument('source_rawbuffer');ap.add_argument('--out',required=True);ap.add_argument('--report',required=True)
    ns=ap.parse_args()
    m=json.load(open(ns.manifest,'r',encoding='utf-8'))
    r=normalize(m,Path(ns.source_rawbuffer),Path(ns.out))
    Path(ns.report).write_text(json.dumps(r,indent=2),encoding='utf-8')
    print(json.dumps(r,indent=2))
if __name__=='__main__':main()
