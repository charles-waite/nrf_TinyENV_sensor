# nrf-TinyENV Project Instructions

## Project scope

- App root: repository root. Target: XIAO nRF52840 (`xiao_ble`), using Zephyr/NCS.
- Distribution: low-volume hobbyist assembly sales with publicly available
  source and enclosure designs. Thread/Matter certification is outside the
  current scope. Assess practical security and reliability separately from
  certification requirements. Secure-boot and debug-protection decisions
  remain open.
- Use the `zephyr-build-debug` skill for firmware build/debug tasks.

## Build workflow

- Use `scripts/build_uf2.sh`; it configures the toolchain and profile.
  Do not substitute a bare `west build` unless investigating the wrapper.
- Default to production low-power UF2. Select diagnostics when needed:

  | Profile | Command | UF2 output |
  | --- | --- | --- |
  | Production low-power | `scripts/build_uf2.sh` | `build/xiao_ble_uf2_app/zephyr/zephyr.uf2` |
  | USB diagnostics | `scripts/build_uf2.sh --diag-usb` | `build/xiao_ble_uf2_diag_usb/zephyr/zephyr.uf2` |
  | UART diagnostics | `scripts/build_uf2.sh --diag-uart` | `build/xiao_ble_uf2_diag_uart/zephyr/zephyr.uf2` |

- Allow several minutes for builds; silence alone does not indicate failure.
- Prefer incremental builds. Prefix the selected command with `CLEAN_BUILD=1`
  when configuration/toolchain changes or stale-cache evidence justify a clean
  build. The wrapper also detects SDK changes.
- Use `CCACHE_DISABLE=1` when diagnosing inconsistent builds.
- Release packaging: `scripts/package_release.sh [version]`.
  Packaging success does not establish hardware verification.
- If investigating `west` directly, its known location is
  `/opt/nordic/ncs/toolchains/322ac893fe/Cellar/python@3.12/3.12.4/Frameworks/Python.framework/Versions/3.12/bin/west`.

## Hardware constraints

- Preserve I2C0 on D4/D5 in `boards/xiao_ble.overlay` unless explicitly
  asked to change it.
- VBAT sense: P0.14 enable, active-low; P0.31 input, AIN7.
- Current firmware treats LEDs as active-high.
- UART diagnostic transport: D6 TX, D7 RX, 115200 baud.

## Change boundaries

- Before substantive firmware/configuration edits, briefly identify the
  intended files, protected scope, and validation command.
- Do not modify root cleanup/organization files during feature work unless
  requested.
- Do not modify `sysbuild/` for UF2-only tasks.
- Never commit `README.md` unless explicitly requested.
- Never stage untracked experimental files unless explicitly requested.
- Show `git status --short` before committing and review the staged diff.

## Verification

- For firmware/configuration changes, build the affected profile. Check
  production and relevant diagnostic profiles when shared changes affect both.
- Documentation-only edits require content and diff review, not a build.
- For diagnostic changes, inspect the resolved configuration for the intended
  shell/log transport and relevant commands, including `diag_dump`.
- Report changed files, build outcome, UF2 path, and relevant warnings when
  a build was performed.
- Separately report hardware checks performed and what remains unverified.
  Do not infer successful flashing, commissioning, sensor operation, or power
  behavior from compilation alone.
