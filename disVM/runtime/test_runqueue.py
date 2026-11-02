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
        command += ["-pthread", "-DDVM_RUNTIME_PROCESS_MAX=128"]
        for include in (runtime, root/"disVM/host", root/"disVM/host/include", root/"disVM/upstream/include",
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
            ("runtime_process(process)->native_owner != context ||\n        runtime_process(process)->dispatch_owner != context ||", "",
             "foreign continuation restoration accepted"),
            ("runtime_process(process)->dispatch_owner || dvm_native_context_process(context) ||",
             "dvm_native_context_process(context) ||", "foreign managed dispatch claim accepted"),
            ("if (!context || !process || runtime_process(process)->dispatch_owner != context ||",
             "if (!context || !process ||", "foreign managed dispatch finish accepted"),
            ("count == DVM_RUNTIME_PROCESS_MAX || runtime_process(process)->queued ||",
             "count == DVM_RUNTIME_PROCESS_MAX ||", "duplicate runnable admission accepted"),
            ("    if (dvm_runqueue_pop(Prelease) != process)\n    {\n        return 0;\n    }", "",
             "native continuation remained runnable"),
        ]
        context_path = root/"disVM/host/native_context.c"
        context_source = context_path.read_text()
        controls = [(runtime/"runqueue.c", source, *entry) for entry in controls]
        controls += [
            (context_path, context_source, "|| !token || context->turn)", "|| !token)",
             "cleared GC root permitted nested dispatch"),
            (context_path, context_source, "context->heap_owner.world || context->turn)",
             "context->heap_owner.world)", "claimed context unbound before finish"),
        ]
        for index, (unit, original, before, after, marker) in enumerate(controls):
            if original.count(before) != 1:
                raise RuntimeError("ownership control predicate not unique: "+marker)
            altered = work/("control"+str(index)+".c")
            altered.write_text(original.replace(before,after))
            altered_command = [str(altered) if item == str(unit) else item for item in command]
            if marker == "foreign managed dispatch finish accepted":
                # Exact ownership has TWO independent checks: the shared Prog
                # claim and its context-local native token. Remove both in the
                # private control; either intact layer must refuse the attack.
                needle = "|| !token || context->turn != token)"
                if context_source.count(needle) != 1:
                    raise RuntimeError("context release control target changed")
                weak_context = work/"weak_release_context.c"
                weak_context.write_text(context_source.replace(needle,"|| !token)"))
                altered_command = [str(weak_context) if item == str(context_path) else item
                                   for item in altered_command]
            altered_command[-1] = str(work/("control"+str(index)))
            subprocess.run(altered_command,check=True)
            result = subprocess.run([altered_command[-1]],capture_output=True,text=True,timeout=30)
            if result.returncode != 1 or marker not in result.stderr:
                raise RuntimeError("inactive ownership control: "+marker+"\n"+result.stdout+result.stderr)
    print("PASS eight active ownership controls: synchronous hold, foreign restore/claim/finish, "
          "duplicate admission, missing withdrawal, nested cleared-root turn, early unbind")



if __name__ == "__main__":
    main()
