# Copyright (c) 2026 bhgv
# EN: Native proxy lifetime, terminal publication and independent source cleanup acceptance.
# RU: Проверки native proxy lifetime, terminal publication и independent source cleanup.
from pathlib import Path
import sys
import tempfile

from network_proxy_fixture import compile_fixture
from proxy_retirement_cases import prepare, local, remote, control
import disconnect_waiter_cases as disconnect
import terminal_publication_cases as publication
import parallel_cleanup_cases as cleanup
import ref_terminal_cases as ref_publication


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
        disconnect.prepare(root, work, compiler, include)
        publication.prepare(root, work, compiler, include)
        cleanup.prepare(root, work, compiler, include)
        ref_publication.prepare(root, work, compiler, include)
        if not only_controls:
            publication.check(runtime, root, work, compiler, include)
            ref_publication.check(runtime, root, work, compiler, include)
            for mode in ("--no-jit", "--jit"):
                for locked in (False, True):
                    tag = mode[2:] + str(locked)
                    cleanup.run(runtime, work, mode, locked, tag + "-parallel-cleanup")
                    local(runtime, work, mode, locked, tag + "-owner", "owner-orphan-client")
                    local(runtime, work, mode, locked, tag + "-policy", "owner-policy-client")
                    remote(runtime, work, mode, locked, tag + "-ref")
                    for kind in ("net", "ref"):
                        disconnect.run(
                            runtime, work, mode, locked, tag + "-disconnect-" + kind, kind
                        )
                        disconnect.run(
                            runtime,
                            work,
                            mode,
                            locked,
                            tag + "-disconnect-orphan-" + kind,
                            kind,
                            orphan=True,
                        )
                    print("PASS owner/ref caller retirement and disconnect " + tag, flush=True)
        if not positive:
            control(runtime, root, work, compiler, include)
            disconnect.control(runtime, root, work, compiler, include)
            publication.check(runtime, root, work, compiler, include, True)
            cleanup.control(runtime, root, work, compiler, include)
            ref_publication.check(runtime, root, work, compiler, include, True)


if __name__ == "__main__":
    main()
