# Copyright (c) 2026 bhgv
# EN: Real unlink/rmdir in retained guest namespaces, with rename and actual wrong-path omission.
# RU: Настоящие unlink/rmdir в удержанном guest namespace с rename и wrong-path omission.
from pathlib import Path
import selectors
import shutil
import subprocess
import sys
import tempfile

from relink_private import relink
from test_owner_location import compile_image
from test_native_kill import wait_marker
from test_debug_mailbox import verify


# EN/RU: Create isolated files, empty/full directories and four immutable module images.
# Создать isolated files, empty/full directories и четыре immutable module images.
def resources(work, provider):
    for name in ("source-a", "moved-a", "visible"):
        path = work / name
        if path.exists():
            shutil.rmtree(path)
    (work / "visible").mkdir()
    source = work / "source-a"
    source.mkdir()
    (source / "one-winner").write_text("A")
    for index in range(4):
        for kind, value in (
            ("victim", "A"),
            ("alias-source", "S"),
            ("alias-target", "V"),
            ("publication", "P"),
        ):
            (source / f"{kind}-{index}").write_text(value)
        (source / f"empty-{index}").mkdir()
        (source / f"full-{index}").mkdir()
        (source / f"full-{index}/keep").write_text("K")
        shutil.copyfile(provider, source / f"provider-{index}.dis")


# EN: Coordinate actual observed readiness before host rename. Require exact
# physical file effects, successful native retirement and zero live anchors.
# RU: Дождаться actual readiness до rename. Проверить точные physical effects,
# native retirement и zero live anchors; timeout/crash are never accepted.
def run(binary, image, work, provider, mode, locked, rename, control):
    resources(work, provider)
    process = subprocess.Popen(
        [
            str(binary),
            mode,
            "--stats",
            "--quantum",
            "7",
            *(["--gc-locked-poll"] if locked else []),
            str(image),
        ],
        cwd=work,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    output, errors = bytearray(), bytearray()
    try:
        if rename:
            with selectors.DefaultSelector() as poll:
                poll.register(process.stdout, selectors.EVENT_READ, output)
                poll.register(process.stderr, selectors.EVENT_READ, errors)
                wait_marker(poll, output, errors, b"REMOVEREADY workers=4\n", True)
            (work / "source-a").rename(work / "moved-a")
            (work / "source-a").mkdir()
            for index in range(4):
                (work / f"source-a/victim-{index}").write_text("Z")
            process.stdin.write(b"1")
            process.stdin.flush()
        tail, diagnostics = process.communicate(timeout=45)
        output.extend(tail)
        errors.extend(diagnostics)
    except BaseException:
        process.kill()
        process.communicate()
        raise
    text = (output + errors).decode(errors="replace")
    if (
        process.returncode
        or f"REMOVEDONE workers=4 bad={control} winners=1 file=A module=11" not in text
        or "REMOVE_PUBLICATION calls=4" not in text
        or "ANCHORLIFETIME " not in text
        or " live=0" not in text
        or "ThreadSanitizer:" in text
    ):
        raise RuntimeError("namespace remove acceptance failed\n" + text[-16000:])
    verify(text, 4)
    source = work / ("moved-a" if rename else "source-a")
    for index in range(4):
        if (source / f"victim-{index}").exists() != (control == 1 and index == 0):
            raise RuntimeError("wrong original victim state")
        for kind in ("empty", "full", "provider", "alias-source", "publication"):
            suffix = ".dis" if kind == "provider" else ""
            if (source / f"{kind}-{index}{suffix}").exists():
                raise RuntimeError("remove effect missing: " + kind)
        if (source / f"alias-target-{index}").read_text() != "V":
            raise RuntimeError("file binding deleted physical target")
        if rename and (work / f"source-a/victim-{index}").exists() != (not control or index != 0):
            raise RuntimeError("wrong actual decoy deletion state")
    if (source / "one-winner").exists():
        raise RuntimeError("shared-name removal missed")
    print(
        f"{'DETECTED actual decoy deletion' if control else 'PASS namespace remove'} {mode} locked={locked} rename={rename}"
    )


# EN/RU: Compile public example and controlled rename variant; run both dispatch/poll
# paths and safe omission using actual kernel removals, without vendor source edits.
# Собрать пример/rename variant; оба dispatch/poll и omission с actual kernel removals.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    example = root.parent / "examples/namespace-remove"
    with tempfile.TemporaryDirectory(prefix="namespace remove ") as temporary:
        work = Path(temporary)
        provider = work / "provider.dis"
        providers = root.parent / "examples/namespace-load"
        compile_image(compiler, include, root, providers / "provider-a.b", provider, (providers,))
        source = (example / "main.b").read_text()
        if source.count("Rename: con 0;") != 1:
            raise RuntimeError("rename fixture boundary drift")
        for control in (0, 1):
            binary = relink(
                runtime,
                work,
                root / "vfs/test_remove_probe.c",
                control,
                [
                    "fvm_port_anchor_open",
                    "fvm_port_anchor_open_at",
                    "fvm_port_anchor_close",
                    "dvm_namespace_boot_shutdown",
                    "dvm_resource_open",
                    "dvm_resource_remove",
                ],
            )
            for rename in ((False, True) if not control else (True,)):
                selected = work / "selected.b"
                selected.write_text(source.replace("Rename: con 0;", f"Rename: con {int(rename)};"))
                image = work / "main.dis"
                compile_image(compiler, include, root, selected, image, (example,))
                for mode in ("--no-jit", "--jit"):
                    for locked in ((False, True) if not control else (False,)):
                        run(binary, image, work, provider, mode, locked, rename, control)


if __name__ == "__main__":
    main()
