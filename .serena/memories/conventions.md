# Conventions

- Follow `AGENTS.md` for branch safety, build defaults, verification, and board facts.
- Before editing, state: files to modify, files explicitly not to modify, build command to validate.
- Never commit `README.md` unless explicitly requested; never stage untracked experimental files unless explicitly requested; show `git status --short` before commit.
- Keep feature changes separate from root cleanup/reorg churn.
- For UF2-only tasks, do not touch `sysbuild/`.
- Prefer script-driven builds over ad hoc CMake commands. Scripts are bash (`#!/usr/bin/env bash`) and use `set -euo pipefail`.
- Do not change XIAO `i2c0` D4/D5 mapping unless explicitly requested.
- Matter model changes must keep ZAP, generated artifacts, Kconfig identity, and runtime attribute writes aligned.
- Logging/diagnostic behavior is config-driven: `TINYENV_VERBOSE_LOGS`, `TINYENV_DIAGNOSTIC_MODE`, and shell `diag_dump` matter for stability investigations.