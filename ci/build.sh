#!/usr/bin/env bash
#
# Build all firmware in the repo, one project directory at a time.
#
# Usage:
#   build.sh
set -euo pipefail

# Root directory.
ROOT_DIR="$(dirname "${BASH_SOURCE[0]}")/.."

# Navigate to the root directory.
cd "$ROOT_DIR"

# Build each project directory containing a Makefile.
for dir in $(find . -mindepth 2 -name Makefile -exec dirname {} \; | sort); do
    echo "==> $dir"
    make -C "$dir"
done
