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

Build commands (from repo root):

```sh
PATH=/opt/nordic/ncs/toolchains/322ac893fe/bin:$PATH \
ZEPHYR_TOOLCHAIN_VARIANT=zephyr \
ZEPHYR_SDK_INSTALL_DIR=/opt/nordic/ncs/toolchains/322ac893fe/opt/zephyr-sdk \
/opt/homebrew/bin/cmake -DWEST_PYTHON=/opt/nordic/ncs/toolchains/322ac893fe/opt/python@3.12/bin/python3.12 \
  -DZEPHYR_BASE=/opt/nordic/ncs/v3.2.1/zephyr \
  -B/Users/cwaite/Documents/nrf-TinyENV/build/xiao_ble_uf2_app -GNinja \
  -DBOARD=xiao_ble -S/Users/cwaite/Documents/nrf-TinyENV \
  -DCONF_FILE=prj.conf\;prj_uf2.conf

PATH=/opt/nordic/ncs/toolchains/322ac893fe/bin:$PATH \
/opt/homebrew/bin/cmake --build /Users/cwaite/Documents/nrf-TinyENV/build/xiao_ble_uf2_app
```

## TODO

- Research the Matter Low Power (0x0508) cluster before enabling it in ZAP.
- Consider adding atmospheric pressure sensing/cluster support.
