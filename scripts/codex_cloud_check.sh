#!/usr/bin/env bash

set -euo pipefail

cmake -S . -B build/codex-cloud -G Ninja
cmake --build build/codex-cloud

app="./build/codex-cloud/diffyinajiffy"

if [ ! -x "$app" ]; then
    echo "Expected executable was not built: $app"
    exit 1
fi

set +e
xvfb-run -a --server-args="-screen 0 1280x900x24" timeout 5s "$app"
status=$?
set -e

if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
    echo "Headless GUI smoke launch failed with exit code $status"
    exit "$status"
fi

echo "Codex Cloud build and headless GUI smoke check passed."
