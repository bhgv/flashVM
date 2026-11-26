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
def compile_movie(args, directory, source):
    fir = directory / "mixed.fir"
    optimized = directory / "mixed.opt.fir"
    obj = directory / "mixed.swo"
    movie = directory / "mixed.swf"
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
    args = parser.parse_args()
    tests = Path(__file__).resolve().parent
    mode = "--jit" if args.jit else "--no-jit"
    with TemporaryDirectory(prefix="styxrpc-mixed-pair-") as scratch:
        directory = Path(scratch)
        fixture_bytes = b"A\x00RPC"
        (directory / "rpc-source").write_bytes(fixture_bytes)
        (directory / "created").mkdir()
        for module in ("styxintref", "styxchannel"):
            compile_image(args, directory, args.source / "limbo" / (module + ".b"), module + ".dis")
        client = compile_image(args, directory, tests / "mixed_pair.b", "mixed.dis")
        compile_movie(args, directory, tests / "mixed_pair.as")
        paired_exchange(
            directory,
            lambda connection: [args.player, *connection, mode],
            lambda connection: [args.runtime, *connection, "--at", "/peer", "--",
                                "--native-spawn", mode, client.name],
            "PASS native mixed RPC paired VMs",
            tcp=args.tcp,
        )
        for created in ("created/rpc-file", "rpc-root-file"):
            if (directory / created).read_bytes() != fixture_bytes:
                raise RuntimeError("the movie did not create the expected VFS file")


if __name__ == "__main__":
    main()
