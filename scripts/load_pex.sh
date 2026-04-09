#!/usr/bin/env bash
set -euo pipefail

sudo insmod kernel/pex.ko
ls -l /dev/pex
cat /proc/pex_stats || true