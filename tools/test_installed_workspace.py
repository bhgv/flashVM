#!/usr/bin/env python3
"""Run real installed products after relocating the whole package collection into a path with spaces."""
import argparse
from pathlib import Path
import os
import shutil
import shlex
import re
import subprocess
import tempfile


# Resolve a required native program in an installed component, preserving .exe on Windows.
# prefix/component/name are trusted local paths/names. Returns a file path or raises a diagnostic.
def program(prefix, component, name):
    path = prefix / component / name
    if path.is_file():
        return path
    windows = path.with_name(path.name + ".exe")
    if windows.is_file():
        return windows
    raise RuntimeError("missing installed product: " + str(path))


# Execute argv literally in a private writable directory, with explicit package/temporary roots.
# input_text supplies optional stdin. Returns stdout; nonzero status includes captured diagnostics.
def run(argv, work, environment, input_text=None):
    result = subprocess.run(
        [str(value) for value in argv],
        cwd=work,
        env=environment,
        input=input_text,
        text=True,
        capture_output=True,
        timeout=60,
    )
    if result.returncode:
        raise RuntimeError(
            "installed command failed: " + repr(argv) + "\n" + result.stdout + "\n" + result.stderr
        )
    return result.stdout


# Compile/verify/optimize the shipped hybrid source with relocated tools and reference execution.
# Both the reference runner and the actual native Dis path are checked with explicit tools.
def table_packages(prefix, work, environment):
    source = prefix / "fir/examples/tables/main.b"
    fir = work / "tables.fir"
    optimized = work / "tables.opt.fir"
    run([program(prefix, "fir", "fir1_source"), "limbo", "--input", source, fir], work, environment)
    run([program(prefix, "fir", "fir1"), "check", fir], work, environment)
    run([program(prefix, "fir", "fir1"), "opt", fir, optimized], work, environment)
    for selected in (fir, optimized):
        result = run([program(prefix, "fir", "fir1_table_run"), selected], work, environment)
        if result.strip() != "42":
            raise RuntimeError("relocated table reference result differs")
    # Generated table Dis must execute on the matching relocated VM, including collection at every step.
    component = "disvm-runtime" if (prefix / "disvm-runtime").is_dir() else "disvm-editor"
    runtime = program(prefix, component, "disvm")
    image = work / "tables.dis"
    for selected in (fir, optimized):
        run([program(prefix, "fir", "fir1"), "dis", selected, image], work, environment)
        for mode in ("--jit", "--no-jit"):
            result = run([runtime, mode, "--quantum", 1, "--stats", "--inspect-words", 0, 2, image], work, environment)
            if "DATA 0 42\nDATA 8 1\n" not in result:
                raise RuntimeError("relocated generated table Dis result differs")
    # Additional modules cover native big arithmetic and checked division/bit operations.
    for example in ("arithmetic.b", "bitwise.b"):
        arithmetic_source = prefix / "fir/examples/tables" / example
        run([program(prefix, "fir", "fir1_source"), "limbo", "--input", arithmetic_source, fir], work, environment)
        run([program(prefix, "fir", "fir1"), "opt", fir, optimized], work, environment)
        for selected in (fir, optimized):
            if run([program(prefix, "fir", "fir1_table_run"), selected], work, environment).strip() != "42":
                raise RuntimeError("relocated big arithmetic reference result differs")
            run([program(prefix, "fir", "fir1"), "dis", selected, image], work, environment)
            for mode in ("--jit", "--no-jit"):
                result = run([runtime, mode, "--quantum", 1, "--inspect-words", 0, 2, image], work, environment)
                if "DATA 0 42\nDATA 8 1\n" not in result:
                    raise RuntimeError("relocated native big arithmetic result differs")
    # Relink the public compiler API without either the bootstrap or reference-execution archive.
    compiled_client = work / "table-compile.c"
    shutil.copyfile(prefix / "fir/examples/tables/compile.c", compiled_client)
    compiler_host = work / "table-compile"
    compiler_archives = [prefix / "fir/lib" / ("lib" + name + ".a")
                         for name in ("fir1_ir", "fir1_io")]
    run([environment.get("CC", "cc"), "-std=c99", "-I", prefix / "fir/include",
         compiled_client, *compiler_archives, "-lm", "-o", compiler_host], work, environment)
    run([compiler_host, fir, image], work, environment)
    if "DATA 0 42\nDATA 8 1\n" not in run([runtime, "--jit", "--quantum", 1, "--inspect-words", 0, 2, image], work, environment):
        raise RuntimeError("relocated public Dis compiler API result differs")
    # Link a fresh C99 host using only the installed public table API and archives.
    client = work / "table-client.c"
    shutil.copyfile(prefix / "fir/examples/tables/client.c", client)
    binary = work / "table-client"
    libraries = [prefix / "fir/lib" / ("lib" + name + ".a")
                 for name in ("fir1_tables", "fir1_ir", "fir1_io")]
    run([environment.get("CC", "cc"), "-std=c99", "-I", prefix / "fir/include",
         client, *libraries, "-lm", "-o", binary], work, environment)
    if run([binary, fir], work, environment).strip() != "42":
        raise RuntimeError("relocated public table C99 API result differs")
    # The multi-block big tutorial must also compile through the freshly relinked public API.
    big_cfg = prefix / "fir/examples/dis/big-cfg.fir"
    run([compiler_host, big_cfg, image], work, environment)
    for mode in ("--jit", "--no-jit"):
        result = run([runtime, mode, "--quantum", 1, "--inspect-words", 0, 4, image], work, environment)
        if "DATA 0 -9007199254740993\nDATA 8 0\nDATA 16 1\nDATA 24 0\n" not in result:
            raise RuntimeError("relocated big CFG library/example result differs")
    # Exercise source decisions through the shipped in-process source+Dis compiler client.
    decision_source = prefix / "fir/examples/dis/big-decisions.b"
    run([program(prefix, "fir", "fir1_dis_example"), decision_source, image], work, environment)
    for mode in ("--jit", "--no-jit"):
        result = run([runtime, mode, "--quantum", 1, "--inspect-words", 0, 6, image], work, environment)
        if "DATA 0 -9223372036854775808\nDATA 8 0\nDATA 16 0\nDATA 24 1\nDATA 32 0\nDATA 40 0\n" not in result:
            raise RuntimeError("relocated big source decision library/example result differs")
    # Exercise DAG owner lifetimes through freshly relinked compiler and reference C99 clients.
    dag_fir = work / "table-dag.fir"
    run([program(prefix, "fir", "fir1_source"), "limbo", "--input",
         prefix / "fir/examples/tables/decisions.b", dag_fir], work, environment)
    if run([binary, dag_fir], work, environment).strip() != "42":
        raise RuntimeError("relocated table DAG reference client result differs")
    run([compiler_host, dag_fir, image], work, environment)
    for mode in ("--jit", "--no-jit"):
        result = run([runtime, mode, "--quantum", 1, "--inspect-words", 0, 6, image], work, environment)
        if "DATA 0 42\nDATA 8 0\nDATA 16 0\nDATA 24 1\nDATA 32 0\nDATA 40 0\n" not in result:
            raise RuntimeError("relocated table DAG compiler client result differs")
    # Relink source+FIR APIs too: the source-to-Dis client runs no compiler subprocess.
    source_host = work / "table-source-compile"
    source_archives = [prefix / "fir/lib" / ("lib" + name + ".a")
                       for name in ("fir1_ir", "fir1_source_api", "fir1_io")]
    run([environment.get("CC", "cc"), "-std=c99", "-I", prefix / "fir/include",
         prefix / "fir/examples/dis/main.c", *source_archives, "-lm", "-o", source_host], work, environment)
    # Branching tutorials have two trace cells followed by their two execution flags.
    # Relinked C99 clients exercise both original/optimized IR and native VM modes.
    for tutorial in ("shifts", "division", "mutation", "arms", "comparisons", "short-circuit", "compound", "unicode", "int-arithmetic", "text-bits", "int-shifts", "text-values", "constructors", "relative", "splice"):
        tutorial_source = prefix / "fir/examples/tables" / (tutorial + ".b")
        if image.exists():
            image.unlink()
        run([source_host, tutorial_source, image], work, environment)
        for mode in ("--jit", "--no-jit"):
            direct = run([runtime, mode, "--quantum", 1, "--inspect-words", 0, 4, image], work, environment)
            if "DATA 0 42\nDATA 8 0\nDATA 16 1\nDATA 24 0\n" not in direct:
                raise RuntimeError("relocated " + tutorial + " source-library client result differs")
        tutorial_fir = work / ("table-" + tutorial + ".fir")
        tutorial_opt = work / ("table-" + tutorial + "-opt.fir")
        run([program(prefix, "fir", "fir1_source"), "limbo", "--input",
             prefix / "fir/examples/tables" / (tutorial + ".b"), tutorial_fir], work, environment)
        run([program(prefix, "fir", "fir1"), "opt", tutorial_fir, tutorial_opt], work, environment)
        for selected in (tutorial_fir, tutorial_opt):
            if run([binary, selected], work, environment).strip() != "42":
                raise RuntimeError("relocated " + tutorial + " reference client result differs")
            run([compiler_host, selected, image], work, environment)
            for mode in ("--jit", "--no-jit"):
                result = run([runtime, mode, "--quantum", 1, "--inspect-words", 0, 4, image], work, environment)
                if "DATA 0 42\nDATA 8 0\nDATA 16 1\nDATA 24 0\n" not in result:
                    raise RuntimeError("relocated " + tutorial + " compiler client result differs")




# Compile the shipped tutorial using only relocated SDK headers/libraries, then run it.
# prefix/work/environment are private test paths/state; failures raise with diagnostics.
def module_packages(prefix, work, environment):
    """Detect missing transitive SDK headers and source-checkout dependencies."""
    host = prefix / "disvm-host"
    ide = prefix / "ide/modules"
    for module in (
        "idecore",
        "uibridge",
        "ideinput",
        "idebridge",
        "idecomplete",
        "idebuiltins",
        "ideworkspace",
        "workspace-example",
    ):
        for suffix in (".dis", ".sbl"):
            if not (ide / (module + suffix)).stat().st_size:
                raise RuntimeError("empty installed service/example product")
    image = ide / "ideworkspace.dis"
    size = str(image.stat().st_size)
    output = run(
        [program(prefix, "disvm-host", "disvm_module_store_example"), image, size],
        work,
        environment,
    )
    if "PASS stored " not in output or "no VM execution" not in output:
        raise RuntimeError("installed module example returned unexpected evidence")
    if os.name != "posix":
        raise RuntimeError("native SDK compilation acceptance currently requires a POSIX host")
    executable = work / "module-sdk-client"
    libraries = [
        host / "lib" / ("lib" + name + ".a")
        for name in (
            "disvm_module_store",
            "disvm_flashvm_host",
            "disvm_module_image",
            "disvm_shared_ports",
        )
    ]
    run(
        [
            environment.get("CC", "cc"),
            "-std=c99",
            "-I" + str(host / "include"),
            host / "examples/module-store/main.c",
            *libraries,
            "-pthread",
            "-o",
            executable,
        ],
        work,
        environment,
    )
    output = run([executable, image, size], work, environment)
    if "PASS stored " not in output:
        raise RuntimeError("relocated public SDK client failed")


# Compile our numeric Limbo source through relocated CLIs and the shipped C99 libraries.
# prefix/work/environment are private test state. Require a matching DisVM package, compare
# emitted bytes independently of tool messages and execute the value in both dispatch modes.
def fir_dis_packages(prefix, work, environment):
    component = prefix / "fir"
    source = component / "examples/dis/main.b"
    fir = work / "answer.fir"
    binary = work / "answer.dis"
    embedded = work / "embedded.dis"
    run([program(prefix, "fir", "fir1_source"), "limbo", "--input", source, fir,
         "--module", "dis_example"], work, environment)
    run([program(prefix, "fir", "fir1"), "dis", fir, binary], work, environment)
    run([program(prefix, "fir", "fir1_dis_example"), source, embedded], work, environment)
    if binary.read_bytes() != embedded.read_bytes():
        raise RuntimeError("installed CLI and embedded Dis generators disagree")
    if os.name != "posix":
        raise RuntimeError("native FIR SDK compilation acceptance currently requires a POSIX host")
    executable = work / "fir-sdk-client"
    libraries = [component / "lib" / ("lib" + name + ".a")
                 for name in ("fir1_ir", "fir1_source_api", "fir1_io")]
    run([*shlex.split(environment.get("CC", "cc")), "-std=c99", "-I" + str(component / "include"),
         component / "examples/dis/main.c", *libraries, "-o", executable], work, environment)
    run([executable, source, embedded], work, environment)
    if binary.read_bytes() != embedded.read_bytes():
        raise RuntimeError("relinked installed C99 libraries changed the generated Dis")
    for name in ("disvm-editor", "disvm-runtime"):
        if (prefix / name / "disvm").is_file():
            runtime = program(prefix, name, "disvm")
            break
    else:
        raise RuntimeError("FIR→Dis acceptance requires an installed NC64 DisVM runtime")
    for mode in ("--jit", "--no-jit"):
        output = run([runtime, mode, "--inspect-words", "0", "1", binary], work, environment)
        if "DATA 0 2147483647\n" not in output:
            raise RuntimeError("relocated FIR/Dis execution produced the wrong i32 result")


    # A second input proves that the shipped source library and emitter support terminal CFG,
    # and that an unvisited first slot cannot masquerade as the selected return value.
    decision = component / "examples/dis/decision.b"
    run([program(prefix, "fir", "fir1_source"), "limbo", "--input", decision, fir,
         "--module", "dis_example"], work, environment)
    run([program(prefix, "fir", "fir1"), "dis", fir, binary], work, environment)
    run([executable, decision, embedded], work, environment)
    if binary.read_bytes() != embedded.read_bytes():
        raise RuntimeError("relocated terminal-decision CLI and relinked C99 client disagree")
    for mode in ("--jit", "--no-jit"):
        output = run([runtime, mode, "--max-instructions", "1000", "--inspect-words", "0", "6", binary],
                     work, environment)
        rows = [tuple(map(int, line.split()[1:])) for line in output.splitlines() if line.startswith("DATA ")]
        if rows != [(0, 0), (8, 42), (16, 0), (24, 0), (32, 1), (40, 0)]:
            raise RuntimeError("relocated decision lost its selected value or capture-validity flags")


    # A numeric AS3 input reaches the same installed C99 libraries, now including binary64 Dis.
    # The relinked client selects its profile explicitly; no build-checkout headers are available.
    reals = component / "examples/dis/reals.as"
    run([program(prefix, "fir", "fir1_source"), "as3", "--input", reals, fir,
         "--module", "dis_example"], work, environment)
    run([program(prefix, "fir", "fir1"), "dis", fir, binary], work, environment)
    run([executable, reals, embedded, "as3"], work, environment)
    if binary.read_bytes() != embedded.read_bytes():
        raise RuntimeError("relocated AS3 binary64 CLI and C99 client disagree")
    for mode in ("--jit", "--no-jit"):
        output = run([runtime, mode, "--inspect-reals", "0", "3", binary], work, environment)
        rows = [(int(line.split()[1]), float(line.split()[2])) for line in output.splitlines()
                if line.startswith("REAL ")]
        if rows != [(0, -11.5), (8, 3.75), (16, -1.25)]:
            raise RuntimeError("relocated AS3 binary64 execution produced wrong values")


    # The same relocated Limbo client selects real from the declared return type, not an argv shortcut.
    real_source = component / "examples/dis/real.b"
    run([program(prefix, "fir", "fir1_source"), "limbo", "--input", real_source, fir,
         "--module", "dis_example"], work, environment)
    run([program(prefix, "fir", "fir1"), "dis", fir, binary], work, environment)
    run([executable, real_source, embedded], work, environment)
    if binary.read_bytes() != embedded.read_bytes():
        raise RuntimeError("relocated Limbo real CLI and C99 client disagree")
    for mode in ("--jit", "--no-jit"):
        output = run([runtime, mode, "--inspect-reals", "0", "1", binary], work, environment)
        if "REAL 0 42.5\n" not in output:
            raise RuntimeError("relocated Limbo real execution produced the wrong result")


# Relink the installed C99 broker example and decode its exact compiler-origin record on Dis.
# All inputs come from the relocated package; no checkout header/library path is used.
def broker_packages(prefix, work, environment):
    component = prefix / "disvm-runtime"
    if not component.is_dir():
        component = prefix / "disvm-editor"
    # Relocation must preserve the linked user documentation, not only executable products.
    guides = [component / name for name in ("PROJECTIONS.md", "VALUES.md", "VALUES.ru.md", "LIVE-PROJECTIONS.md", "LIVE-PROJECTIONS.ru.md", "WINDOWS-GUTTER.md", "WINDOWS-GUTTER.ru.md")]
    guides += [component / "docs/dis" / guide.name for guide in list(guides) if (component / "docs/dis" / guide.name).is_file()]
    guides += list((component / "examples/projection").glob("*.md"))
    guides += list((component / "examples/broker").glob("*.md"))
    for guide in guides:
        for target in re.findall(r"\]\(([^)]+)\)", guide.read_text()):
            if "://" in target or target.startswith("#"):
                continue
            if not (guide.parent / target.split("#", 1)[0]).exists():
                raise RuntimeError("broken relocated guide link: " + str(guide) + " -> " + target)
    libraries = list((component / "lib").rglob("libdisvm_broker.a"))
    if len(libraries) != 1:
        raise RuntimeError("installed broker archive is missing or ambiguous")
    executable = work / "broker-sdk-client"
    run([*shlex.split(environment.get("CC", "cc")), "-std=c99", "-I" + str(component / "include/disvm"),
         component / "examples/broker/frame_example.c", libraries[0], "-o", executable], work, environment)
    source, fir, origins = (work / name for name in ("source.as", "program.fir", "origins.json"))
    source.write_text("var o = 2; o = o + 5 * o;")
    run([program(prefix, "fir", "fir1_source"), "as3", "--input", source, fir, "-g", origins], work, environment)
    import hashlib
    import json
    debug = json.loads(origins.read_text())
    reads = [event for event in debug["events"] if event["kind"] == "read"]
    if len(reads) != 2:
        raise RuntimeError("installed compiler lacks exact repeated-name access sites")
    selected = reads[1]
    frame = run([executable, source, selected["line"], selected["column"], selected["byte_offset"],
                 selected["byte_length"], selected["binding"], hashlib.sha256(source.read_bytes()).hexdigest(),
                 debug["module"]], work, environment)
    (work / "diagnostic.bd1").write_text(frame, encoding="ascii")
    shutil.copyfile(component / "brokerdiag.dis", work / "brokerdiag.dis")
    binary = work / "diagnostic.dis"
    run([program(prefix, component.name, "limbo-native"), "-I", component / "include/limbo", "-o", binary,
         component / "examples/broker/examples/diagnostic.b"], work, environment)
    for mode in ("--jit", "--no-jit"):
        result = run([program(prefix, component.name, "disvm"), mode, binary], work, environment)
        if "DIAG 1 0000000000000001 " not in result or str(source) not in result:
            raise RuntimeError("relocated typed diagnostic channel lost its source/request")


    # Recompile the shipped inspector example using only installed headers/modules.
    for name in ("ideprojection.dis", "ideinspect.dis", "idevalue.dis", "idedocprojection.dis"):
        shutil.copyfile(component / name, work / name)
    projection = work / "projection-check.dis"
    run([program(prefix, component.name, "limbo-native"), "-I", component / "include/limbo",
         "-o", projection, component / "examples/projection/main.b"], work, environment)
    for mode in ("--jit", "--no-jit"):
        result = run([program(prefix, component.name, "disvm"), mode, projection], work, environment)
        if "PASS projection inspector:" not in result:
            raise RuntimeError("relocated inspector projection/model failed")


    # The shipped live document adapter must execute against a genuine installed Idecore owner.
    shutil.copyfile(component / "idecore.dis", work / "idecore.dis")
    document_example = work / "document-projection.dis"
    run([program(prefix, component.name, "limbo-native"), "-I", component / "include/limbo",
         "-o", document_example, component / "examples/document-projection/main.b"], work, environment)
    for mode in ("--jit", "--no-jit"):
        result = run([program(prefix, component.name, "disvm"), mode, document_example], work, environment)
        if "PASS live document projections:" not in result:
            raise RuntimeError("relocated live document projection failed")


    # Recompile the native hybrid example using only its relocated matching module declaration.
    hybrid_example = work / "hybrid-table.dis"
    run([program(prefix, component.name, "limbo-native"), "-I", component / "include/limbo",
         "-o", hybrid_example, component / "examples/hybrid-table/main.b"], work, environment)
    for mode in ("--jit", "--no-jit"):
        result = run([program(prefix, component.name, "disvm"), mode, hybrid_example], work, environment)
        if "PASS native hybrid tables:" not in result:
            raise RuntimeError("relocated native hybrid table failed")


    # A graphical package must render with its own shipped font/modules, without build-tree paths.
    if component.name == "disvm-editor":
        import struct
        replay = work / "installed-window-events.bin"
        frame = b"fvd1:" + "".join(format(value, "08x") for value in (1, 1, 65539, 0, 0, 0)).encode("ascii")
        replay.write_bytes(struct.pack("<I", len(frame)) + frame)
        capture = work / "installed-editor.ppm"
        for entry in ("graphical-editor.dis", "ide.dis"):
            for mode in ("--jit", "--no-jit"):
                result = run([program(prefix, component.name, "disvm-editor"), mode, "--document", source,
                              "--events", replay, "--capture", capture, component / entry], work, environment)
                if "GUTTER 1 " not in result or not capture.read_bytes().startswith(b"P6\n960 640\n255\n"):
                    raise RuntimeError("relocated IDE application lost default gutter/font/raster support")
        if not (component / "run-ide.sh").is_file():
            raise RuntimeError("relocated IDE launcher is missing")
        for mode in ("--jit", "--no-jit"):
            result = run(["sh", component / "run-ide.sh", mode, "--document", source,
                          "--events", replay, "--capture", capture], work, environment)
            if "GUTTER 1 " not in result or not capture.read_bytes().startswith(b"P6\n960 640\n255\n"):
                raise RuntimeError("relocated IDE launcher failed its actual application startup")



# Validate actual VM/parser/FIR/linker/native-tool behavior independently of the source build paths.
# CLI chooses prefix/scratch. Returns via exit status; only its temporary directory is removed.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--scratch", type=Path, required=True)
    parser.add_argument("--broker-only", action="store_true",
                        help="Check relocated broker C99 SDK, compiler access sites and real Dis channels")
    parser.add_argument("--fir-dis-only", action="store_true",
                        help="Check relocated FIR CLIs, public C99 libraries and direct Dis execution")
    parser.add_argument(
        "--modules-only",
        action="store_true",
        help="Check relocated Dis host and IDE packages without requiring other packages",
    )
    args = parser.parse_args()
    if sum((args.modules_only, args.fir_dis_only, args.broker_only)) > 1:
        parser.error("choose one relocated-package subset")
    args.scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix="relocated packages ", dir=args.scratch.resolve()
    ) as directory:
        base = Path(directory)
        prefix = base / "installed products"
        shutil.copytree(args.prefix.resolve(), prefix)
        work = base / "work"
        work.mkdir()
        environment = dict(os.environ, TMPDIR=str(work), TEMP=str(work), TMP=str(work))
        if args.broker_only:
            table_packages(prefix, work, environment)
            broker_packages(prefix, work, environment)
            print("PASS relocated broker C99 SDK, exact source sites, diagnostic channels and IDE projections, interpreter/JIT")
            return
        if args.fir_dis_only:
            table_packages(prefix, work, environment)
            fir_dis_packages(prefix, work, environment)
            print("PASS relocated our Limbo/FIR→Dis, relinked C99 SDK, interpreter and native JIT")
            return
        if args.modules_only:
            module_packages(prefix, work, environment)
            print("PASS relocated Dis/IDE modules, native tutorial and public SDK client")
            return
        if (prefix / "disvm-host/lib/libdisvm_module_store.a").is_file():
            module_packages(prefix, work, environment)
        variants = sorted(path for path in (prefix / "flashvm").iterdir() if path.is_dir())
        if not variants:
            raise RuntimeError("no installed FlashVM variant")
        for variant in variants:
            runner = program(prefix, "flashvm/" + variant.name, "flashvm-console")
            for sample, expected in (("constant", "123"), ("arithmetic", "42")):
                output = run([runner, variant / (sample + ".swf")], work, environment)
                if output.strip() != expected:
                    raise RuntimeError("incorrect relocated movie trace: " + output)
            for name in ("01-animation-audio.swf", "02-input-audio.swf", "03-stop-sounds.swf"):
                data = (variant / "examples/player/animation_audio" / name).read_bytes()
                if data[:3] != b"FWS" or data[3] > 8:
                    raise RuntimeError("missing/invalid installed graphics-input-sound movie")
            for name in ("video-only.swf", "video-audio.swf"):
                data = (variant / "examples/video" / name).read_bytes()
                if data[:4] != b"FWS\x06" or len(data) < 1000:
                    raise RuntimeError("missing/invalid installed video compatibility fixture")
        for language, sample in (
            ("as1", "Automatic.as"),
            ("as2", "Automatic.as"),
            ("as3", "Counter.as"),
            ("limbo", "Syntax.b"),
        ):
            run(
                [
                    program(prefix, language, language + "_parse"),
                    prefix / language / "examples" / sample,
                ],
                work,
                environment,
            )
        # The installed script discovers its sibling SDKs; it must not require the original checkout.
        run(["bash", prefix / "fir/examples/firststeps/build.sh"], work, environment)
        output = run(
            [
                program(prefix, "flashvm/" + variants[0].name, "flashvm-console"),
                work / "firststeps-build/arithmetic.swf",
            ],
            work,
            environment,
        )
        if output.strip() != "42":
            raise RuntimeError("installed AS2-to-FIR-to-SWO-to-SWF pipeline failed")
        if any((prefix / name / "disvm").is_file() for name in ("disvm-editor", "disvm-runtime")):
            fir_dis_packages(prefix, work, environment)
        run(
            [program(prefix, "symbols", "sblmap"), prefix / "ide/modules/idecore.sbl"],
            work,
            environment,
        )
        for module in (
            "idecore",
            "uibridge",
            "ideinput",
            "idebridge",
            "idecomplete",
            "idebuiltins",
            "ideworkspace",
            "workspace-example",
        ):
            for suffix in (".dis", ".sbl"):
                if not (prefix / "ide/modules" / (module + suffix)).stat().st_size:
                    raise RuntimeError("empty installed IDE module/debug product")
        mkwork = work / "mk"
        mkwork.mkdir()
        run(
            [program(prefix, "mk", "mk"), "-f", prefix / "mk/examples/hello.mkfile"],
            mkwork,
            environment,
        )
        if "Hello from" not in (mkwork / "hello.txt").read_text():
            raise RuntimeError("installed mk did not execute its real recipe")
        (mkwork / "input.txt").write_text("copied by mk\n")
        run(
            [program(prefix, "mk", "mk"), "-f", prefix / "mk/examples/copy.mkfile"],
            mkwork,
            environment,
        )
        if (mkwork / "input.copy").read_text() != "copied by mk\n":
            raise RuntimeError("installed mk pattern rule failed")
        yaccwork = work / "iyacc"
        yaccwork.mkdir()
        yaccenv = dict(environment, ROOT=str(prefix / "iyacc"))
        run(
            [program(prefix, "iyacc", "iyacc"), "-d", prefix / "iyacc/examples/sum.y"],
            yaccwork,
            yaccenv,
        )
        run([os.environ.get("CC", "cc"), "y.tab.c", "-o", "sum"], yaccwork, environment)
        if run([yaccwork / "sum"], yaccwork, environment, "19+23\n").strip() != "42":
            raise RuntimeError("installed iyacc/generated parser example failed")
        for name in ("cocor", "cocor-limbo"):
            for grammar in ("number", "name"):
                generatorwork = work / (name + "-" + grammar)
                generatorwork.mkdir()
                generatorenv = dict(environment, CRFRAMES=str(prefix / name / "frames"))
                shutil.copyfile(
                    prefix / name / "examples" / (grammar + ".atg"),
                    generatorwork / (grammar + ".atg"),
                )
                shutil.copyfile(
                    prefix / name / "examples" / (grammar + ".atg"),
                    generatorwork / (grammar + ".atg"),
                )
                generated = run(
                    [program(prefix, name, "cocor"), grammar + ".atg"], generatorwork, generatorenv
                )
                if not list(generatorwork.glob("*p.c")) or not list(generatorwork.glob("*s.c")):
                    raise RuntimeError(
                        "installed Coco/R files: "
                        + repr([p.name for p in generatorwork.iterdir()])
                        + "\n"
                        + generated
                    )
        bootstrap = prefix / "limbo-bootstrap"
        run(
            [
                program(prefix, "limbo-bootstrap", "limbo-bootstrap"),
                "-g",
                "-I",
                bootstrap / "module",
                "-I",
                bootstrap / "examples",
                "-o",
                work / "arithmetic.dis",
                bootstrap / "examples/arithmetic.b",
            ],
            work,
            environment,
        )
        if (work / "arithmetic.dis").read_bytes()[:4] != b"\xc0\x0c\x80\x30" or not (
            work / "arithmetic.sbl"
        ).stat().st_size:
            raise RuntimeError("installed bootstrap did not emit real Dis/debug data")
    print(
        "PASS relocated packages: VM traces, numeric compiler/linker pipeline, IDE products and native tools"
    )


if __name__ == "__main__":
    main()
