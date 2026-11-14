# Copyright (c) 2026 bhgv
# EN: Permanent owner/ref supervisors retire orphan alarms without weakening caller policy.
# RU: Permanent owner/ref supervisors завершают orphan alarms без ослабления caller policy.
from pathlib import Path
import sys
import tempfile

from network_proxy_fixture import compile_fixture
from proxy_retirement_cases import prepare, local, remote, control


# EN/RU: Borrow matched runtime/compiler/include; own fixtures through C/JIT/poll and omission.
# Borrow matching runtime/compiler/include; владеть fixtures до C/JIT/poll и omission.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    positive = sys.argv[4:] == ["--positive-only"]
    only_controls = sys.argv[4:] == ["--controls-only"]
    if sys.argv[4:] and not positive and not only_controls:
        raise RuntimeError("unsupported retirement flags")
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="proxy retirement ") as temporary:
        work = Path(temporary)
        compile_fixture(root, work, compiler, include)
        prepare(root, work, compiler, include)
        if not only_controls:
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    tag = mode[2:] + str(locked)
                    local(runtime, work, mode, locked, tag + "-owner", "owner-orphan-client")
                    local(runtime, work, mode, locked, tag + "-policy", "owner-policy-client")
                    remote(runtime, work, mode, locked, tag + "-ref")
                    print("PASS owner/ref caller retirement " + tag, flush=True)
        if not positive:
            control(runtime, root, work, compiler, include)


if __name__ == "__main__":
    main()
