# Copyright (c) 2026 bhgv
# EN: Real wide owner retirement, exact child multiplicity and transient retry.
# RU: Настоящее освобождение широких владельцев, кратность ссылок и повторы.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from rc_test_source import read_rc_source
from relink_private import relink
from resident_evidence import evidence
from test_duke_pin_retirement import private_includes


# EN: Own private real-runtime relinks using CLI runtime/compiler/include paths.
# Positive profiles cover ordinary records, list tail/data, arrays, nil and aliases,
# allocation/queue pressure and actual competing retains in C/JIT and both polls.
# Negative builds require exact semantic errors; compiler/sanitizer/process-timeout
# failures never count as controls. --positive-only skips intentional violations.
# RU: Владеть личными сборками настоящего runtime по CLI путям. Проверить записи,
# хвост/данные списка, массивы, nil, кратность ссылок, память/очереди и настоящие
# новые ссылки в C/JIT с обоими poll. Отрицательные сборки требуют конкретных
# семантических ошибок; ошибки сборки/санитайзера/таймауты процесса не принимаются.
def main():
    positive_only = "--positive-only" in sys.argv[1:]
    channel_only = "--channel-only" in sys.argv[1:]
    runtime, compiler, include = (
        Path(value).resolve()
        for value in sys.argv[1:]
        if value not in ("--positive-only", "--channel-only")
    )
    root = Path(__file__).parent
    source = read_rc_source(root)
    anchor = "            retry_leaf |= dvm_refcount_load_u(&entry->heap->ref) > 1;"
    declaration = "/* [GC:] EN: Visit source under sole Fast shared admission with an independent"
    if source.count(anchor) != 1 or source.count(declaration) != 1:
        raise RuntimeError("large owner diagnostic boundary drift")
    source = source.replace(
        declaration, "extern void dvm_test_large_leaf_retry(Heap *, int);\n" + declaration
    ).replace(anchor, anchor + "\n            dvm_test_large_leaf_retry(entry->heap, retry_leaf);")
    claim = "    int claimed = dvm_refcount_claim_last_u(&heap->ref);"
    claim_doc = "/* [GC:] EN: Claim heap's independently owned last reference without children."
    if source.count(claim) != 1 or source.count(claim_doc) != 1:
        raise RuntimeError("childless atomic claim boundary drift")
    source = source.replace(
        claim_doc, "extern int dvm_test_empty_leaf_claim(Heap *);\n\n" + claim_doc
    ).replace(claim, "    int claimed = dvm_test_empty_leaf_claim(heap);")
    sole = "    int sole = dvm_refcount_load_u(&heap->ref) == 1;"
    sole_doc = "/* [GC:] EN: Sample independently live heap before child transfer. Return one"
    if source.count(sole) != 1 or source.count(sole_doc) != 1:
        raise RuntimeError("preliminary leaf count boundary drift")
    source = source.replace(
        sole_doc, "extern int dvm_test_leaf_sole_owner(Heap *);\n\n" + sole_doc
    ).replace(sole, "    int sole = dvm_test_leaf_sole_owner(heap);")
    mutations = {
        100: (
            "    if (count > RECORD_EDGE_LIMIT)\n    {",
            "    if (count > RECORD_EDGE_LIMIT)\n    {\n        return 0;",
        ),
        101: (
            "        if (!keys)\n        {\n            *retry = 1;\n            return 0;",
            "        if (!keys)\n        {\n            return 0;",
        ),
        102: (
            "    if (!packet || !roots)\n    {\n        if (retry)\n        {\n            *retry = 1;",
            "    if (!packet || !roots)\n    {\n        if (retry)\n        {\n            *retry = 0;",
        ),
        103: (
            "    if (!packet || !roots)\n    {\n        if (retry)\n        {\n            *retry = 1;",
            "    if (!packet || !roots)\n    {\n        if (retry)\n        {\n            *retry = 0;",
        ),
        104: (
            "    FrameRetirement *packet = malloc(storage);\n    if (!packet)\n    {\n        *retry = 1;",
            "    FrameRetirement *packet = malloc(storage);\n    if (!packet)\n    {",
        ),
        105: ("        *retry = packet->roots == NULL;", ""),
        106: ("retry |= retry_leaf && dvm_world_has_active_guest(world);", "(void)retry_leaf;"),
        107: (
            "    *retry = 1;\n    if (!dvm_runtime_root_batch_clear(packet->roots) || !dvm_runtime_root_batch_destroy(&packet->roots) ||",
            "    if (!dvm_runtime_root_batch_clear(packet->roots) || !dvm_runtime_root_batch_destroy(&packet->roots) ||",
        ),
        108: ("    *retry |= rooted;", ""),
        109: (
            "    *retry = 1;\n    if (!dvm_runtime_root_batch_clear(packet->roots) || !dvm_runtime_root_batch_destroy(&packet->roots) ||",
            "    *retry = 1;\n    if (0 || !dvm_runtime_root_batch_destroy(&packet->roots) ||",
        ),
        110: ("retry |= retry_leaf && dvm_world_has_active_guest(world);", "retry |= retry_leaf;"),
        125: (
            "*retry |= dvm_runtime_channel_buffer_busy_snapshot(channel) ||",
            "*retry = dvm_runtime_channel_buffer_busy_snapshot(channel) ||",
        ),
        126: ("*retry |= !claimed &&", "*retry = !claimed &&"),
    }
    mutations.update(
        {control: ("    *retry |= !claimed;", "    (void)retry;") for control in range(111, 117)}
    )
    mutations.update(
        {control: ("    *retry |= !sole;", "    (void)retry;") for control in range(117, 121)}
    )
    with tempfile.TemporaryDirectory(prefix="large owner release ") as temporary:
        work = Path(temporary)
        image = work / "wide.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(image),
                str(root / "fixtures/resident-packet-cycle/main.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        channel_image = work / "channel.dis"
        subprocess.run(
            [
                str(compiler),
                "-I",
                str(include),
                "-I",
                str(root.parent / "module"),
                "-o",
                str(channel_image),
                str(root / "fixtures/resident-channel-claim/main.b"),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        positive = tuple(range(31, 39)) if channel_only else tuple(range(39))
        host_controls = (121, 122, 123, 124, 127, 128, 129, 130, 131)
        negatives = (125, 126, *host_controls) if channel_only else (*mutations, *host_controls)
        for control in positive if positive_only else (*positive, *negatives):
            text = source
            host = None
            channel_case = control in range(31, 39) or control >= 121
            if control in host_controls:
                path = root / "host_runtime/channel_retirement.inc"
                body = path.read_text()
                gate = "if (!claimed && retry && !channel->aux)"
                if body.count(gate) != 1:
                    raise RuntimeError("channel observed refusal anchor drift")
                body = body.replace(
                    gate,
                    (
                        "if (!claimed && retry)"
                        if control == 127
                        else (gate if control >= 128 else "if (0)")
                    ),
                )
                if control == 127:
                    body, count = re.subn(
                        r"\*retry \|= channel->buf != buffer.*?;", "*retry = 1;", body, flags=re.S
                    )
                    if count != 1:
                        raise RuntimeError("channel unsupported observation anchor drift")
                if control in (128, 129):
                    gate = "dvm_runtime_channel_buffer_busy(channel) ||"
                    if body.count(gate) != 1:
                        raise RuntimeError("channel busy refusal anchor drift")
                    body = body.replace(gate, "")
                if control in (130, 131):
                    gate = "channel->buf == buffer && quiet &&"
                    if body.count(gate) != 1:
                        raise RuntimeError("channel buffer identity anchor drift")
                    body = body.replace(gate, "quiet &&")
                fragment = work / "channel_retirement.inc"
                fragment.write_text(body)
                path = root / "main.c"
                main = path.read_text().replace(
                    '#include "host_runtime/channel_retirement.inc"',
                    '#include "' + str(fragment) + '"',
                )
                host = work / "main.c"
                host.write_text(private_includes(main, path))
            if control in mutations:
                before, after = mutations[control]
                if text.count(before) != 1:
                    raise RuntimeError("large owner mutation anchor drift: " + str(control))
                text = text.replace(before, after)
            rc = work / "runtime_rc.c"
            rc.write_text(text)
            binary = relink(
                runtime,
                work,
                Path(__file__).with_suffix(".c"),
                control,
                [
                    "Sys_werrstr",
                    "dvm_runtime_guest_sleep",
                    "malloc",
                    "free",
                    "poolfree",
                    "dvm_runtime_rc_defer",
                    "dvm_retirement_commit_ready",
                    "dvm_world_epoch_elapsed",
                    "dvm_retirement_reserve",
                    "dvm_runtime_root_batch_create",
                    "dvm_runtime_root_batch_publish",
                    "dvm_runtime_root_batch_clear",
                    "dvm_runtime_root_batch_destroy",
                    "dvm_runtime_rc_shared_leaves",
                    "dvm_runtime_channel_claim_observed",
                    "dvm_runtime_channel_waiter_published",
                ],
                runtime_rc_source=rc,
                runtime_main_source=host,
            )
            profiles = (
                [(mode, locked) for mode in ("--no-jit", "--jit") for locked in (False, True)]
                if control in positive
                else [("--no-jit", False)]
            )
            for mode, locked in profiles:
                result = subprocess.run(
                    [
                        str(binary),
                        "--native-spawn",
                        "--gc-idle-cycles",
                        "--stats",
                        mode,
                        *(["--gc-locked-poll"] if locked else []),
                        str(channel_image if channel_case else image),
                    ],
                    cwd=work,
                    capture_output=True,
                    text=True,
                    timeout=25,
                )
                output = result.stdout + result.stderr
                if "WARNING: ThreadSanitizer:" in output or "runtime error:" in output:
                    raise RuntimeError("large owner sanitizer failure\n" + output[-14000:])
                if control not in positive:
                    marker = {
                        100: "supported mapped owner must not require retained fallback",
                        106: "shared leaf retry survives retained backlog",
                        109: "failed last claim must withdraw child roots before destruction",
                        110: "quiescent cleanup must release shared packet pressure",
                        121: "channel claim captures temporary refusal under queue mutex",
                        122: "channel claim captures temporary refusal under queue mutex",
                        123: "channel claim captures temporary refusal under queue mutex",
                        124: "channel claim captures temporary refusal under queue mutex",
                        127: "unsupported transport channel remains a hard refusal",
                        128: "channel reservation refusal captured before the cause disappears",
                        129: "channel reservation refusal captured before the cause disappears",
                        130: "changed channel buffer must refuse the original snapshot",
                        131: "changed channel buffer must refuse the original snapshot",
                    }.get(control, "temporary leaf failure must preserve shared retry")
                    if result.returncode != 12 or marker not in output:
                        raise RuntimeError(
                            f"large owner control missed {control}\n{output[-14000:]}"
                        )
                    continue
                if result.returncode or f"PASS large owner release case={control} " not in output:
                    raise RuntimeError(
                        f"large owner {control} {mode} locked={locked}\n{output[-14000:]}"
                    )
                evidence(output, children=3 if channel_case else 2)
                if not re.search(r"NATIVERC enqueued=(\d+) released=\1 pending=0", output):
                    raise RuntimeError("large owner queue accounting failed\n" + output[-14000:])
    print(
        "PASS large owner release: "
        + ("32 channel" if channel_only else "156")
        + " C/JIT/poll profiles, sparse/boundary/2300/alias/list/array/channel, "
        "memory/FULL/BYTES retries, actual claim races, epochs and physical frees"
        + (
            "; positive only"
            if positive_only
            else (
                "; eleven channel semantic controls"
                if channel_only
                else "; thirty-two semantic omission controls"
            )
        )
    )


if __name__ == "__main__":
    main()
