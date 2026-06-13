# Task Completion

- Run the relevant build script for code/config changes: default `scripts/build_uf2.sh`, UART-only `scripts/build_uf2_uart.sh`, diagnostic `scripts/build_uf2.sh --diag`.
- For config, overlay, board, or toolchain changes, use `CLEAN_BUILD=1`; for cache inconsistencies, add `CCACHE_DISABLE=1`.
- Report the output UF2 path and any warnings that matter.
- Run or report `git status --short` after changes; before commits, show staged scope and avoid unrelated/untracked experimental files.
- If shell diagnostics are relevant, confirm `diag_dump` remains available when `CONFIG_SHELL=y`.
- Generated ZAP file edits require regeneration verification; direct generated-file edits are not an acceptable completion state.