# TinyENV nRF Sensor (Matter + Thread)

This repo customizes Nordic's Matter Temperature Sensor sample for the Seeed XIAO nRF52840.

## Current Functionality

- **SHT4x sensor on I2C0** (real temperature + humidity).
- **Battery voltage sense** using the XIAO nRF52840 VBAT circuit:
  - **Enable pin**: P0.14 (drive low to enable)
  - **ADC input**: P0.31 (AIN7)
  - **Divider**: 1 MΩ / 510 kΩ
- **Wake button (optional)**: add a `wake_btn` alias in the devicetree overlay to enable a GPIO wake event.
- **Sleep logging toggle**: set `kEnableSleepLogs = true` in `src/app_task.cpp` to log sleep/wake cycles.
- **Diagnostic mode** (`CONFIG_TINYENV_DIAGNOSTIC_MODE=y` in `prj.conf`):
  - Persists boot/watchdog/thread/sensor/ADC counters under `tinyenv/diag/*`.
  - Captures and stores `RESETREAS` at boot.
  - Enables watchdog reset recovery tracking.
  - Stores periodic health snapshots (uptime + Thread role).
  - Reboots if a commissioned node remains Thread-detached longer than the configured threshold.

## Branches

### `main` (MCUboot)

Builds MCUboot + signed application via sysbuild.

Outputs:
- `build/xiao_ble/mcuboot/zephyr/zephyr.uf2` (MCUboot)
- `build/xiao_ble/nrf-TinyENV/zephyr/zephyr.uf2` (signed app)
- `build/xiao_ble/merged.hex` (MCUboot + app)

Flashing:
- If MCUboot is already installed, you can copy the **signed app UF2** over USB.
- If MCUboot is **not** installed, you need SWD once to flash `build/xiao_ble/merged.hex`.

### `uf2` (UF2-only)

Builds a UF2 app without MCUboot/OTA for the stock UF2 bootloader.

Output:
- `build/xiao_ble_uf2_app/zephyr/zephyr.uf2`
- Low-power profile output: `build/xiao_ble_uf2_lowpower/zephyr/zephyr.uf2`

Build commands (from repo root):

```sh
PATH=/opt/nordic/ncs/toolchains/322ac893fe/bin:$PATH \
ZEPHYR_TOOLCHAIN_VARIANT=zephyr \
ZEPHYR_SDK_INSTALL_DIR=/opt/nordic/ncs/toolchains/322ac893fe/opt/zephyr-sdk \
/opt/homebrew/bin/cmake -DWEST_PYTHON=/opt/nordic/ncs/toolchains/322ac893fe/opt/python@3.12/bin/python3.12 \
  -DZEPHYR_BASE=/opt/nordic/ncs/v3.2.1/zephyr \
  -B./build/xiao_ble_uf2_app -GNinja \
  -DBOARD=xiao_ble -S. \
  -DCONF_FILE=config/app/prj.conf \
  -DEXTRA_CONF_FILE=config/app/prj_uf2.conf

PATH=/opt/nordic/ncs/toolchains/322ac893fe/bin:$PATH \
/opt/homebrew/bin/cmake --build ./build/xiao_ble_uf2_app
```

Low-power measurement build:

```sh
./scripts/build_uf2_lowpower.sh
```

UART-only serial/log build (no USB CDC):

```sh
./scripts/build_uf2_uart.sh
```

Notes:
- `./scripts/build_uf2.sh` remains the debug-friendly UF2 profile.
- `./scripts/build_uf2_lowpower.sh` adds `config/app/prj_lowpower.conf` to enable PM and disable console/log/shell/LED scan activity for current measurements.
- `./scripts/build_uf2_uart.sh` builds a UF2 image with **UART-only** console/logging:
  - overlays `boards/xiao_ble_uart_console.overlay`
  - disables USB device + CDC in `config/app/prj_uf2_uart.conf`
  - routes Zephyr console/shell/log transport to `uart0` (TX=D6, RX=D7, 115200 baud)
- Shell command `diag_dump` prints persisted diagnostic keys from `tinyenv/diag/*`.
- Both scripts use `ccache` by default. Set `CCACHE_DISABLE=1` to bypass cache.

## Repository Layout

- `config/app/`: application config overlays (`prj*.conf`).
- `config/sysbuild/`: sysbuild config overlays.
- `config/pm/`: partition manager static files for xiao_ble builds.
- `boards/`: xiao_ble board overlays used by this project.
- `docs/`: project notes and handoff docs.
- `scripts/`: reproducible local build wrappers.

## TODO

- Research the Matter Low Power (0x0508) cluster before enabling it in ZAP.
- Consider adding atmospheric pressure sensing/cluster support.
