# Project Agent Notes

- Builds can take several minutes; use longer timeouts for build commands.
- `west` is available at `/opt/nordic/ncs/toolchains/322ac893fe/Cellar/python@3.12/3.12.4/Frameworks/Python.framework/Versions/3.12/bin/west`.
- I2C on XIAO nRF52840 is currently working via `i2c0` mapped to D4/D5 in `boards/xiao_ble.overlay`.
