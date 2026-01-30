# Handoff Notes — nrf-TinyENV

## Current state
- Repo location: `/Users/cwaite/Documents/nrf-TinyENV`
- Target board: **Seeed XIAO nRF52840 (non Plus/Sense)**
- SDK: **nRF Connect SDK v3.2.1** installed at `/opt/nordic/ncs/v3.2.1`
- Toolchain: `/opt/nordic/ncs/toolchains/322ac893fe`
- Build command that works (run from nRF Connect terminal):
  ```bash
  cd /opt/nordic/ncs/v3.2.1
  west build -b xiao_ble -s "/Users/cwaite/Documents/nrf-TinyENV" -d "/Users/cwaite/Documents/nrf-TinyENV/build/xiao_ble"
  ```
- App scaffold was replaced with NCS **Matter temperature_sensor** sample (copied from `/opt/nordic/ncs/v3.2.1/nrf/samples/matter/temperature_sensor`).
- ESP32 reference project remains under `ESP32 source/` for feature mapping.

## ZAP / Matter Cluster Editor notes
- Matter Cluster Editor (MCE) expects a `.matter` file, not `.zap`.
- ZAP file path in repo: `src/default_zap/temperature_sensor.zap`
- Generated Matter IDL: `src/default_zap/temperature_sensor.matter`
- ZAP tool failed to open the file via GUI due to missing template paths.
- Use the NCS wrapper to launch ZAP with correct templates:
  ```bash
  /opt/nordic/ncs/v3.2.1/modules/lib/matter/scripts/tools/zap/run_zaptool.sh \
    "/Users/cwaite/Documents/nrf-TinyENV/src/default_zap/temperature_sensor.zap"
  ```
- If manual template selection is needed:
  - `/opt/nordic/ncs/v3.2.1/modules/lib/matter/src/app/zap-templates/zcl/zcl.json`
  - `/opt/nordic/ncs/v3.2.1/modules/lib/matter/src/app/zap-templates/app-templates.json`

## Desired endpoint changes (via ZAP)
- Add **Relative Humidity Measurement** cluster
- Add **Power Source** cluster for battery voltage + battery % remaining
- Charging status can be set to a safe static value (no warnings required)

## Key ESP32 reference features to port
- SHT41 over I2C (SDA/SCL pins need mapping for XIAO nRF52840)
- 2:1 voltage divider for battery measurement
- Matter clusters: Temperature + Humidity + Battery
- ICD/LIT sleepy device behavior
- Button for decommission / verbose toggle (XIAO nRF52840 has no boot button)

## Cleanup notes
- `.DS_Store` ignored via `.gitignore` and removed from index.
- If running ZAP from this CLI, needs escalated permission to open GUI app.

