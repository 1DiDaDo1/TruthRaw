#!/usr/bin/env python3
from pathlib import Path

# Preserve the first v0.31 patch as failed-build provenance. It reconstructed and transformed the
# scientific path correctly, but its final bundle assertion missed the uppercase V030 token used in
# the pre-existing bundle filename. Execute the exact first patch with only that deterministic text
# correction injected before compilation.
source_path = Path('tools/patch_fotograaf_v031_int32_value_domain_sweep.py')
source = source_path.read_text()
needle = "s = s.replace('v030', 'v031')\n"
replacement = needle + "s = s.replace('V030', 'V031')\n"
if needle not in source:
    raise SystemExit('v0.31b uppercase-version correction anchor not found')
patched_source = source.replace(needle, replacement, 1)
ns = {}
exec(compile(patched_source, 'patch_fotograaf_v031b_int32_value_domain_sweep.py', 'exec'), ns, ns)
print('v0.31b correction applied: uppercase V030 bundle/file tokens are now V031')
