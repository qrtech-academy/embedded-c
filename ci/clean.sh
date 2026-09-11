#!/usr/bin/env bash
#
# Remove all built firmware in the repo, one project directory at a time.
#
# Usage:
#   clean.sh
set -euo pipefail

# Root directory.
ROOT_DIR="$(dirname "${BASH_SOURCE[0]}")/.."

# Navigate to the root directory.
cd "$ROOT_DIR"

# Clean each project directory under lectures/ containing a Makefile. The book has a Makefile of
# its own, in book/, and needs LuaLaTeX rather than the AVR toolchain: see book/README.md.
for dir in $(find ./lectures -name Makefile -exec dirname {} \; | sort); do
    echo "==> $dir"
    make -C "$dir" clean
done
