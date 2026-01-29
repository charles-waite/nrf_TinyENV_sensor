# Repository Guidelines

## Project Structure & Module Organization
This repo hosts a TinyENV Sensor port targeting **Seeed XIAO nRF52840 (non Plus/Sense)**. The `ESP32 source/` folder is an ESP32‑C6 reference; use it for guidance, not direct reuse.
- `ESP32 source/` contains the prior ESP‑IDF project, drivers, and Matter endpoint examples.
- New nRF52840 code should live in a dedicated top‑level folder (create as needed) and follow Nordic/Zephyr layouts.

## Toolchain & Workspace Model
- Standardize on **nRF Connect SDK (NCS) 3.2.1** and `west`.
- VS Code “workspace” is just an editor concept; the **NCS workspace** is the folder containing `.west/`, `nrf/`, `zephyr/`, and `modules/`.
- Keep this repo inside the NCS workspace, e.g. `~/ncs/apps/tinyenv/`.

## Build, Flash, and Development Commands
Use `west` (Zephyr/NCS equivalent of `idf.py`):
- `west init -l <app-folder>` — initialize the NCS workspace around this repo.
- `west update` — fetch SDK dependencies.
- `west build -b xiao_ble <app-folder>` — build for XIAO nRF52840.
- `west flash` — flash (UF2 via USB‑C or J‑Link via SWD).

## Coding Style & Naming Conventions
- Power is the primary constraint: average current must be <200 µA; >1 mA is unacceptable.
- Follow existing style: 2‑space indentation, `snake_case` functions, `UPPER_SNAKE_CASE` constants.
- Favor small, testable modules over large monolithic files.
- Avoid deprecated APIs; prefer current Nordic/Zephyr equivalents.

## Testing Guidelines
- No automated tests are defined.
- Validate on hardware: Matter commissioning, ~2‑minute sensor cadence, Sleepy End Device behavior, and current draw.
- Record power measurements in PR notes when they change.

## Commit & Pull Request Guidelines
- Use short, imperative commit messages (e.g., “Add SHT41 minimal driver”).
- PRs should include: purpose, hardware tested, commissioning results, and power measurements.

## Configuration & Hardware Notes
- Target platform: XIAO nRF52840 + SHT41 + 18650 Li‑Ion (board BMS).
- Include Matter Battery Voltage and Battery Percentage Remaining clusters.
- A 2:1 voltage divider is required for battery sensing; document wiring differences between XIAO nRF52840 and XIAO ESP32‑C6 before implementation.
- OTA is not required; LED status codes and a custom GPIO button are allowed.

## Open Research Tasks
- Compare XIAO nRF52840 vs XIAO ESP32‑C6 battery‑sense wiring and ADC capabilities for a 2:1 divider.
- Identify the lowest‑power ADC or GPIO strategy that still enables battery voltage reporting.
