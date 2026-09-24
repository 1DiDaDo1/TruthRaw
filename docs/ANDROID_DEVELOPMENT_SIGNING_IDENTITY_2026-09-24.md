# D.RAW stable Android development signing identity

Status: active build contract from 2026-09-24.

## Purpose

D.RAW development/test APKs must keep one stable Android signing identity so installed builds can be updated in place and device-level app identity state is not reset on every CI run.

The private key MUST NOT be committed to this public repository.

## Admitted development certificate

Certificate SHA-256:

`a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

Alias expected by the current generated development key:

`draw-dev`

This identity is development/test-only. It is not a production/release signing authority.

## GitHub Actions secret contract

The Android workflow expects these repository Actions secrets:

- `DRAW_DEBUG_KEYSTORE_B64`
- `DRAW_DEBUG_STORE_PASSWORD`
- `DRAW_DEBUG_KEY_ALIAS`
- `DRAW_DEBUG_KEY_PASSWORD`

The workflow decodes the keystore only into the ephemeral GitHub runner and verifies the final APK certificate fingerprint before publishing the artifact.

## Migration rule

Existing APKs signed by prior ephemeral GitHub debug keys cannot be updated by this identity. One uninstall/reinstall is required when adopting the stable D.RAW development key. After that migration, future CI builds signed by this same identity can update in place.

No RAW, Scientific Master, Open Scene, TruthNegative, color, reconstruction or export authority is affected by Android package signing.
