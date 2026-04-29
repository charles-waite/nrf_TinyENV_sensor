#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOLCHAIN_ROOT="/opt/nordic/ncs/toolchains/322ac893fe"
PYTHON_BIN="${TOOLCHAIN_ROOT}/opt/python@3.12/bin"
TOOLCHAIN_BIN="${TOOLCHAIN_ROOT}/bin"
ZEPHYR_SDK="${TOOLCHAIN_ROOT}/opt/zephyr-sdk"
BUILD_DIR="${ROOT_DIR}/build/xiao_ble_uf2_lowpower"
CACHE_DIR="${ROOT_DIR}/build/zephyr-cache-lowpower"

export PATH="${TOOLCHAIN_BIN}:${PYTHON_BIN}:${PATH}"
if [[ "${CCACHE_DISABLE:-}" == "1" ]]; then
  export CCACHE_DISABLE=1
else
  unset CCACHE_DISABLE
fi

if [[ "${CLEAN_BUILD:-}" == "1" ]]; then
  rm -rf "${BUILD_DIR}" "${CACHE_DIR}" || true
  if [[ -d "${BUILD_DIR}" || -d "${CACHE_DIR}" ]]; then
    sleep 1
    rm -rf "${BUILD_DIR}" "${CACHE_DIR}" || true
  fi
fi

/opt/homebrew/bin/cmake \
  -S "${ROOT_DIR}" \
  -B "${BUILD_DIR}" \
  -GNinja \
  -DBOARD=xiao_ble \
  -DCONF_FILE=config/app/prj.conf \
  -DEXTRA_CONF_FILE="config/app/prj_uf2.conf;config/app/prj_lowpower.conf" \
  -DPython3_EXECUTABLE="${PYTHON_BIN}/python3.12" \
  -DZEPHYR_TOOLCHAIN_VARIANT=zephyr \
  -DZEPHYR_SDK_INSTALL_DIR="${ZEPHYR_SDK}" \
  -DUSER_CACHE_DIR="${CACHE_DIR}" \
  -DUSE_CCACHE=1

/opt/homebrew/bin/cmake --build "${BUILD_DIR}"
