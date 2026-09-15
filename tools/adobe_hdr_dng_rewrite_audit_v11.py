#!/usr/bin/env python3
"""TruthRaw v1.1: Adobe Lightroom HDR DNG rewrite audit contract.

A rewritten DNG may preserve the measured CFA raster exactly while changing the
container and calibration/presentation metadata. This module encodes that
boundary and forbids treating byte-different Adobe DNG metadata as original
source authority without field-by-field comparison.
"""
from __future__ import annotations
from dataclasses import dataclass
from math import isfinite

SHA256_HEX=set('0123456789abcdef')

class DngRewriteAuditError(RuntimeError): pass

def _sha(v:str,name:str)->str:
    v=v.lower()
    if len(v)!=64 or any(c not in SHA256_HEX for c in v):
        raise DngRewriteAuditError(f'{name} must be SHA-256 hex')
    return v

@dataclass(frozen=True)
class AdobeHdrDngRewriteAuditV11:
    original_file_sha256:str
    rewritten_file_sha256:str
    original_bytes:int
    rewritten_bytes:int
    width:int
    height:int
    cfa:str
    original_raw_sample_sha256:str
    rewritten_raw_sample_sha256:str
    raw_mismatch_count:int
    raw_max_abs_difference:int
    raw_min:int
    raw_max:int
    source_white_count:int
    rewritten_raw_compression:str
    tile_width:int
    tile_height:int
    tile_count:int
    new_raw_image_digest_hex:str
    hdr_edit_mode:bool
    hdr_max_value_ev:float
    exposure_2012_ev:float
    whites_2012:float
    tone_curve_name:str
    original_orientation:int
    rewritten_orientation:int
    noise_profile_exact:bool
    opcode_list2_exact:bool
    opcode_list3_exact:bool
    black_level_numeric_exact:bool
    white_level_exact:bool
    as_shot_neutral_max_abs_delta:float
    forward_matrix_max_abs_delta:float
    camera_calibration_max_abs_delta:float
    color_matrix1_max_abs_delta:float
    color_matrix2_max_abs_delta:float
    original_raw_file_digest_present:bool
    original_raw_file_name_present:bool

    def validate(self)->'AdobeHdrDngRewriteAuditV11':
        _sha(self.original_file_sha256,'original_file_sha256'); _sha(self.rewritten_file_sha256,'rewritten_file_sha256')
        _sha(self.original_raw_sample_sha256,'original_raw_sample_sha256'); _sha(self.rewritten_raw_sample_sha256,'rewritten_raw_sample_sha256')
        if self.original_file_sha256==self.rewritten_file_sha256:
            raise DngRewriteAuditError('rewrite audit requires byte-distinct containers')
        if self.original_bytes<=0 or self.rewritten_bytes<=0: raise DngRewriteAuditError('invalid file size')
        if (self.width,self.height,self.cfa)!=(4080,3072,'BGGR'): raise DngRewriteAuditError('unexpected source class')
        if self.raw_mismatch_count<0 or self.raw_max_abs_difference<0: raise DngRewriteAuditError('invalid raw diff')
        if self.raw_min>self.raw_max: raise DngRewriteAuditError('invalid raw range')
        if self.source_white_count<0 or self.source_white_count>self.width*self.height: raise DngRewriteAuditError('invalid white count')
        if self.tile_width<=0 or self.tile_height<=0 or self.tile_count<=0: raise DngRewriteAuditError('invalid tile geometry')
        if len(self.new_raw_image_digest_hex)!=32: raise DngRewriteAuditError('NewRawImageDigest must be 16-byte hex')
        for v in (self.hdr_max_value_ev,self.exposure_2012_ev,self.whites_2012,self.as_shot_neutral_max_abs_delta,
                  self.forward_matrix_max_abs_delta,self.camera_calibration_max_abs_delta,self.color_matrix1_max_abs_delta,
                  self.color_matrix2_max_abs_delta):
            if not isfinite(float(v)): raise DngRewriteAuditError('non-finite audit value')
        return self

    @property
    def cfa_sample_exact(self)->bool:
        self.validate()
        return self.raw_mismatch_count==0 and self.raw_max_abs_difference==0 and self.original_raw_sample_sha256==self.rewritten_raw_sample_sha256

    @property
    def container_exact(self)->bool:
        return False

    @property
    def metadata_source_exact(self)->bool:
        self.validate()
        return False

    @property
    def classification(self)->str:
        if not self.cfa_sample_exact:
            return 'ADOBE_HDR_DNG_REWRITE_CFA_CHANGED_REJECT_AS_SOURCE_DERIVATIVE'
        return 'ADOBE_HDR_DNG_REWRITE_CFA_SAMPLE_EXACT_METADATA_NOT_SOURCE_EXACT'

    @property
    def measured_cfa_authority_may_be_preserved(self)->bool:
        return self.cfa_sample_exact

    @property
    def rewritten_metadata_may_replace_original_calibration_authority(self)->bool:
        return False

    @property
    def adobe_hdr_xmp_is_scientific_evidence(self)->bool:
        return False

    @property
    def scientific_master_writeback_allowed(self)->bool:
        return False

FROZEN_ADOBE_HDR_DNG_REWRITE_V11=AdobeHdrDngRewriteAuditV11(
    original_file_sha256='7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67',
    rewritten_file_sha256='a69e434c53277f612ae3afea265dfdaf5ba5d7b20c5ab8f4c588000b3f1ede1e',
    original_bytes=25106120, rewritten_bytes=7839986, width=4080, height=3072, cfa='BGGR',
    original_raw_sample_sha256='883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c',
    rewritten_raw_sample_sha256='883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c',
    raw_mismatch_count=0, raw_max_abs_difference=0, raw_min=61, raw_max=1023, source_white_count=217,
    rewritten_raw_compression='LOSSLESS_JPEG_SOF3', tile_width=256, tile_height=256, tile_count=192,
    new_raw_image_digest_hex='6faea350add53e0841d80a210d9cf3a0', hdr_edit_mode=True, hdr_max_value_ev=8.0,
    exposure_2012_ev=0.0, whites_2012=2.0, tone_curve_name='Custom', original_orientation=3, rewritten_orientation=1,
    noise_profile_exact=True, opcode_list2_exact=True, opcode_list3_exact=True, black_level_numeric_exact=True,
    white_level_exact=True, as_shot_neutral_max_abs_delta=3.125e-7, forward_matrix_max_abs_delta=5e-5,
    camera_calibration_max_abs_delta=1.25e-5, color_matrix1_max_abs_delta=0.02985,
    color_matrix2_max_abs_delta=0.2094875, original_raw_file_digest_present=False, original_raw_file_name_present=False,
)
