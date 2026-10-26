"""Verify source-view preparation is idempotent and never compounds dispatch changes."""

from pathlib import Path
import subprocess
import sys
import tempfile


# Prepare twice using the real compiler and pinned source; require byte-identical output.
# Arguments are native compiler, pinned libinterp source and actual compiled-source directory.
# Returns only after idempotency and current-view freshness both match.
def main():
    compiler, source, compiled_view = sys.argv[1:]
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        command = [
            sys.executable,
            str(Path(__file__).with_name("prepare.py")),
            "--compiler",
            compiler,
            "--source",
            source,
            "--sys",
            str(Path(source).parent / "module/sys.m"),
            "--output",
            temporary,
        ]
        subprocess.run(command, check=True)
        first = {file.name: file.read_bytes() for file in root.iterdir() if file.is_file()}
        subprocess.run(command, check=True)
        second = {file.name: file.read_bytes() for file in root.iterdir() if file.is_file()}
        if first != second:
            raise RuntimeError("native source generation compounded a prior adaptation")
        # Inspect actual build inputs, not only two equally fresh throwaway generations.
        actual = Path(compiled_view)
        critical = ("string.c", "xec.c", "comp-leaf64.h", "leaf64-ops.h",
                    "amd64-emit.h", "arm64-emit.h", "riscv-emit.h")
        stale = [name for name in critical if not (actual / name).is_file()
                 or (actual / name).read_bytes() != second[name]]
        if stale:
            raise RuntimeError("compiled native view is stale relative to pinned source: " + ", ".join(stale))
        dispatch = second["xec.c"]
        if (dispatch.count(b"dvm_runtime_step();") != 1 or
                dispatch.count(b"dvm_runtime_instructions++;") != 1 or
                dispatch.count(b"if(dvm_runtime_instruction_limit)") != 1):
            raise RuntimeError("instruction quota/counter must share one dispatch boundary")
        poll = dispatch.index(b"if(!dvm_native_context_poll()) abort();")
        guard = dispatch.index(b"if(dvm_runtime_instruction_limit)")
        step = dispatch.index(b"dvm_runtime_step();")
        inline = dispatch.index(b"dvm_runtime_instructions++;")
        translated = dispatch.index(b"if(DVM_R.M->m->cstep != nil")
        decode = dispatch.index(b"dec[DVM_R.PC->add]();", translated)
        if not poll < guard < step < inline < translated < decode:
            raise RuntimeError("quota must follow the GC safepoint and precede both execution paths")
    print("PASS byte-identical native source regeneration and one quota/count boundary")


if __name__ == "__main__":
    main()
