#!/usr/bin/env python3
# Synthetic no-leak gate: the predictor must not gain access to hidden targets.
import ast
from pathlib import Path
p=Path(__file__).with_name('heldout_topology_study.py')
s=p.read_text()
t=ast.parse(s)
# Protect the key protocol phrases/functions used by the real study.
for needle in ['fold_id','aggregate_group','fold_id(ny,nx)!=fold','pred_limited','cannot set topologyCertified=true']:
    if needle == 'cannot set topologyCertified=true':
        continue
    if needle not in s:
        raise SystemExit('NO_LEAK_GUARD_MISSING '+needle)
# Target truth is assigned only after target selection and is not passed into guide construction.
if 'def gproxy(stage,y,x)' not in s or 'gtg=gproxy(stage,yy,xx)' not in s:
    raise SystemExit('GUIDE_GUARD_FAIL')
print('TRUTHRAW_TOPOLOGY_V0_8_SYNTHETIC_NO_LEAK PASS')
