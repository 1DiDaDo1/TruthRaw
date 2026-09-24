# D.RAW project rename — 2026-09-24

Status: **CURRENT PROJECT IDENTITY**

The complete project/product identity is renamed from **TruthRaw** to **D.RAW**
from this checkpoint forward.

## Canonical naming rule

- Public project name: **D.RAW**
- Android visible application label: **D.RAW**
- Visible product modes: **D.RAW PURE**, **D.RAW ADVANCED**, **D.RAW PRO**
- New user-facing exported filenames use the `_draw_` brand token.
- New documentation and future research branches should use D.RAW naming.

## Provenance and compatibility rule

The rename is a product/project identity migration, not a rewrite of history.

Existing `TruthRaw` identifiers remain unchanged when they are part of:

- immutable or sealed source evidence;
- historical documents, handoffs, commits, hashes or experiment names;
- serialized schemas, magic strings, container identifiers or provenance records;
- stable Android package/application identifiers where changing them would break
  upgrade/install compatibility;
- stable class/JNI/native symbol identities where a cosmetic rename would create
  needless compatibility risk.

Those legacy identifiers mean **D.RAW historical/compatibility identity** after
this rename; they do not indicate a separate current project.

## Scientific invariants

The rename grants no new scientific authority and changes no pixel math.

All existing invariants remain binding:

- sealed source evidence stays immutable;
- measured, reconstructed, censored, unknown, counterfactual and appearance remain distinct;
- Scientific Master remains separate from presentation/export;
- one physical frame remains one evidence root;
- representation may exceed the source, while knowledge claims may not exceed evidence.

## Repository name

The GitHub repository is currently still hosted under the legacy repository path
`1DiDaDo1/TruthRaw`. Renaming that repository path is an administrative GitHub
operation and is intentionally separate from the source-level identity migration.

## Migration checkpoint

Base branch at migration start:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

Base commit:

`3bae85833afbc34b0052823303fdc6bfc25aa4ef`

Migration branch:

`integration/draw-brand-migration-2026-09-24`
