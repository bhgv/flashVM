# Copyright (c) 2026 bhgv
# EN: Package the independent Dis64 inspector, codecs and semantic examples.
# RU: Упаковка отдельного инспектора Dis64, кодеков и семантических примеров.


# Install the shared structural codecs, actual inspector and explicitly non-executable demo.
# EN: base is a tested build directory, destination is the owned package directory.
# Copy products, transitive headers and fixtures; fail on any missing required file.
# Uses the parent installer copy/program helpers and ROOT; no target program is executed.
# RU: base — проверенная сборка, destination — каталог создаваемого пакета.
# Копирует продукты, все зависимые заголовки и примеры; отсутствие файла — ошибка.
# Использует copy/program и ROOT установщика; целевые программы не запускаются.
install_dis64_envelope() {
    local base=$1        # Borrowed tested build directory.
    local destination=$2 # Owned package output directory.
    local file           # Current source file or relative header path.
    install_program "$base" dis64inspect "$destination"
    install_program "$base" dis64_envelope_example "$destination"
    install_program "$base" dis64_pilot_example "$destination"
    install_program "$base" dis64_interfaces_example "$destination"
    copy_tree "$ROOT/disVM/envelope/examples" "$destination/examples/dis64-envelope"
    if [[ -f "$base/envelope/sample.dis64" ]]; then
        copy_file "$base/envelope/sample.dis64" "$destination/examples/dis64-envelope/sample.dis64"
    else
        # Cross builds package verified bytes without executing target binaries.
        "${PYTHON:-python3}" - "$ROOT/disVM/envelope/examples/sample.dis64.hex" "$destination/examples/dis64-envelope/sample.dis64" <<'PY'
from pathlib import Path
import sys
Path(sys.argv[2]).write_bytes(bytes.fromhex(Path(sys.argv[1]).read_text()))
PY
    fi
    # The real-payload fixture also has a host-only hexadecimal fallback.
    if [[ -f "$base/envelope/pilot42.dis64" ]]; then
        copy_file "$base/envelope/pilot42.dis64" "$destination/examples/dis64-envelope/pilot42.dis64"
    else
        "${PYTHON:-python3}" - "$ROOT/disVM/upstream/libinterp/tests/dis64/pilot42.hex" "$destination/examples/dis64-envelope/pilot42.dis64" <<'PYCODE'
from pathlib import Path
import sys
Path(sys.argv[2]).write_bytes(bytes.fromhex(Path(sys.argv[1]).read_text()))
PYCODE
    fi
    copy_file "$ROOT/disVM/upstream/libinterp/tests/dis64/pilot42.hex" "$destination/examples/dis64-envelope/pilot42.hex"
    if [[ -f "$base/envelope/pilot-init42.dis64" ]]; then
        copy_file "$base/envelope/pilot-init42.dis64" "$destination/examples/dis64-envelope/pilot-init42.dis64"
    else
        "${PYTHON:-python3}" - "$ROOT/disVM/upstream/libinterp/tests/dis64/pilot-init42.hex" "$destination/examples/dis64-envelope/pilot-init42.dis64" <<'PYCODE'
from pathlib import Path
import sys
Path(sys.argv[2]).write_bytes(bytes.fromhex(Path(sys.argv[1]).read_text()))
PYCODE
    fi
    copy_file "$ROOT/disVM/upstream/libinterp/tests/dis64/pilot-init42.hex" "$destination/examples/dis64-envelope/pilot-init42.hex"
    copy_file "$ROOT/disVM/upstream/utils/dis64-locations.py" "$destination/tools/dis64-locations.py"
    copy_file "$ROOT/disVM/upstream/utils/dis64inspect/dis64inspect.c" "$destination/source/dis64inspect.c"
    for file in isa.h dis64.h dis-rune.h dis-string-pool.h \
        dis-operand.h dis-layout.h dis64-module.h dis64-types.h \
        dis64-code.h dis64-pilot.h dis64-scalar.h dis64-data.h \
        dis64-signature.h dis64-bindings.h dis64-storage-image.h; do
        copy_file "$ROOT/disVM/upstream/include/$file" "$destination/include/dis64/$file"
    done
    # Keep relative includes valid for consumers outside the source checkout.
    # Сохранять относительные include при сборке вне исходного репозитория.
    copy_tree "$ROOT/disVM/upstream/include/dis64/pilot" "$destination/include/dis64/dis64/pilot"
    for file in "$ROOT/disVM/upstream/doc"/dis64-*.md "$ROOT/disVM/upstream/doc"/dis64-*.json; do
        copy_file "$file" "$destination/docs/dis64/$(basename -- "$file")"
    done
    # EN/RU: This retained native-series note is linked by the Dis64 guides.
    # Связанная заметка native-series также доступна без сети в SDK.
    copy_file "$ROOT/disVM/upstream/doc/native64-series0075.md" "$destination/docs/dis64/native64-series0075.md"
    copy_tree "$ROOT/examples/firststeps/tools/limbo-scalar" "$destination/examples/dis64-scalar"
    copy_file "$ROOT/tools/limbo-bootstrap/upstream/limbo/tests/README.dis64-scalar.md" "$destination/examples/dis64-scalar/UPSTREAM-SCALAR.md"
    copy_file "$ROOT/disVM/envelope/README.md" "$destination/DIS64-ENVELOPE.md"
    copy_file "$ROOT/disVM/envelope/README.ru.md" "$destination/DIS64-ENVELOPE.ru.md"
    "${PYTHON:-python3}" "$ROOT/tools/link-installed-fir-docs.py" "$ROOT" "$destination" --envelope
    cat >"$destination/run-dis64-envelope.sh" <<'EOF'
#!/bin/sh
# Inspect the packaged structural fixture; this launcher never executes Dis64 code.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$here/dis64inspect" "$here/examples/dis64-envelope/sample.dis64" "$@"
EOF
    chmod 755 "$destination/run-dis64-envelope.sh"
    cat >"$destination/run-dis64-pilot.sh" <<'EOF'
#!/bin/sh
# Check real scalar metadata/types/code; semantic validation never starts a VM.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$here/dis64inspect" "$here/examples/dis64-envelope/pilot42.dis64" --pilot "$@"
EOF
    chmod 755 "$destination/run-dis64-pilot.sh"
    cat >"$destination/run-dis64-storage.sh" <<'EOF'
#!/bin/sh
# Validate initialized scalar data and code without applying them to a VM heap.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$here/dis64inspect" "$here/examples/dis64-envelope/pilot-init42.dis64" --pilot "$@"
EOF
    chmod 755 "$destination/run-dis64-storage.sh"
    for file in peer_call_semantics.py peer_call_roles.py peer_call_flow.py peer_call_binding.py peer_call_schema.py peer_call_goldens.py test_peer_call_semantics.py; do
        copy_file "$ROOT/disVM/envelope/tests/$file" "$destination/source/dis64-checkpoint/$file"
    done
    copy_file "$ROOT/disVM/envelope/TYPED-CHECKPOINT.md" "$destination/docs/dis64/TYPED-CHECKPOINT.md"
    copy_file "$ROOT/disVM/envelope/reports/inferno0120-semantics.json" "$destination/docs/dis64/typed-checkpoint.json"


}
