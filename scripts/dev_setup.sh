#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
MODULE_PATH="${ROOT_DIR}/kernel/pex.ko"

if [[ "${EUID}" -ne 0 ]]; then
  echo "Run as root (sudo)." >&2
  exit 1
fi

if [[ ! -f "${MODULE_PATH}" ]]; then
  echo "kernel/pex.ko not found, building kernel module..."
  make -C "${ROOT_DIR}" kernel
fi

if [[ ! -f "${MODULE_PATH}" ]]; then
  echo "Could not find built module at ${MODULE_PATH}" >&2
  echo "Make sure kernel headers are installed and 'make kernel' succeeds." >&2
  exit 1
fi

modprobe -r pex 2>/dev/null || true
insmod "${MODULE_PATH}"

major="$(grep " pex$" /proc/devices | awk '{print $1}')"
if [[ -z "${major}" ]]; then
  echo "Could not find pex major number" >&2
  exit 1
fi

if [[ ! -e /dev/pex ]]; then
  mknod /dev/pex c "${major}" 0
fi
chmod 666 /dev/pex

echo "PEX device ready at /dev/pex (major=${major})."
