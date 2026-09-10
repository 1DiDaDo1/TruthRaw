# Technical Backplane v0.1 — Failure History

## F1 — detached local working directory

**Classification:** tooling/harness failure before compilation.

The first setup command started with its cwd inside the candidate directory and then removed/recreated that same directory. The shell remained attached to the deleted inode, so later relative compiler paths could not see the newly created files.

**Resolution:** rerun the exact same source bytes from stable parent `/mnt/data`. GCC, Clang and ASan/UBSan then all passed. No production source or test gate was weakened.

## F2 — missing empty seal directories

**Classification:** candidate-packaging failure before verification/CI.

The first repository-layout copy brought across populated source/test directories but not the previously empty `evidence`, `state`, and `tools` directories. Writing the first evidence file therefore stopped with `No such file or directory`.

**Resolution:** create the intended directory structure explicitly before writing sealed files. No module source/test byte changed.

## F3 — first repository seal mismatch in test comments

**Classification:** byte-transfer/seal failure before compilation.

GitHub run `34539436405` failed in the integrity step on all three jobs because the staged test file omitted one explanatory comment line present in the locally validated bytes. The executable test logic was unchanged, but byte identity is part of the promotion contract.

**Resolution:** restore the test file to the exact locally validated bytes and regenerate the manifest. The verifier was not weakened and compilation remained blocked until the seal matched.

## F4 — final evidence report newline mismatch

**Classification:** byte-transfer/seal failure before compilation.

GitHub run `34539681903` failed in the integrity step because the staged `REPORT_v0_1.md` normalized one blank-line difference relative to the locally sealed evidence report. No native source, test, state semantic, or metric value changed.

**Resolution:** restore the exact sealed report bytes, preserve this failure, regenerate the manifest, and rerun the unchanged compiler/sanitizer gates.
