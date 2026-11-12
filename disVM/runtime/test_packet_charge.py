# Copyright (c) 2026 bhgv
# [GC:] EN: Actual mapped record/array alias-sort OOM must preserve shared retry.
# RU: Настоящий OOM сортировки алиасов записи/массива обязан сохранить shared retry.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from concurrent_owner_fixture import prepare_fixture
from rc_test_source import read_rc_source
from relink_private import relink
from resident_evidence import evidence


# [GC:] EN: Borrow CLI runtime/compiler/include roots; own private relinks and
# bytecode. Derive real case 6 with one child in 2300 mapped fields, as record
# and array. Clamp charge limit, never payload sizes/counts/roots. Sorting OOM
# must retry eight actual passes, then free exactly; lost-retry controls fail12.
# --positive-only excludes intentional violations for sanitizer runs.
# RU: Заимствовать CLI runtime/compiler/include; владеть личными сборками и
# байткодом. Настоящий case 6 содержит одного ребёнка в 2300 полях записи либо
# массива. Уменьшать лимит учёта, не размеры/ссылки/корни. Восемь OOM должны
# дать повтор, затем точное освобождение; потеря retry даёт код 12.
# --positive-only исключает намеренные нарушения для санитайзера.
def main():
    positive = "--positive-only" in sys.argv[1:]
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:] if x != "--positive-only")
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix="packet alias charge ") as folder:
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
        )
        fixture = prepare_fixture(root, work)
        source = fixture.read_text()
        source = (
            "enum { ChargeShape = CONTROL >= 4 ? CONTROL - 4 : CONTROL / 2, ChargeFail = CONTROL >= 4 || CONTROL % 2 };\n#undef CONTROL\n#define CONTROL 6\n"
            + source
        )
        anchor = "enum FaultKind"
        source = source.replace(
            anchor,
            "#undef IsArray\n#define IsArray ChargeShape\nstatic _Thread_local int charge_sorting; /* Actual packet-charge vector only. */\n\n"
            + anchor,
            1,
        )
        anchor = "void *__wrap_malloc(size_t bytes)\n{"
        if source.count(anchor) != 1:
            raise RuntimeError("alias fixture malloc boundary changed")
        source = source.replace(
            anchor,
            anchor
            + "\n    if (charge_sorting)\n    {\n        if (ChargeFail && refuse_selected(NoFault)) { return NULL; }\n        return __real_malloc(bytes);\n    }",
        )
        anchor = '    fprintf(stderr,\n            "PASS large owner release'
        if source.count(anchor) != 1:
            raise RuntimeError("alias fixture completion boundary changed")
        source = source.replace(
            anchor,
            '    require(refusals == (ChargeFail ? 8 : 0), "actual alias-sort OOM count");\n'
            + anchor,
        )
        source = source.replace(
            "(Fault ? refusals >= 8 && failure_passes >= 8 : !refusals)",
            "(ChargeFail ? refusals >= 8 && failure_passes >= 8 : !refusals)",
        )
        source += "\n" + (root / "test_packet_charge_probe.inc").read_text()
        fixture.write_text(source)
        symbols = sorted(set(re.findall(r"__real_([A-Za-z0-9_]+)", source)))
        rc = read_rc_source(root)
        declaration = (
            "/* [GC:] EN: Visit source under sole Fast shared admission with an independent"
        )
        anchor = "            retry_leaf |= dvm_refcount_load_u(&entry->heap->ref) > 1;"
        if rc.count(declaration) != 1 or rc.count(anchor) != 1:
            raise RuntimeError("leaf refusal observation boundary changed")
        rc = rc.replace(
            declaration, "extern void dvm_test_large_leaf_retry(Heap *, int);\n" + declaration
        )
        rc = rc.replace(
            anchor, anchor + "\n            dvm_test_large_leaf_retry(entry->heap, retry_leaf);"
        )
        runtime_source = work / "runtime_rc.c"
        runtime_source.write_text(rc)
        for control in range(4) if positive else range(6):
            budget_source = None
            if control >= 4:
                code = (root / "gc/fast/packet_budget.c").read_text()
                if code.count("            *retry = 1;") != 1:
                    raise RuntimeError("packet sorting retry boundary changed")
                budget_source = work / "packet_budget.c"
                budget_source.write_text(
                    code.replace('"packet_budget.h"', '"gc/fast/packet_budget.h"').replace(
                        "            *retry = 1;", "            (void)retry;"
                    )
                )
            binary = relink(
                runtime,
                work,
                fixture,
                control,
                symbols,
                runtime_rc_source=runtime_source,
                packet_budget_source=budget_source,
            )
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    result = subprocess.run(
                        [
                            str(binary),
                            "--native-spawn",
                            "--gc-idle-cycles",
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
                        raise RuntimeError(
                            "packet alias charge sanitizer failure\n" + output[-10000:]
                        )
                    valid = (
                        result.returncode == 12
                        and "temporary leaf failure must preserve shared retry" in output
                        if control >= 4
                        else result.returncode == 0 and "PASS large owner release case=6 " in output
                    )
                    if not valid:
                        raise RuntimeError(
                            f"alias charge control={control} {mode} locked={locked}\n{output[-10000:]}"
                        )
                    if control < 4:
                        evidence(output, children=2)
    print(
        "PASS packet alias charging: sixteen real record/array profiles"
        + ("" if positive else "; eight lost-retry controls")
    )


if __name__ == "__main__":
    main()
