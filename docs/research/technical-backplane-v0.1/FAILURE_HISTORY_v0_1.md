# Technical Backplane v0.1 — Failure History

## F1 — detached local working directory

**Classification:** tooling/harness failure before compilation.

The first setup command started with its cwd inside the candidate directory and then removed/recreated that same directory. The shell remained attached to the deleted inode, so later relative compiler paths could not see the newly created files.

**Resolution:** rerun the exact same source bytes from stable parent `/mnt/data`. GCC, Clang and ASan/UBSan then all passed. No production source or test gate was weakened.

## F2 — missing empty seal directories

**Classification:** candidate-packaging failure before verification/CI.

The first repository-layout copy brought across populated source/test directories but not the previously empty `evidence`, `state`, and `tools` directories. Writing the first evidence file therefore stopped with `No such file or directory`.

**Resolution:** create the intended directory structure explicitly before writing sealed files. No module source/test byte changed.
