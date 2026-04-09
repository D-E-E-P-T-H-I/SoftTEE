#!/usr/bin/env bash
set -euo pipefail

make all
./examples/demo || true
./tests/test_faults
cat /proc/pex_stats || true