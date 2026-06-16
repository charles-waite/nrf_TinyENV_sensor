#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NCS_ROOT="${NCS_ROOT:-/opt/nordic/ncs/v3.2.1}"
TOOLCHAIN_ROOT="/opt/nordic/ncs/toolchains/322ac893fe"
PYTHON_BIN="${TOOLCHAIN_ROOT}/opt/python@3.12/bin"
TOOLCHAIN_BIN="${TOOLCHAIN_ROOT}/bin"
ZEPHYR_SDK="${TOOLCHAIN_ROOT}/opt/zephyr-sdk"
ZEPHYR_BASE="${NCS_ROOT}/zephyr"
BUILD_DIR="${ROOT_DIR}/build/xiao_ble_uf2_wipe_settings"
CACHE_DIR="${ROOT_DIR}/build/zephyr-cache-wipe-settings"

EXTRA_CONF_FILE="config/app/prj_uf2.conf;config/app/prj_wipe_settings.conf"

clean_path() {
  local path="$1"
  [[ -e "${path}" ]] || return 0

  for _ in 1 2 3; do
    rm -rf "${path}"
    [[ ! -e "${path}" ]] && return 0
    sleep 1
  done

  echo "Failed to remove ${path}; stop before reusing a stale build directory." >&2
  exit 1
}

export PATH="${TOOLCHAIN_BIN}:${PYTHON_BIN}:${PATH}"
export ZEPHYR_BASE
if [[ "${CCACHE_DISABLE:-}" == "1" ]]; then
  export CCACHE_DISABLE=1
else
  unset CCACHE_DISABLE
  export CCACHE_DIR="${CCACHE_DIR:-${ROOT_DIR}/build/ccache}"
  mkdir -p "${CCACHE_DIR}"
fi

if [[ "${CLEAN_BUILD:-}" == "1" ]]; then
  clean_path "${BUILD_DIR}"
  clean_path "${CACHE_DIR}"
elif [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  if ! grep -qx "NRF_DIR:PATH=${NCS_ROOT}/nrf" "${BUILD_DIR}/CMakeCache.txt"; then
    echo "Detected SDK change for ${BUILD_DIR}; cleaning stale CMake state." >&2
    clean_path "${BUILD_DIR}"
    clean_path "${CACHE_DIR}"
  fi
fi

mkdir -p "${CACHE_DIR}"
COMPAT_CONF="${CACHE_DIR}/ncs_compat.conf"
: > "${COMPAT_CONF}"
if grep -q "config CHIP_FACTORY_DATA_NONE" "${NCS_ROOT}/modules/lib/matter/config/nrfconnect/chip-module/Kconfig"; then
  cat > "${COMPAT_CONF}" <<'EOC'
CONFIG_CHIP_FACTORY_DATA_NONE=y
CONFIG_CHIP_FACTORY_DATA_NRFCONNECT_BACKEND=n
CONFIG_CHIP_FACTORY_DATA_BUILD=n
EOC
  EXTRA_CONF_FILE="${EXTRA_CONF_FILE};${COMPAT_CONF}"
fi

/opt/homebrew/bin/cmake \
  -S "${ROOT_DIR}" \
  -B "${BUILD_DIR}" \
  -GNinja \
  -DBOARD=xiao_ble \
  -DCONF_FILE=config/app/prj.conf \
  -DEXTRA_CONF_FILE="${EXTRA_CONF_FILE}" \
  -DDTC_OVERLAY_FILE="boards/xiao_ble.overlay;boards/xiao_ble_uart_console.overlay" \
  -DPython3_EXECUTABLE="${PYTHON_BIN}/python3.12" \
  -DZEPHYR_BASE="${ZEPHYR_BASE}" \
  -DZEPHYR_TOOLCHAIN_VARIANT=zephyr \
  -DZEPHYR_SDK_INSTALL_DIR="${ZEPHYR_SDK}" \
  -DUSER_CACHE_DIR="${CACHE_DIR}" \
  -DUSE_CCACHE=1

/opt/homebrew/bin/cmake --build "${BUILD_DIR}"
