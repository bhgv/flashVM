#!/usr/bin/env bash
# Verify actual native products without invoking an emulator or CMake/CTest.
set -euo pipefail
root=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
output=${BUILD_ROOT:-"$root/build/native"}
python=${PYTHON:-python3}
for component in "$@"; do
    case "$component" in
        mk)
            "$python" "$root/tools/mk/tests/smoke.py" "$output/mk/mk" ;;
        iyacc)
            "$python" "$root/tools/iyacc/tests/smoke.py" "$output/iyacc/iyacc" \
                "$output/iyacc" "$root/tools/limbo-bootstrap/upstream/limbo/limbo.y" \
                "$root/tools/limbo-bootstrap/upstream/limbo/y.tab.h" ;;
        limbo-bootstrap)
            "$python" "$root/tools/limbo-bootstrap/tests/smoke.py" \
                "$output/limbo-bootstrap/limbo-bootstrap" ;;
        *) printf 'No native test adapter for %s\n' "$component" >&2; exit 2 ;;
    esac
done
