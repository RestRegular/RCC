#!/bin/bash
# ============================================================================
# RCC Runtime Library - Build Script (Linux/macOS)
# ============================================================================
# Usage: ./build_runtime.sh
#
# Prerequisites:
#   - clang or gcc installed
# ============================================================================

set -e

echo "[RCC] Building runtime library..."

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

if command -v clang &> /dev/null; then
    echo "[RCC] Using clang..."
    clang -c -O2 rcc_runtime.c -o rcc_runtime.o
elif command -v gcc &> /dev/null; then
    echo "[RCC] Using gcc..."
    gcc -c -O2 rcc_runtime.c -o rcc_runtime.o
else
    echo "[RCC] ERROR: No C compiler found. Install clang or gcc."
    exit 1
fi

echo "[RCC] Runtime library built: rcc_runtime.o"
echo "[RCC] Usage: lli -extra-archive=rcc_runtime.o tests/irs/test.ll"
