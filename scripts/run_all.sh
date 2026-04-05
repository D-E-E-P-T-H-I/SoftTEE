#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID}" -ne 0 ]]; then
  echo "Run this script as root: sudo ./scripts/run_all.sh" >&2
  exit 1
fi

sleep_s=1
if [[ "${1:-}" == "--sleep" && "${2:-}" != "" ]]; then
  sleep_s="${2}"
fi

echo "[1/5] Building userspace components..."
make lib examples tests

echo "[2/5] Building kernel module..."
make kernel

echo "[3/5] Loading module and preparing /dev/pex..."
./scripts/dev_setup.sh

echo "[4/5] Running full demo sequence..."
echo "----- [demo] protected_workload -----"
./examples/protected_workload || true

echo "----- [demo] showcase_blocking (record mode) -----"
./examples/showcase_blocking --sleep "${sleep_s}" || true

echo "----- [demo] printing /proc/pex_stats snapshot -----"
echo "----- /proc/pex_stats -----"
cat /proc/pex_stats || true

echo "----- [demo] recent kernel log (faults) -----"
dmesg | tail -n 80 || true

echo "[5/5] Running tests..."
./tests/test_multithread_violation
./tests/benchmark_entry_exit

echo "----- /proc/pex_stats -----"
cat /proc/pex_stats

echo "Run complete."
