# Repository Guidelines

## Project Structure & Module Organization
This repo is the new home for a TinyENV Sensor port targeting **Seeed XIAO nRF52840**. The existing `ESP32 source/` directory is a reference implementation for the ESP32‑C6; use it for guidance, not as a direct code drop.
- `ESP32 source/` holds the prior ESP‑IDF project, drivers, and Matter endpoint examples.
- New nRF52840 code should live in a dedicated top‑level folder (create as needed) and follow Nordic‑native layouts and tooling.
- Keep hardware‑specific assets or notes alongside the platform folder that uses them.

## Build, Test, and Development Commands
Current build commands are only defined for the ESP32 reference project:
- `idf.py set-target esp32c6` — set target MCU.
- `idf.py build` — build the ESP32 reference with `sdkconfig.defaults`.
- `idf.py -p /dev/cu.usbmodemXXXX erase-flash flash monitor` — clean flash + monitor.

For the nRF52840 target, prefer Nordic tooling and update this section once a build system is established.

## Coding Style & Naming Conventions
- Keep code lean and power‑focused; low‑power behavior is the primary design requirement.
- Use existing C/C++ style in the repo as a reference: 2‑space indentation, `snake_case` functions, `UPPER_SNAKE_CASE` constants.
- Favor small, testable modules over large monolithic files.
- Avoid deprecated APIs; prefer current Nordic SDK/Zephyr equivalents.

## Testing Guidelines
- No automated tests are currently defined.
- Validate by flashing to hardware and confirming:
  - Matter over Thread commissioning.
  - ~2‑minute sensor update cadence.
  - Sleepy End Device behavior and power targets (<1 mA average).
- Record power measurements in PR notes when they change.

## Commit & Pull Request Guidelines
- Git history is not available in this snapshot, so conventions can’t be inferred.
- Use short, imperative commit messages (e.g., “Add SHT41 minimal driver”).
- PRs should include: purpose, hardware tested, commissioning results, and power measurements.

## Configuration & Hardware Notes
- Target platform: Seeed XIAO nRF52840 with SHT41 sensor and 18650 battery.
- OTA is not required; LED status codes and a custom GPIO button are allowed.
- The user is CLI‑comfortable but benefits from explicit steps for new commands.
