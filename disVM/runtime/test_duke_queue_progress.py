# Copyright (c) 2026 bhgv
# [GC:] EN: Real large Duke graphs without exclusion or a fixed snapshot ceiling.
# RU: Настоящие большие графы Duke без эксклюзива и фиксированного потолка снимка.
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from resident_evidence import evidence


# [GC:] EN: Borrow runtime/compiler/include paths; own private binaries and real
# bytecode through exit. Refuse actual initial/growth allocations until a smaller
# graph frees. Both graphs must free beside admitted guests. Negative controls
# restore the old byte ceiling or permit invalid extents; require exact diagnostics,
# never compilation failure, signal, timeout or sanitizer output.
# RU: Заимствовать runtime/compiler/include; владеть личными бинарниками и
# байткодом до выхода. Отклонять начальное выделение/рост до free меньшего графа.
# Оба графа обязаны освободиться при активных гостях. Контроли возвращают старый
# лимит либо допускают неверные размеры; требовать точную диагностику, а не
# ошибку сборки, сигнал, таймаут или сообщение санитайзера.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (Path(v).resolve() for v in sys.argv[1:] if v != "--positive-only")
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="concurrent large Duke graphs ") as folder:
        work = Path(folder)
        image = work / "peers.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-o",
                str(image),
                str(root / "fixtures/resident-managed-retirement/main.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        for control in (0, 1, 2) if positive_only else (0, 1, 2, 100, 101):
            growth = storage = None
            if control == 100:
                source = (root / "gc/duke/snapshot_growth.c").read_text()
                anchor = "size_t maximum = dvm_rc_record_group_limit();"
                if source.count(anchor) != 1:
                    raise RuntimeError("Duke native limit anchor changed")
                source = source.replace(
                    anchor,
                    "size_t maximum = (4UL * 1024UL * 1024UL - per_node) / (per_node + per_field);",
                )
                growth = work / "snapshot_growth.c"
                growth.write_text(
                    source.replace('"../common/graph_snapshot.h"', '"gc/common/graph_snapshot.h"')
                )
            if control == 101:
                source = (root / "gc/common/graph_storage.c").read_text()
                anchor = "group->edges_used > group->field_limit ||"
                if source.count(anchor) != 1:
                    raise RuntimeError("snapshot extent guard anchor changed")
                storage = work / "graph_storage.c"
                storage.write_text(
                    source.replace(anchor, "0 ||").replace(
                        '"graph_snapshot.h"', '"gc/common/graph_snapshot.h"'
                    )
                )
            binary = relink(
                runtime,
                work,
                root / "test_duke_queue_progress.c",
                control,
                [
                    "dvm_runtime_guest_sleep",
                    "Sys_werrstr",
                    "poolfree",
                    "dvm_gc_duke_run",
                    "dvm_rc_grow_record_group",
                    "calloc",
                ],
                snapshot_growth_source=growth,
                graph_storage_source=storage,
            )
            profiles = (
                [("--no-jit", False)]
                if control >= 100
                else [(mode, locked) for mode in ("--no-jit", "--jit") for locked in (False, True)]
            )
            for mode, locked in profiles:
                result = subprocess.run(
                    [
                        str(binary),
                        "--native-spawn",
                        "--stats",
                        mode,
                        *(["--gc-locked-poll"] if locked else []),
                        str(image),
                    ],
                    cwd=work,
                    capture_output=True,
                    text=True,
                    timeout=25,
                )
                output = result.stdout + result.stderr
                if "Sanitizer" in output or "runtime error:" in output:
                    raise RuntimeError("large Duke sanitizer failure\n" + output[-12000:])
                if control >= 100:
                    status, marker = (
                        (70, "reason=refused-cycle-graph")
                        if control == 100
                        else (12, "invalid snapshot extents must not bypass bounds")
                    )
                    if result.returncode != status or marker not in output:
                        raise RuntimeError(
                            f"large Duke control missed {control}\n{output[-12000:]}"
                        )
                    if (
                        control == 100
                        and "DUKE_QUEUE_SHARED head_owned=1 follower_freed=1" not in output
                    ):
                        raise RuntimeError(
                            "old ceiling control lost independent progress\n" + output[-12000:]
                        )
                else:
                    if (
                        result.returncode
                        or "PASS Duke queue progress large=70000 independent=2300" not in output
                    ):
                        raise RuntimeError(
                            f"{control} {mode} locked={locked} exit={result.returncode}\n{output[-12000:]}"
                        )
                    evidence(output, children=2)
    print(
        "PASS concurrent Duke: 70000/2300 fields, initial/growth allocation retries, 70000 actual alias edges, native extent bounds, C/JIT and both polls"
        + ("; positive only" if positive_only else "; two semantic controls")
    )


if __name__ == "__main__":
    main()
