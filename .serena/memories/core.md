# Core

- NCS/Zephyr Matter firmware for Seeed XIAO nRF52840, derived from Nordic Matter temperature sensor sample.
- Source roots: `src/` application code, `src/default_zap/` ZAP/Matter model, `boards/` Zephyr overlays, `config/app/` Kconfig fragments, `config/pm/` partition manager files, `config/sysbuild/` sysbuild fragments.
- Main app symbols live in `src/app_task.cpp`/`.h`; `src/main.cpp` only starts `AppTask`; `src/shell_commands.cpp` is compiled only when `CONFIG_SHELL` is enabled.
- ZAP generated artifacts live under `src/default_zap/zap-generated/`; do not hand-edit generated files. Update `temperature_sensor.zap`/`.matter` and regenerate when model changes.
- Project-local guidance is in `AGENTS.md`; read it before edits. Related memories: `mem:tech_stack`, `mem:suggested_commands`, `mem:conventions`, `mem:task_completion`.