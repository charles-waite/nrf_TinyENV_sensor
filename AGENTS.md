# Project Agent Notes

- Builds can take several minutes; use longer timeouts for build commands.
- `west` is available at `/opt/nordic/ncs/toolchains/322ac893fe/Cellar/python@3.12/3.12.4/Frameworks/Python.framework/Versions/3.12/bin/west`.
- I2C on XIAO nRF52840 is currently working via `i2c0` mapped to D4/D5 in `boards/xiao_ble.overlay`.

## Codex Zephyr Workflow
- Use the global skill `zephyr-build-debug` for Zephyr/NCS tasks in this repo.
- App root default: `/Users/cwaite/Documents/nrf-TinyENV`.
- Preferred `west` path: `/opt/nordic/ncs/toolchains/322ac893fe/Cellar/python@3.12/3.12.4/Frameworks/Python.framework/Versions/3.12/bin/west`.
- Default board if not specified by the task: `xiao_ble`.
- Do not change XIAO I2C mapping (`i2c0` on D4/D5 in `boards/xiao_ble.overlay`) unless explicitly requested.
- Default verification command:
  `/opt/nordic/ncs/toolchains/322ac893fe/Cellar/python@3.12/3.12.4/Frameworks/Python.framework/Versions/3.12/bin/west build -b xiao_ble -s /Users/cwaite/Documents/nrf-TinyENV -d /Users/cwaite/Documents/nrf-TinyENV/build`
- Use clean reconfigure (`-p always`) only when config/toolchain/board settings changed or build cache is suspected stale.

## Branch Safety
- Never commit `README.md` unless explicitly requested.
- Never stage untracked experimental files unless explicitly requested.
- Always show `git status --short` before commit.

## Build Defaults
- Default build target is UF2 debug: `scripts/build_uf2.sh`.
- For UART-only, use `scripts/build_uf2_uart.sh`.
- Use `CCACHE_DISABLE=1` when diagnosing build inconsistencies.

## Scope Control
- Do not modify root cleanup/organization files during feature work unless asked.
- Do not touch `sysbuild/` for UF2-only tasks.

## Verification Checklist
- After changes: build, report output UF2 path, list changed files, note warnings.

## Known Board Facts
- I2C is `i2c0` on D4/D5.
- VBAT sense is P0.14 (enable, active low) + P0.31 (AIN7).
- LED polarity is active-high in the current firmware.

## Useful Custom Skills To Add
- `nrf-build-verify`: runs the correct build script, captures warnings/errors, confirms output artifact path.
- `safe-commit`: enforces staged-file review plus commit scope guardrails.
- `diag-mode-check`: confirms diagnostic Kconfig/prj settings and shell command availability (`diag_dump`).

## Prompt Guideline To Prevent Repeated Mistakes
- Before editing, restate scope in 3 bullets:
  - files to modify
  - files explicitly not to modify
  - build command to validate
