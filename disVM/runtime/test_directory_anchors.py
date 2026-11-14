# Copyright (c) 2026 bhgv
# EN: Real directory rename/replacement under four dedicated native namespace owners.
# RU: Настоящий rename/replacement каталогов при четырёх native namespace owners.
from pathlib import Path
import selectors
import shutil
import subprocess
import sys
import tempfile

from test_owner_location import compile_image
from test_debug_mailbox import verify
from test_native_kill import wait_marker
from relink_private import relink


# EN/RU: Prepare independent resources, optionally installing decoys after real rename.
# Подготовить независимые ресурсы; decoys устанавливаются после настоящего rename.
def resources(work, a, b):
    for name in ("source-a", "source-b", "moved-a", "moved-b", "visible", "other"):
        path = work / name
        if path.exists():
            shutil.rmtree(path)
    for name, token, image in (
        ("source-a", "A", a),
        ("source-b", "B", b),
        ("visible", "V", a),
        ("other", "O", b),
    ):
        path = work / name
        path.mkdir()
        (path / "token").write_text(token)
        shutil.copyfile(image, path / "provider.dis")


# EN: Drive only observed ready/rename/permit milestones. Keep the actual process
# and inherited streams owned until complete native retirement; always kill on timeout.
# RU: Двигаться только по ready/rename/permit. Владеть процессом и streams до
# полного native retirement; при timeout завершить процесс, не считать успехом.
def run(binary, image, work, a, b, mode, locked, rename, control):
    resources(work, a, b)
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
                wait_marker(poll, output, errors, b"ANCHORSREADY workers=4\n", True)
            (work / "source-a").rename(work / "moved-a")
            (work / "source-b").rename(work / "moved-b")
            for name, image_source in (("source-a", b), ("source-b", a)):
                path = work / name
                path.mkdir()
                (path / "token").write_text("Z")
                shutil.copyfile(image_source, path / "provider.dis")
            process.stdin.write(b"1")
            process.stdin.flush()
        tail, diagnostics = process.communicate(timeout=60)
        output.extend(tail)
        errors.extend(diagnostics)
    except BaseException:
        process.kill()
        process.communicate()
        raise
    text = (output + errors).decode(errors="replace")
    expected = 1 if control else 0
    if (
        process.returncode
        or f"ANCHORSDONE workers=4 bad={expected} cwd=A route=B module=11" not in text
        or "ANCHORLIFETIME " not in text
        or " live=0" not in text
        or "ThreadSanitizer:" in text
    ):
        raise RuntimeError("directory anchor acceptance failed\n" + text[-18000:])
    if control and "DETECTED directory path fallback value=90" not in text:
        raise RuntimeError("path fallback did not reach actual Z decoy")
    verify(text, 4)
    target = work / ("moved-a" if rename else "source-a")
    for index in range(4):
        if (target / f"created-{index}").read_text() != "C":
            raise RuntimeError("anchored create missed original resource")
        if rename and (work / "source-a" / f"created-{index}").exists():
            raise RuntimeError("create escaped into decoy directory")
    print(
        f"{'DETECTED real pathname fallback' if control else 'PASS directory anchors'} {mode} locked={locked} rename={rename}"
    )


# EN/RU: Compile ordinary public example and rename variant, then require actual
# lifetime/provenance evidence in both dispatch/poll paths and safe omission.
# Собрать пример и rename-вариант; требовать lifetime/provenance в обоих путях.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    example = root.parent / "examples/directory-anchors"
    providers = root.parent / "examples/namespace-load"
    with tempfile.TemporaryDirectory(prefix="directory anchors ") as temporary:
        work = Path(temporary)
        images = []
        for name in ("a", "b"):
            image = work / f"provider-{name}.dis"
            compile_image(
                compiler, include, root, providers / f"provider-{name}.b", image, (providers,)
            )
            images.append(image)
        source = (example / "main.b").read_text()
        if source.count("Rename: con 0;") != 1:
            raise RuntimeError("anchor example boundary changed")
        for control in (0, 1):
            binary = relink(
                runtime,
                work,
                root / "vfs/test_anchor_probe.c",
                control,
                [
                    "fvm_port_anchor_open",
                    "fvm_port_anchor_open_at",
                    "fvm_port_anchor_close",
                    "dvm_namespace_boot_shutdown",
                    "dvm_resource_open",
                ],
            )
            for rename in ((False, True) if not control else (True,)):
                selected = work / "selected.b"
                selected.write_text(
                    source.replace("Rename: con 0;", f"Rename: con {int(rename)};", 1)
                )
                image = work / "main.dis"
                compile_image(compiler, include, root, selected, image, (example,))
                for mode in ("--no-jit", "--jit"):
                    for locked in ((False, True) if not control else (False,)):
                        run(binary, image, work, *images, mode, locked, rename, control)


if __name__ == "__main__":
    main()
