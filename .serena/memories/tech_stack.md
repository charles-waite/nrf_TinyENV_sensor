# Tech Stack

- Language: C++17 application code with Zephyr/NCS C APIs; Serena language backend: C++ LSP.
- SDK/toolchain: nRF Connect SDK v3.2.1 under `/opt/nordic/ncs/v3.2.1`; bundled toolchain root `/opt/nordic/ncs/toolchains/322ac893fe`.
- Preferred `west`: `/opt/nordic/ncs/toolchains/322ac893fe/Cellar/python@3.12/3.12.4/Frameworks/Python.framework/Versions/3.12/bin/west`.
- Build generator/tooling: CMake + Ninja via `/opt/homebrew/bin/cmake`; scripts set `ZEPHYR_TOOLCHAIN_VARIANT=zephyr` and `ZEPHYR_SDK_INSTALL_DIR`.
- Matter stack: Nordic/NCS Matter integration using generated ZAP artifacts and `ncs_configure_data_model()` from NCS common Matter CMake.
- Sensors: Zephyr built-in Sensirion SHT4x driver (`CONFIG_SHT4X=y`), ADC/SAADC battery sensing, GPIO LEDs/buttons.
- Board invariants: XIAO nRF52840, `i2c0` on D4/D5 via `boards/xiao_ble.overlay`; VBAT sense P0.14 active-low enable + P0.31/AIN7; current firmware LED polarity is active-high.
- Config layout: primary app config is `config/app/prj.conf`; UF2/low-power/UART variants are `config/app/prj_*.conf`.