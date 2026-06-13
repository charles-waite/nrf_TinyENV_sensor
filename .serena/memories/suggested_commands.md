# Suggested Commands

- Default UF2 debug build: `scripts/build_uf2.sh`; output `build/xiao_ble_uf2_app/zephyr/zephyr.uf2`.
- Diagnostic UF2 build: `scripts/build_uf2.sh --diag`; output directory `build/xiao_ble_uf2_diag/`.
- UART-only UF2 build: `scripts/build_uf2_uart.sh`; output `build/xiao_ble_uf2_uart/zephyr/zephyr.uf2`.
- Use `CLEAN_BUILD=1 <script>` when config/toolchain/board settings changed or cache is suspected stale.
- Use `CCACHE_DISABLE=1 <script>` when diagnosing build inconsistencies.
- Sysbuild/MCUboot path uses `west build` and `sysbuild/`; do not use it for UF2-only tasks unless requested.
- Check repo state before commits: `git status --short`.
- Serena setup sanity: `serena memories check` from project root.