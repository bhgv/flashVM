# Copyright (c) 2026 bhgv
# EN: Compile both applications and run the ordinary paired executables over Styx.
# RU: Собрать оба приложения и запустить штатную пару через Styx.
import argparse
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory

from run import compile_image, paired_exchange


# EN: Run the project frontend, FIR optimizer/object emitter and SWF linker.
# RU: Запустить фронтенд проекта, оптимизатор и эмиттер FIR и линкер SWF.
def compile_movie(args, directory, source, name, companions=()):
    movie = directory / (name + ".swf")
    objects = []
    for unit in (source, *companions):
        fir = directory / (unit.stem + ".fir")
        optimized = directory / (unit.stem + ".opt.fir")
        obj = directory / (unit.stem + ".swo")
        for command in ([args.as2, unit, fir], [args.fir, "check", fir],
                        [args.fir_opt, fir, optimized], [args.fir, "object", optimized, obj]):
            subprocess.run(command, check=True, timeout=15)
        objects.append(obj)
    subprocess.run([args.swl, "link", movie, *objects], check=True, timeout=15)


# EN: Test the stock mounted exchange using only the compiled Limbo and AS applications.
# RU: Проверить штатный смонтированный обмен только с собранными приложениями Limbo и AS.
def main():
    parser = argparse.ArgumentParser()
    for field in ("source", "runtime", "player", "limbo", "as2", "fir", "fir-opt", "swl"):
        parser.add_argument("--" + field, type=lambda path: Path(path).resolve(), required=True)
    parser.add_argument("--jit", action="store_true")
    parser.add_argument("--tcp", action="store_true")
    profiles = parser.add_mutually_exclusive_group()
    profiles.add_argument("--named", action="store_true")
    profiles.add_argument("--typed", action="store_true")
    profiles.add_argument("--execute", action="store_true")
    args = parser.parse_args()
    tests = Path(__file__).resolve().parent
    name = "execute" if args.execute else "typed" if args.typed else "named" if args.named else "mixed"
    mode = "--jit" if args.jit else "--no-jit"
    with TemporaryDirectory(prefix="styxrpc-mixed-pair-") as scratch:
        directory = Path(scratch)
        fixture_bytes = b"A\x00RPC"
        if name == "mixed":
            (directory / "rpc-source").write_bytes(fixture_bytes)
            (directory / "created").mkdir()
        modules = {
            "execute": ("styxexec",),
            "typed": ("styxvalue", "styxnamed"),
            "named": ("styxvalue", "styxnamed"),
            "mixed": ("styxintref", "styxchannel"),
        }[name]
        for module in modules:
            compile_image(args, directory, args.source / "limbo" / (module + ".b"), module + ".dis")
        if args.named:
            (directory / "named-area").mkdir()
        client = compile_image(args, directory, tests / (name + "_pair.b"), name + ".dis")
        companions = (tests / "execute_apply.as", tests / "execute_refuse.as") if args.execute else ()
        compile_movie(args, directory, tests / (name + "_pair.as"), name, companions)
        paired_exchange(
            directory,
            lambda connection: [args.player, *connection, mode],
            lambda connection: [args.runtime, *connection, "--at", "/peer", "--",
                                "--native-spawn", mode, client.name],
            f"PASS native {name} RPC paired VMs",
            tcp=args.tcp,
        )
        if name == "mixed" and (directory / "created/nested").exists():
            raise RuntimeError("the movie did not remove its temporary directory")
        for created in (("created/rpc-file", "rpc-root-file") if name == "mixed" else ()):
            if (directory / created).read_bytes() != fixture_bytes:
                raise RuntimeError("the movie did not create the expected VFS file")


if __name__ == "__main__":
    main()
