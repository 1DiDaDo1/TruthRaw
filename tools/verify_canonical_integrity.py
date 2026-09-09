#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, zlib
ROOT=Path(__file__).resolve().parents[1]

def gitblob(p):
    b=p.read_bytes()
    return hashlib.sha1(b'blob '+str(len(b)).encode()+b'\0'+b).hexdigest()

EXPECTED_BLOBS={
 'README.md':'341cdf2903e626ffd7b53d53de5edbb4fced9fc2',
 'START_HERE_NEW_CHAT.md':'fab115fe4303554ecd1383dcb1aaf1dc10724812',
 'docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md':'176f826a5513c5bc7075127d4419c2186ed7a3e9',
 'docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md':'edb6d5e96026a65512416243ef85c1c5d2f37c9e',
 'state/CURRENT_CANONICAL_STATE_2026-09-08.json':'13571f07246a12401d590757f54bcfe0c980d38b',
 'state/CURRENT_CANONICAL_STATE_2026-09-09.json':'db0b864667f778bbb827a333264a8fb603bf2c55',
 'state/REPOSITORY_MIGRATION_STATUS.json':'b7d9a2ef63bcf1e824e90ddcf56d560126d87011',
 'docs/PROJECT_STATE_AUDIT_2026-09-08.md':'069220b82de4f50c42e214b4d9213e66e9423cc8',
 'docs/research/zero-line-dynamic-range-v0.1/implementation/IMPLEMENTATION_INDEX_v0_1.json':'25a9411ef00c4f1fe35436ae6019afa3105699fa',
 'docs/research/zero-line-dynamic-range-v0.1/implementation/REPORT_v0_1.md':'8126533d3e229ee16130554e3bdf42aae2a6ed2e',
 'docs/research/zero-line-dynamic-range-v0.1/implementation/test_truthrange_core_v0_1.py':'e8e6017761c769b83bf2c6499f47b89222698f26',
 'docs/research/zero-line-dynamic-range-v0.1/implementation/truthrange_core_v0_1.py':'a6325d3a588416a8c66b0ec47438409f9adc6db8',
 'docs/research/truthrange-latent-binding-v0.2/CMakeLists.txt':'daf81224132867deab4f825947d2d3444de46afd',
 'docs/research/truthrange-latent-binding-v0.2/IMPLEMENTATION_INDEX_v0_2.json':'83814bc1fbe7f58304c6a69f58cd2774b99134fa',
 'docs/research/truthrange-latent-binding-v0.2/README.md':'71456ef05c2d57b58d65771ae541b1f5149cced6',
 'docs/research/truthrange-latent-binding-v0.2/REAL_BNCAM_SELF_GAUGE_RESULTS_v0_2.json':'feb820e5f78a083db3afba17a09b4449bb8b6e68',
 'docs/research/truthrange-latent-binding-v0.2/REPORT_v0_2.md':'e6f37ddea5ba7fd776c5a83b92096ab1b5e2b6da',
 'docs/research/truthrange-latent-binding-v0.2/SHA256_MANIFEST_v0_2.json':'77b6926cf1898c5870105adcf1710ae7ddd0cc7b',
 'docs/research/truthrange-latent-binding-v0.2/TEST_OUTPUT_v0_2.txt':'fb86f4d63fddc189a59412aef4d1d6950c81bdf7',
 'docs/research/truthrange-latent-binding-v0.2/native/truthrange_latent_v0_2.cpp':'2bf0aecd05c0b36440d14300d9ea973ffa58c62a',
 'docs/research/truthrange-latent-binding-v0.2/native/truthrange_latent_v0_2.h':'90b15bb5e4f1f9daee6babe39730f88882ecce0e',
 'docs/research/truthrange-latent-binding-v0.2/tests/test_truthrange_latent_v0_2.cpp':'676a984146bd034c56a634f15bdeb10456abd969',
 '.github/workflows/canonical-integrity.yml':'3d1213b5fe9c0f07fabbdc1c5711bfeeeebb5297',
 'docs/research/truthrange-dense-uncertainty-v0.3/CMakeLists.txt':'fe3d14a6bc837572d9858880caaa434c57379bd6',
 'docs/research/truthrange-dense-uncertainty-v0.3/IMPLEMENTATION_INDEX_v0_3.json':'306346816c0fbf4270013a2efe2cf4c4c49dcabd',
 'docs/research/truthrange-dense-uncertainty-v0.3/README.md':'1e7cc580f3ecbd66ab915e0ab80d7049bb7f5484',
 'docs/research/truthrange-dense-uncertainty-v0.3/REAL_BNCAM_NOISEPROFILE_RECON_v0_3.json.zlib':'e2d36b85ab868e49eaa490ed212a8df170058c10',
 'docs/research/truthrange-dense-uncertainty-v0.3/REPORT_v0_3.md':'3af36fe71c599571e62ca24eab8eeda466d0b0a0',
 'docs/research/truthrange-dense-uncertainty-v0.3/SHA256_MANIFEST_v0_3.json':'d03f076145c375a617ea9182ecdd8d2bf28c9c7b',
 'docs/research/truthrange-dense-uncertainty-v0.3/TEST_OUTPUT_v0_3.txt':'93324136d8336d860742e98eb8c8d307c6944bda',
 'docs/research/truthrange-dense-uncertainty-v0.3/native/truthrange_dense_uncertainty_v0_3.cpp':'ceb438898dcaf5d5820d961d13d423c4672b3591',
 'docs/research/truthrange-dense-uncertainty-v0.3/native/truthrange_dense_uncertainty_v0_3.h':'bd4d85af9ddc472fdc72aee38d7636704989ead8',
 'docs/research/truthrange-dense-uncertainty-v0.3/tests/test_truthrange_dense_uncertainty_v0_3.cpp':'c67019fd7e8ad13f89e3d0d27df7024f9ffdf87b',
 'canonical/reconstruction/v4.7i/native/include/truthraw/core.h':'cfb9fd42bc310ddb4fd16ee8f26c3ed554a0f92a',
 'canonical/reconstruction/v4.7i/native/src/core.cpp':'f79b951ba54cff08db400023e528e4eb91909ba6',
 'canonical/detail/v4.7j/native/include/truthraw/core.h':'ff5a6c976be8228d990aa4ee577db8613d8c9bd1',
 'canonical/detail/v4.7j/native/src/core.cpp':'4c818fed9644605693e93e35c88bedeee5360cef',
 'canonical/detail/v4.7j/VALIDATION_v4_7j.json':'73593dcf546cb4ec8da3209963ea4717c331f7fe',
 'canonical/output-acutance/v4.7k/STATUS_v4_7k.json':'4b1af55b6dd1e754c434838974b47084c94e6cf4',
 'canonical/output-acutance/v4.7k/VALIDATION_v4_7k.json':'1c67dfc25beca27b4fe1de4079b2a9e0d1e64310',
 'canonical/output-acutance/v4.7k/native/CMakeLists.txt':'ffb243278c361b5b71bef6ee3e548a6f2b518dfa',
 'canonical/output-acutance/v4.7k/native/output_acutance.cpp':'e0465dadd453427c904d5292abd763397075331d',
 'canonical/output-acutance/v4.7k/native/output_acutance.h':'e6982fb702ddb8b0f60c451a3b3315671afb823e',
 'canonical/ptc/v1.1/python/truthraw_ptc.py':'92f177f2292a4c194b698916c951d3c751766699',
 'canonical/uncertainty/v5.0g/source/uncertainty_core_v5_0g.py':'3a833147f892a970c60175a7ebe4ab1e8cf0221c',
 'canonical/uncertainty/v5.0g/source/prospective_holdout_v5_0g.py':'8f1f57f8d965b3a7067aed51ec60fe8586f196df',
 'canonical/uncertainty/v5.0g/source/finalize_uncertainty_v5_0g.py':'7832f3da0cb960f74f025f1c2c9fbd9f2c772a62',
 'canonical/uncertainty/v5.0g/source/fit_uncertainty_v5_0g.py':'0e3e8be37e890f5a3cf6b2e25ae9374e62aaa2b6',
 'canonical/uncertainty/v5.0g/source/train_uncertainty_v5_0g.py':'e9a9cf08c54637111bcfd0b6b8a29f0566e46075',
 'canonical/uncertainty/v5.0g/runtime/runtime_parity_test.cpp':'923f581612cb1632cc352659acd68d90dc9cec01',
}

for rel,exp in EXPECTED_BLOBS.items():
    p=ROOT/rel
    if not p.is_file() or gitblob(p)!=exp:
        raise SystemExit(f'FAIL blob {rel}')
    print('PASS blob',rel)

# v0.3 real BnCam evidence is stored losslessly compressed; verify both container and original JSON bytes.
v03z=ROOT/'docs/research/truthrange-dense-uncertainty-v0.3/REAL_BNCAM_NOISEPROFILE_RECON_v0_3.json.zlib'
v03c=v03z.read_bytes()
if len(v03c)!=4712 or hashlib.sha256(v03c).hexdigest()!='b52951c90196d8d5a7349f90a782bd8a985cbe7d78e47d0b9a68c7165e671c66':
    raise SystemExit('FAIL v0.3 compressed real evidence')
v03raw=zlib.decompress(v03c)
if len(v03raw)!=21530 or hashlib.sha256(v03raw).hexdigest()!='34d8fcb694c23ecee55a442dc15f34844bebbecd91dd39874e7dc21db9b7b78c':
    raise SystemExit('FAIL v0.3 uncompressed real evidence')
print('PASS v0.3 real evidence zlib -> original JSON')

v5e=ROOT/'canonical/unified-material/v5.0e'
data=b''.join((v5e/'source-parts'/f'unified_material_v5_0e.py.part0{i}').read_bytes() for i in range(1,5))
if len(data)!=26771 or hashlib.sha256(data).hexdigest()!='7a60703c47a981da8aa4f9613545e1615a6a25cdbb3a8347ed1c1a4df076b1e0':
    raise SystemExit('FAIL v5.0e source')
print('PASS v5.0e source reconstruction')

u=ROOT/'canonical/uncertainty/v5.0g'
prov=json.loads((u/'EVIDENCE_PROVENANCE_v5_0g.json').read_text())
for rec in prov['files']:
    b=b''.join((u/x).read_bytes() for x in rec['parts'])
    if rec['encoding']=='zlib-concat':
        if len(b)!=rec['compressed_bytes'] or hashlib.sha256(b).hexdigest()!=rec['compressed_sha256']:
            raise SystemExit('FAIL compressed '+rec['target'])
        b=zlib.decompress(b)
    if len(b)!=rec['bytes'] or hashlib.sha256(b).hexdigest()!=rec['sha256']:
        raise SystemExit('FAIL evidence '+rec['target'])
    print('PASS evidence',rec['target'])

for p in ROOT.rglob('*'):
    if p.is_file() and p.suffix.lower() in {'.dng','.rawsensor','.raw10','.raw12','.apk','.npz'}:
        raise SystemExit('FAIL forbidden payload '+str(p.relative_to(ROOT)))

print('TruthRaw canonical integrity: PASS')
