#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/xiao_ble_uf2_app"
ZEPHYR_DIR="${BUILD_DIR}/zephyr"
BOARD="xiao_ble"
APP_OFFSET_DEFAULT="0x27000"
DO_BUILD=1
VERSION=""

usage() {
  cat >&2 <<'USAGE'
Usage: scripts/package_release.sh [version] [--no-build]

Builds the production UF2 profile and packages release-ready artifacts:
  - UF2 for drag-and-drop flashing with the stock XIAO nRF52840 UF2 bootloader
  - HEX with embedded addresses for SWD/debug-probe flashing
  - raw application BIN, which must be written at the reported flash offset

Examples:
  scripts/package_release.sh Verified
  scripts/package_release.sh Verified --no-build
USAGE
}

for arg in "$@"; do
  case "${arg}" in
    --no-build)
      DO_BUILD=0
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --*)
      usage
      exit 2
      ;;
    *)
      if [[ -n "${VERSION}" ]]; then
        usage
        exit 2
      fi
      VERSION="${arg}"
      ;;
  esac
done

if [[ -z "${VERSION}" ]]; then
  VERSION="$(git -C "${ROOT_DIR}" describe --tags --exact-match 2>/dev/null || git -C "${ROOT_DIR}" rev-parse --short HEAD)"
fi

safe_version="$(printf '%s' "${VERSION}" | tr -c 'A-Za-z0-9._-' '_')"
OUT_DIR="${ROOT_DIR}/build/releases/${safe_version}"
BASENAME="tinyenv_nrf_xiao_ble_${safe_version}"

if [[ "${DO_BUILD}" == "1" ]]; then
  "${ROOT_DIR}/scripts/build_uf2.sh"
fi

for artifact in zephyr.uf2 zephyr.hex zephyr.bin; do
  if [[ ! -f "${ZEPHYR_DIR}/${artifact}" ]]; then
    echo "Missing ${ZEPHYR_DIR}/${artifact}; run scripts/build_uf2.sh first." >&2
    exit 1
  fi
done

APP_OFFSET="${APP_OFFSET_DEFAULT}"
if [[ -f "${BUILD_DIR}/zephyr/.config" ]]; then
  configured_offset="$(sed -n 's/^CONFIG_FLASH_LOAD_OFFSET=//p' "${BUILD_DIR}/zephyr/.config" | tail -n 1 | tr -d '"')"
  if [[ -n "${configured_offset}" ]]; then
    APP_OFFSET="${configured_offset}"
  fi
fi

COMMIT="$(git -C "${ROOT_DIR}" rev-parse HEAD)"
COMMIT_SHORT="$(git -C "${ROOT_DIR}" rev-parse --short HEAD)"
BUILD_UTC="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

UF2_OUT="${OUT_DIR}/${BASENAME}.uf2"
HEX_OUT="${OUT_DIR}/${BASENAME}.hex"
BIN_OUT="${OUT_DIR}/${BASENAME}_app_${APP_OFFSET}.bin"
README_OUT="${OUT_DIR}/README_${safe_version}.txt"
MANIFEST_OUT="${OUT_DIR}/manifest_${safe_version}.json"

cp "${ZEPHYR_DIR}/zephyr.uf2" "${UF2_OUT}"
cp "${ZEPHYR_DIR}/zephyr.hex" "${HEX_OUT}"
cp "${ZEPHYR_DIR}/zephyr.bin" "${BIN_OUT}"

cat > "${README_OUT}" <<EOF_README
TinyENV nRF Sensor Release: ${VERSION}

Build metadata
- Git commit: ${COMMIT} (${COMMIT_SHORT})
- Board: ${BOARD}
- Profile: production low-power UF2
- Built UTC: ${BUILD_UTC}
- Application flash offset for raw BIN: ${APP_OFFSET}

Artifacts
- $(basename "${UF2_OUT}"): drag-and-drop UF2 for the stock XIAO nRF52840 UF2 bootloader.
- $(basename "${HEX_OUT}"): addressed Intel HEX image for SWD/debug-probe flashing.
- $(basename "${BIN_OUT}"): raw application binary; write this at ${APP_OFFSET} only.

UF2 flashing
1. Put the XIAO nRF52840 into bootloader mode.
2. Copy $(basename "${UF2_OUT}") to the mounted UF2 volume.

SWD/debug-probe flashing
- Prefer the HEX artifact because it carries flash addresses:
  nrfjprog --family NRF52 --program $(basename "${HEX_OUT}") --sectorerase --verify --reset

Raw BIN flashing
- The BIN does not carry addresses. Configure your flashing tool to write it at ${APP_OFFSET}.
- If your tool cannot set an explicit load address, use the HEX artifact instead.
EOF_README

cat > "${MANIFEST_OUT}" <<EOF_MANIFEST
{
  "name": "TinyENV nRF Sensor",
  "version": "${VERSION}",
  "git_commit": "${COMMIT}",
  "board": "${BOARD}",
  "profile": "production-low-power-uf2",
  "built_utc": "${BUILD_UTC}",
  "app_flash_offset": "${APP_OFFSET}",
  "artifacts": {
    "uf2": "$(basename "${UF2_OUT}")",
    "hex": "$(basename "${HEX_OUT}")",
    "bin": "$(basename "${BIN_OUT}")"
  }
}
EOF_MANIFEST

(
  cd "${OUT_DIR}"
  shasum -a 256 "$(basename "${UF2_OUT}")" "$(basename "${HEX_OUT}")" "$(basename "${BIN_OUT}")" "$(basename "${README_OUT}")" "$(basename "${MANIFEST_OUT}")" > SHA256SUMS.txt
)

cat <<EOF_DONE
Release package created: ${OUT_DIR}
UF2: ${UF2_OUT}
HEX: ${HEX_OUT}
BIN: ${BIN_OUT}
README: ${README_OUT}
SHA256: ${OUT_DIR}/SHA256SUMS.txt
EOF_DONE
