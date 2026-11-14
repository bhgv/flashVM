# Copyright (c) 2026 bhgv
# EN: Routed live-variable result acceptance on actual native VM owners.
# RU: Routed live-variable result acceptance на actual native VM owners.
from pathlib import Path
import sys
import tempfile

from network_proxy_fixture import compile_fixture
from network_proxy_routed_results import peers, oracle, orphan
from network_proxy_routed_controls import controls


# EN/RU: Borrow matching runtime/compiler/include; own fixtures and execute C/JIT/poll matrix.
# Borrow matching runtime/compiler/include; владеть fixtures и исполнить C/JIT/poll matrix.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    positive = sys.argv[4:] == ["--positive-only"]
    only_controls = sys.argv[4:] == ["--controls-only"]
    if sys.argv[4:] not in ([], ["--positive-only"], ["--controls-only"]):
        raise RuntimeError("unsupported routed result test flags")
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="routed result ") as temporary:
        work = Path(temporary)
        compile_fixture(root, work, compiler, include)
        if not only_controls:
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    tag = mode[2:] + str(locked)
                    peers(runtime, work, mode, locked, tag)
                    oracle(runtime, work, mode, locked, tag + "-oracle")
                    orphan(runtime, work, mode, locked, tag + "-orphan")
                    orphan(runtime, work, mode, locked, tag + "-loss", True)
                    print("PASS live routed result " + tag, flush=True)
        if not positive:
            controls(runtime, root, work, compiler, include)


if __name__ == "__main__":
    main()
