# Copyright (c) 2026 bhgv
# EN: Compile the native result-channel client and run it against the independent C provider.
# RU: Собрать нативного клиента каналов результата и запустить с независимым провайдером C.
import argparse
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests/flashvm"))
from run import compile_image, exchange


# EN: Build the same Limbo contract used by the stock FlashVM exercise, with no movie in the mount.
# RU: Собрать тот же контракт Limbo, что у штатной проверки FlashVM, без фильма на монтировании.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "runtime", "limbo", "device"):
        parser.add_argument("--" + name, type=lambda path: Path(path).resolve(), required=True)
    parser.add_argument("--jit", action="store_true")
    parser.add_argument("--tcp", action="store_true")
    args = parser.parse_args()
    fixture = Path(__file__).resolve().parents[2] / "tests/flashvm/result-channels_pair.b"
    mode = "--jit" if args.jit else "--no-jit"
    with TemporaryDirectory(prefix="styxrpc-result-c-") as scratch:
        directory = Path(scratch)
        image = compile_image(args, directory, fixture, "client.dis")
        exchange(directory, lambda fd: [args.device, fd],
                 lambda fd: [args.runtime, "--styx-mount-fd", fd, "--at", "/peer",
                             "--", "--native-spawn", mode, image.name],
                 "PASS native result-channels RPC paired VMs",
                 ("PASS independent C result provider",), tcp=args.tcp)


if __name__ == "__main__":
    main()
