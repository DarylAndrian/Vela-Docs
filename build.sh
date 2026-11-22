#!/usr/bin/env bash
# ════════════════════════════════════════════════════════════════════════════
# Vela Docs — POSIX build helper (for reference / CI)
# ════════════════════════════════════════════════════════════════════════════
# Mostly for CI runners; the primary target is Windows.
# ════════════════════════════════════════════════════════════════════════════

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
BUILD_TYPE="${BUILD_TYPE:-Release}"
QT_PATH="${VELA_QT_PATH:-/opt/Qt/6.11.1/gcc_64}"

ACTION="${1:-build}"
case "$ACTION" in
    debug)
        BUILD_TYPE=Debug
        ACTION=build
        ;;
    clean)
        rm -rf "$BUILD_DIR"
        echo "Cleaned."
        exit 0
        ;;
esac

if [ ! -f "$QT_PATH/lib/cmake/Qt6/Qt6Config.cmake" ]; then
    echo "ERROR: Qt 6 not found at $QT_PATH"
    echo "Set VELA_QT_PATH environment variable."
    exit 1
fi

echo "Configuring ($BUILD_TYPE)..."
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DCMAKE_PREFIX_PATH="$QT_PATH"

echo "Building..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel

echo "✓ Build succeeded: $BUILD_DIR/bin/$BUILD_TYPE/vela-docs"
