#!/usr/bin/env python3
from __future__ import annotations
import hashlib, json, pathlib, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'SHA256_MANIFEST_v0_6.json'

def sha256(path: pathlib.Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda: f.read(1024*1024), b''):
            h.update(b)
    return h.hexdigest()

def main() -> int:
    m=json.loads(MANIFEST.read_text(encoding='utf-8'))
    if m.get('schema') != 'TruthRawCameraRgbCovarianceSha256Manifest/0.6':
        raise SystemExit('wrong manifest schema')
    expected={x['path']:x for x in m['files']}
    for rel, rec in expected.items():
        p=ROOT/rel
        if not p.is_file(): raise SystemExit(f'missing: {rel}')
        if p.stat().st_size != rec['bytes']: raise SystemExit(f'byte-size mismatch: {rel}')
        got=sha256(p)
        if got != rec['sha256']: raise SystemExit(f'sha256 mismatch: {rel}: {got}')
    # Semantic anti-overclaim checks are intentionally simple and fail closed.
    h=(ROOT/'native/camera_rgb_covariance_v0_6.h').read_text()
    c=(ROOT/'native/camera_rgb_covariance_v0_6.cpp').read_text()
    a=(ROOT/'native/dense_uncertainty_v0_3_adapter_v0_6.cpp').read_text()
    required=[
      'Unknown values MUST remain NaN',
      'quantile-only MUST NOT become variance' if False else 'Quantile-only reconstructed-channel uncertainty is not converted into variance',
    ]
    if 'Unknown values MUST remain NaN' not in h: raise SystemExit('missing unknown-is-NaN contract')
    if 'unknown covariance must be NaN, never zero-by-default' not in c: raise SystemExit('missing no-independence guard')
    if 'v5.0g quantile-only entry must not be promoted to sigma/variance' not in a: raise SystemExit('missing quantile anti-promotion guard')
    if 'covarianceOut.covarianceKnownMask != 0u' not in a: raise SystemExit('missing adapter off-diagonal guard')
    print('TRUTHRAW_CAMERA_RGB_COVARIANCE_V0_6_INTEGRITY_PASS')
    print(f'files={len(expected)}')
    return 0
if __name__ == '__main__': sys.exit(main())
