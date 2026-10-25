#!/usr/bin/env bash
# Compatibility spelling: keep one implementation of the workspace builder.
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd) # Directory owning the canonical builder.
exec bash "$ROOT/build-workspace.sh" "$@"
