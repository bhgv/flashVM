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
def compile_movie(args, directory, source, name):
    fir = directory / (name + ".fir")
    optimized = directory / (name + ".opt.fir")
    obj = directory / (name + ".swo")
    movie = directory / (name + ".swf")
    for command in ([args.as2, source, fir], [args.fir, "check", fir],
                    [args.fir_opt, fir, optimized], [args.fir, "object", optimized, obj],
                    [args.swl, "link", movie, obj]):
        subprocess.run(command, check=True, timeout=15)


# EN: Test the stock mounted exchange using only the compiled Limbo and AS applications.
# RU: Проверить штатный смонтированный обмен только с собранными приложениями Limbo и AS.
def main():
    parser = argparse.ArgumentParser()
    for field in ("source", "runtime", "player", "limbo", "as2", "fir", "fir-opt", "swl"):
        parser.add_argument("--" + field, type=lambda path: Path(path).resolve(), required=True)
    parser.add_argument("--jit", action="store_true")
    parser.add_argument("--tcp", action="store_true")
    parser.add_argument("--named", action="store_true")
    args = parser.parse_args()
    tests = Path(__file__).resolve().parent
    mode = "--jit" if args.jit else "--no-jit"
    with TemporaryDirectory(prefix="styxrpc-mixed-pair-") as scratch:
        directory = Path(scratch)
        fixture_bytes = b"A\x00RPC"
        if not args.named:
            (directory / "rpc-source").write_bytes(fixture_bytes)
            (directory / "created").mkdir()
        modules = ("styxnamed",) if args.named else ("styxintref", "styxchannel")
        for module in modules:
            compile_image(args, directory, args.source / "limbo" / (module + ".b"), module + ".dis")
        if args.named:
            (directory / "named-area").mkdir()
        name = "named" if args.named else "mixed"
        client = compile_image(args, directory, tests / (name + "_pair.b"), name + ".dis")
        compile_movie(args, directory, tests / (name + "_pair.as"), name)
        paired_exchange(
            directory,
            lambda connection: [args.player, *connection, mode],
            lambda connection: [args.runtime, *connection, "--at", "/peer", "--",
                                "--native-spawn", mode, client.name],
            f"PASS native {name} RPC paired VMs",
            tcp=args.tcp,
        )
        if not args.named and (directory / "created/nested").exists():
            raise RuntimeError("the movie did not remove its temporary directory")
        for created in (() if args.named else ("created/rpc-file", "rpc-root-file")):
            if (directory / created).read_bytes() != fixture_bytes:
                raise RuntimeError("the movie did not create the expected VFS file")


if __name__ == "__main__":
    main()
