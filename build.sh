#!/usr/bin/env bash
# CMake-free build script for the Enigma test suite.
# Usage: ./build.sh
set -euo pipefail
cd "$(dirname "$0")"

CXX="${CXX:-g++}"
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

# core is pure C++20: no OS or OpenGL headers are allowed there.
"$CXX" -std=c++20 -O2 -Wall -Wextra -Wpedantic \
    -Icore -Itests \
    core/enigma.cpp tests/test_core.cpp \
    -o "$BUILD_DIR/enigma_tests"

echo "Build OK: $BUILD_DIR/enigma_tests"
"$BUILD_DIR/enigma_tests"

# math is pure C++20 as well (no OS or OpenGL headers).
"$CXX" -std=c++20 -O2 -Wall -Wextra -Wpedantic \
    -Imath -Itests \
    tests/test_math.cpp \
    -o "$BUILD_DIR/enigma_math_tests"

echo "Build OK: $BUILD_DIR/enigma_math_tests"
"$BUILD_DIR/enigma_math_tests"
