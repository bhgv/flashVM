# Copyright (c) 2026 bhgv
# EN: Run both roles of the standalone C SDK against a real native DisVM.
# RU: Запустить обе роли самостоятельного SDK C с настоящей нативной DisVM.
import argparse
from pathlib import Path
from tempfile import TemporaryDirectory

from run import compile_image, exchange


# EN: Compile the same exports as the AS test, run calls, then check live VFS from C.
# RU: Собрать те же экспорты, что в тесте AS, вызвать их и проверить живую VFS из C.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("source", "runtime", "limbo", "driver"):
        parser.add_argument("--" + key, type=lambda path: Path(path).resolve(), required=True)
    parser.add_argument("--jit", action="store_true")
    parser.add_argument("--tcp", action="store_true")
    args = parser.parse_args()
    mode = "--jit" if args.jit else "--no-jit"
    fixtures = Path(__file__).resolve().parent
    with TemporaryDirectory(prefix="styx-live-sdk-") as scratch:
        directory = Path(scratch)
        module = compile_image(args, directory, fixtures / "live-stage_library.b", "stage-library.dis").name
        dispatcher = compile_image(args, directory, args.source / "limbo/scalarowner.b", "dispatcher.dis").name
        options = [part for name in ("words", "update") for part in ("--export", module, module, name)]
        exchange(directory,
            lambda fd: [args.runtime, "--styx-library-serve-fd", fd, *options, "--", "--native-spawn", mode, dispatcher],
            lambda fd: [args.driver, fd, "methods"], "PASS independent C SDK and native DisVM", tcp=args.tcp)
        (directory / "live/.var").mkdir(parents=True)
        (directory / "sdk-done").write_bytes(b"0")
        guest = compile_image(args, directory, fixtures / "live-stage_sdk.b", "sdk.dis").name
        exchange(directory, lambda fd: [args.driver, fd, "variables"],
            lambda fd: [args.runtime, "--styx-mount-fd", fd, "--at", "/peer", "--", "--native-spawn", mode, guest],
            "PASS DisVM observed C SDK writes", ("PASS independent C SDK and native DisVM",), tcp=args.tcp)


if __name__ == "__main__":
    main()
