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

CC=""
AR=""

if command -v clang &> /dev/null; then
    CC="clang"
    AR="llvm-ar"
elif command -v gcc &> /dev/null; then
    CC="gcc"
    AR="gcc-ar"
else
    echo "[RCC] ERROR: No C compiler found. Install clang or gcc."
    exit 1
fi

echo "[RCC] Using $CC..."
$CC -c -O2 rcc_runtime.c -o rcc_runtime.o

# Create static archive (.a) for lli -extra-archive
if command -v "$AR" &> /dev/null; then
    echo "[RCC] Creating archive with $AR..."
    $AR rcs rcc_runtime.a rcc_runtime.o
elif command -v ar &> /dev/null; then
    echo "[RCC] Creating archive with ar..."
    ar rcs rcc_runtime.a rcc_runtime.o
else
    echo "[RCC] WARNING: ar not found, .o may not work with lli -extra-archive"
fi

echo "[RCC] Runtime library built."
echo "[RCC] Usage: lli -extra-archive=rcc_runtime.a ../tests/irs/test.ll"
