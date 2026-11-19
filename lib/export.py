# Copyright (c) 2026 bhgv
# EN: Export pinned standalone source snapshots; check drift without changing canonical engine sources.
# RU: Экспортировать самостоятельные снимки исходников; проверять дрейф, не меняя исходники движков.
from pathlib import Path
import argparse
import hashlib
import json
import importlib.util


# EN: Yield source/destination pairs for independent packages; repository paths are exporter inputs, never build inputs.
# RU: Выдать пары исходник/назначение пакетов; пути репозитория нужны экспортёру, но не сборке библиотек.
def files(root):
    mappings = [
        ("disVM/native/src", "styx/vendor/inferno", "**/*"),
        ("disVM/native/client", "styx/client", "*.[ch]"),
        ("disVM/native/server/phy", "styx/server/phy", "*.[ch]"),
        ("disVM/native/ports/posix", "styx/ports/posix", "*.[ch]"),
        ("disVM/host/traversal", "styx/walk", "*.[ch]"),
        ("remote_phy", "styx/remote_phy", "**/*.[ch]"),
        ("disVM/native/terminal/order", "libunibus/order", "*.[ch]"),
    ]
    for source, destination, pattern in mappings:
        for path in sorted((root / source).glob(pattern)):
            if path.is_file():
                parts = path.relative_to(root / source).parts
                if source == "disVM/native/src" and ("tests" in parts or path.name in ("Nt.c", "Plan9.c") or parts[:2] == ("Linux", "arm")):
                    continue
                if source == "remote_phy" and ("tests" in parts or "mux" in parts):
                    continue
                yield path, root / "lib" / destination / path.relative_to(root / source)
    for name in ("prepare_fmt.py", "prepare_errstr.py", "prepare_dtoa.py", "host/decimal_sync.c"):
        yield root / "tools/limbo-bootstrap" / name, root / "lib/styx/support" / name
    for name in ("codec.c", "queues.c", "rollover.c", "fault.c", "check.h", "oracle.c", "oracle.py"):
        yield root / "disVM/native/terminal/order/tests" / name, root / "lib/libunibus/order/tests" / name
    for name in ("native64.c", "stream.c", "session.c", "directories.c", "lifecycle.c", "fid-context.c"):
        yield root / "disVM/native/src/tools/libstyx/tests" / name, root / "lib/styx/vendor/inferno/tools/libstyx/tests" / name
    yield root / "disVM/native/lib9/sources.py", root / "lib/styx/support/sources.py"
    yield root / "disVM/native/vfs/file.h", root / "lib/styx/client/file.h"
    yield root / "disVM/native/src/include/fcall.h", root / "lib/styx/include/styx/fcall.h"
    yield root / "disVM/native/UPSTREAM.json", root / "lib/styx/vendor/UPSTREAM.json"
    for package in ("styx", "libunibus"):
        yield root / "LICENSE", root / "lib" / package / "LICENSE.bhgv"
        for name in ("srecord", "sstream"):
            yield root / f"flashVM/src/protocol/{name}.c", root / f"lib/{package}/protocol/{name}.c"
            yield root / f"flashVM/include/flashvm/{name}.h", root / f"lib/{package}/include/flashvm/{name}.h"
    for name in ("sbindings", "scontrol", "sslot", "sadapt", "smeasure", "smedia"):
        yield root / f"flashVM/src/protocol/{name}.c", root / f"lib/libunibus/protocol/{name}.c"
        yield root / f"flashVM/include/flashvm/{name}.h", root / f"lib/libunibus/include/flashvm/{name}.h"
    for path in sorted((root / "flashVM/src/protocol/order").glob("*.c")):
        yield path, root / "lib/libunibus/protocol/order" / path.name
    for name in ("sorder", "sordctl"):
        yield root / f"flashVM/include/flashvm/{name}.h", root / f"lib/libunibus/include/flashvm/{name}.h"


# EN: Write or compare each declared snapshot and its deterministic provenance manifest; fail on any drift.
# RU: Записать или сравнить снимки и детерминированный manifest происхождения; при дрейфе завершиться ошибкой.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    manifests = {"styx": [], "libunibus": []}
    expected_paths = set()
    for source, target in files(root):
        data = source.read_bytes()
        transformation = "identity"
        native_preparation = {"fmt.c": "fmt", "errstr-posix.c": "errstr", "dtoa.c": "dtoa"}
        if source.parent in (root / "disVM/native/src/lib9", root / "disVM/native/src/libmath") and source.name in native_preparation:
            unit = native_preparation[source.name]
            spec = importlib.util.spec_from_file_location("prepare_" + unit, root / f"tools/limbo-bootstrap/prepare_{unit}.py")
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            data = module.prepare(data.decode()).encode()
            transformation = "existing-native-" + unit + "-preparation"
        if source.name == "srecord.h":
            if data.count(b'#include "flashvm.h"') != 1:
                raise RuntimeError("protocol export boundary drift")
            data = data.replace(b'#include "flashvm.h"', b'#include "wire_export.h"')
            transformation = "standalone-export-header"
        if source == root / "disVM/native/vfs/file.h":
            marker = b"/* EN/RU: Consume backing;"
            if data.count(marker) != 1:
                raise RuntimeError("client scalar boundary drift")
            data = data.split(marker)[0] + b"#endif\n"
            transformation = "client-scalar-types-only"
        if target == root / "lib/styx/include/styx/fcall.h":
            marker = b"int\tfcallfmt(Fmt*);"
            if data.count(marker) != 1:
                raise RuntimeError("portable codec declaration boundary drift")
            data = data.split(marker)[0]
            data = b"#ifndef STANDALONE_STYX_FCALL_H\n#define STANDALONE_STYX_FCALL_H\n" + b"\n".join(
                line for line in data.splitlines() if not line.startswith(b"#pragma")) + b"\n#endif\n"
            transformation = "portable-codec-declarations-no-formatter"
        if args.check:
            if not target.is_file():
                raise RuntimeError("snapshot drift: " + str(target))
            from tools.layout import same_tokens
            if target.read_bytes() != data and (target.suffix not in (".c", ".h", ".inc", ".py") or not same_tokens(data.decode(), target)):
                raise RuntimeError("snapshot algorithm drift: " + str(target))
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        expected_paths.add(target)
        package = target.relative_to(root / "lib").parts[0]
        manifests[package].append(dict(source=str(source.relative_to(root)), path=str(target.relative_to(root / "lib" / package)),
                                      sha256=hashlib.sha256(data).hexdigest(), transform=transformation))
    for package, entries in manifests.items():
        text = "\n".join(json.dumps(entry, ensure_ascii=False, sort_keys=True) for entry in entries) + "\n"
        manifest = root / "lib" / package / "SOURCES.jsonl"
        if args.check:
            if manifest.read_text() != text:
                raise RuntimeError("manifest drift: " + str(manifest))
        else:
            manifest.write_text(text)
        print(package, len(entries), "verified" if args.check else "exported")
    if not args.check:
        for package in ("styx", "libunibus"):
            inventory = root / "lib" / package / "EXPORTED-PATHS.txt"
            if inventory.exists():
                for name in inventory.read_text().splitlines():
                    stale = root / name
                    if stale not in expected_paths and stale.is_file():
                        stale.unlink()
            inventory.write_text("\n".join(str(path.relative_to(root)) for path in sorted(expected_paths)
                if path.is_relative_to(root / "lib" / package)) + "\n")


if __name__ == "__main__":
    main()
