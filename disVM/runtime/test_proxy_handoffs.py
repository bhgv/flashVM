# Copyright (c) 2026 bhgv
# EN: Preserve absolute deadlines at source/owner handoffs under actual native execution.
# RU: Сохранить absolute deadlines на source/owner handoffs при actual native execution.
from pathlib import Path
import sys
import tempfile

from network_proxy_fixture import compile_fixture
from proxy_handoff_cases import prepare, check


# EN/RU: Borrow matched runtime/compiler/include; own private gaps and exact semantic omissions.
# Borrow matched runtime/compiler/include; владеть private gaps и exact semantic omissions.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    positive = sys.argv[4:] == ["--positive-only"]
    only_controls = sys.argv[4:] == ["--controls-only"]
    if sys.argv[4:] and not positive and not only_controls:
        raise RuntimeError("unsupported handoff flags")
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="proxy handoff ") as temporary:
        work = Path(temporary)
        compile_fixture(root, work, compiler, include)
        prepare(root, work, compiler, include)
        if not only_controls:
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    for role in ("write", "startup", "owner", "result"):
                        check(runtime, root, work, compiler, include, mode, locked, role)
        if not positive:
            for role in ("write", "startup", "owner", "result"):
                check(runtime, root, work, compiler, include, "--no-jit", False, role, True)


if __name__ == "__main__":
    main()
