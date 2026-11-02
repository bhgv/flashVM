"""Compile the real production queue and exercise semantic ownership controls."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile


# Use actual retained Prog declarations and real native ports. Private temporary
# units contain intentional controls only; repository sources remain untouched.
# No args/result; compiler/semantic/timeout errors raise instead of counting pass.
def main():
    root = Path(__file__).resolve().parents[2]
    runtime = root/"disVM/runtime"
    with tempfile.TemporaryDirectory(prefix="runqueue ownership ") as directory:
        work = Path(directory)
        command = shlex.split(os.environ.get("CC","cc"))
        command += shlex.split(os.environ.get("CFLAGS","-O1 -g"))
        command += ["-pthread"]
        for include in (runtime, root/"disVM/host/include", root/"disVM/upstream/include",
                        root/"disVM/upstream/libinterp", root/"disVM/upstream/emu/port",
                        root/"disVM/include", root/"flashVM/ports"):
            command += ["-I"+str(include)]
        command += [str(runtime/"test_runqueue.c"),str(runtime/"runqueue.c"),
                    str(root/"disVM/host/native_context.c"),
                    str(root/"flashVM/ports/linux/thread.c"),
                    str(root/"flashVM/ports/linux/channel_sync.c"),"-o",str(work/"queue")]
        subprocess.run(command,check=True)
        subprocess.run([command[-1]],check=True,timeout=30)
        source = (runtime/"runqueue.c").read_text()
        controls = [
            ("return !process || !process->native64holds;", "(void)process; return 1;",
             "synchronous Native64Caller hold was promoted to resumable"),
            ("runtime_process(process)->native_owner != context ||", "",
             "foreign continuation restoration accepted"),
            ("count == DVM_RUNTIME_PROCESS_MAX || runtime_process(process)->queued ||",
             "count == DVM_RUNTIME_PROCESS_MAX ||", "duplicate runnable admission accepted"),
            ("    if (dvm_runqueue_pop(Prelease) != process)\n    {\n        return 0;\n    }", "",
             "native continuation remained runnable"),
        ]
        for index, (before, after, marker) in enumerate(controls):
            if source.count(before) != 1:
                raise RuntimeError("queue control predicate not unique: "+marker)
            altered = work/("control"+str(index)+".c")
            altered.write_text(source.replace(before,after))
            altered_command = [str(altered) if item == str(runtime/"runqueue.c") else item for item in command]
            altered_command[-1] = str(work/("control"+str(index)))
            subprocess.run(altered_command,check=True)
            result = subprocess.run([altered_command[-1]],capture_output=True,text=True,timeout=30)
            if not result.returncode or marker not in result.stderr:
                raise RuntimeError("inactive queue control: "+marker+"\n"+result.stdout+result.stderr)
    print("PASS active queue controls: synchronous hold, foreign restore, duplicate admission, missing withdrawal")


if __name__ == "__main__":
    main()
