# Professional RAW Ingress / Decoder ABI v0.1 — Failure History

## F1 — repository manifest used stale verifier hash

**Classification:** candidate-packaging / seal failure before compilation.

The first repository candidate run `34591022370` failed in `Verify sealed module` on all three matrix jobs before any compiler step. The staged verifier bytes matched the locally validated verifier, but `MANIFEST_SHA256.txt` accidentally contained an earlier SHA-256 value for `tools/verify_integrity_v0_1.py` (`30aba7...`) instead of the final verifier hash (`225634...`).

**Resolution:** preserve this failure, regenerate the manifest from the final candidate bytes, and rerun the unchanged GCC/Clang/sanitizer gates. No scientific classifier logic or compiler gate was weakened.

Future failures must record at minimum: source family/model/mode, codec variant, decoder identity, whether failure is parse/decode/topology/admission/resource related, and whether any sample was ever exposed to downstream science before fail-closed rejection.

## F2 — Release tests used `assert` under `-DNDEBUG`

**Classification:** test-harness/compiler-policy failure after seal verification.

Repository run `34591175151` passed `Verify sealed module`, then GCC Release and Clang Release failed. The workflow intentionally compiles Release with `-DNDEBUG -Werror`; the test harness used `assert(...)`, so assertions vanished under `NDEBUG` and the now-unused result variables triggered compiler warnings promoted to errors. The sanitizer configuration, which did not define `NDEBUG`, was unaffected by this specific mechanism.

**Resolution:** replace all `assert` checks with an always-active `require(...)` test helper, keep `-DNDEBUG` and `-Werror` unchanged, regenerate the seal, and rerun all compiler/sanitizer gates. No classifier condition or scientific admission rule was weakened.
