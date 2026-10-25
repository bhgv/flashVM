#!/usr/bin/env python3
"""Run real installed products after relocating the whole package collection into a path with spaces."""
import argparse
from pathlib import Path
import os
import shutil
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
    result = subprocess.run([str(value) for value in argv], cwd=work, env=environment,
                            input=input_text, text=True, capture_output=True, timeout=60)
    if result.returncode:
        raise RuntimeError("installed command failed: " + repr(argv) + "\n"
                           + result.stdout + "\n" + result.stderr)
    return result.stdout


# Validate actual VM/parser/FIR/linker/native-tool behavior independently of the source build paths.
# CLI chooses prefix/scratch. Returns via exit status; only its temporary directory is removed.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--scratch", type=Path, required=True)
    args = parser.parse_args()
    args.scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="relocated packages ", dir=args.scratch.resolve()) as directory:
        base = Path(directory)
        prefix = base / "installed products"
        shutil.copytree(args.prefix.resolve(), prefix)
        work = base / "work"
        work.mkdir()
        environment = dict(os.environ, TMPDIR=str(work), TEMP=str(work), TMP=str(work))
        variants = sorted(path for path in (prefix / "flashvm").iterdir() if path.is_dir())
        if not variants:
            raise RuntimeError("no installed FlashVM variant")
        for variant in variants:
            runner = program(prefix, "flashvm/" + variant.name, "flashvm")
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
        for language, sample in (("as1", "Automatic.as"), ("as2", "Automatic.as"),
                                 ("as3", "Counter.as"), ("limbo", "Syntax.b")):
            run([program(prefix, language, language + "_parse"),
                 prefix / language / "examples" / sample], work, environment)
        # The installed script discovers its sibling SDKs; it must not require the original checkout.
        run(["bash", prefix / "fir/examples/firststeps/build.sh"], work, environment)
        output = run([program(prefix, "flashvm/" + variants[0].name, "flashvm"),
                      work / "firststeps-build/arithmetic.swf"], work, environment)
        if output.strip() != "42":
            raise RuntimeError("installed AS2-to-FIR-to-SWO-to-SWF pipeline failed")
        run([program(prefix, "symbols", "sblmap"), prefix / "ide/modules/idecore.sbl"], work, environment)
        for module in ("idecore", "uibridge", "ideinput", "idebridge", "idecomplete", "idebuiltins"):
            for suffix in (".dis", ".sbl"):
                if not (prefix / "ide/modules" / (module + suffix)).stat().st_size:
                    raise RuntimeError("empty installed IDE module/debug product")
        mkwork = work / "mk"
        mkwork.mkdir()
        run([program(prefix, "mk", "mk"), "-f", prefix / "mk/examples/hello.mkfile"], mkwork, environment)
        if "Hello from" not in (mkwork / "hello.txt").read_text():
            raise RuntimeError("installed mk did not execute its real recipe")
        (mkwork / "input.txt").write_text("copied by mk\n")
        run([program(prefix, "mk", "mk"), "-f", prefix / "mk/examples/copy.mkfile"], mkwork, environment)
        if (mkwork / "input.copy").read_text() != "copied by mk\n":
            raise RuntimeError("installed mk pattern rule failed")
        yaccwork = work / "iyacc"
        yaccwork.mkdir()
        yaccenv = dict(environment, ROOT=str(prefix / "iyacc"))
        run([program(prefix, "iyacc", "iyacc"), "-d", prefix / "iyacc/examples/sum.y"], yaccwork, yaccenv)
        run([os.environ.get("CC", "cc"), "y.tab.c", "-o", "sum"], yaccwork, environment)
        if run([yaccwork / "sum"], yaccwork, environment, "19+23\n").strip() != "42":
            raise RuntimeError("installed iyacc/generated parser example failed")
        for name in ("cocor", "cocor-limbo"):
            for grammar in ("number", "name"):
                generatorwork = work / (name + "-" + grammar)
                generatorwork.mkdir()
                generatorenv = dict(environment, CRFRAMES=str(prefix / name / "frames"))
                shutil.copyfile(prefix / name / "examples" / (grammar + ".atg"), generatorwork / (grammar + ".atg"))
                shutil.copyfile(prefix / name / "examples" / (grammar + ".atg"), generatorwork / (grammar + ".atg"))
                generated = run([program(prefix, name, "cocor"), grammar + ".atg"], generatorwork, generatorenv)
                if not list(generatorwork.glob("*p.c")) or not list(generatorwork.glob("*s.c")):
                    raise RuntimeError("installed Coco/R files: " + repr([p.name for p in generatorwork.iterdir()])
                                       + "\n" + generated)
        bootstrap = prefix / "limbo-bootstrap"
        run([program(prefix, "limbo-bootstrap", "limbo-bootstrap"), "-g", "-I", bootstrap / "module",
             "-I", bootstrap / "examples", "-o", work / "arithmetic.dis", bootstrap / "examples/arithmetic.b"],
            work, environment)
        if (work / "arithmetic.dis").read_bytes()[:4] != b"\xc0\x0c\x80\x30" or not (work / "arithmetic.sbl").stat().st_size:
            raise RuntimeError("installed bootstrap did not emit real Dis/debug data")
    print("PASS relocated packages: VM traces, numeric compiler/linker pipeline, IDE products and native tools")


if __name__ == "__main__":
    main()
