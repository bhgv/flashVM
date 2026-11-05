"""Observe actual runnable withdrawal from an extra registered native OS owner."""
from pathlib import Path
import re
import subprocess
import sys
from test_native_wait import execute


# Validate actual normal/error execution, not elapsed time. runtime/module are
# matching built products; command flags select real collector/poll/dispatch.
# Native queue and observer diagnostics must match the controller's lease count.
def verify(runtime, module, mode, flags, quantum, limited=False):
    output = execute(runtime,module,mode,flags,quantum,limited)
    queue = re.search(r"NATIVEQUEUE queued=\d+ leases=(\d+) returns=(\d+) inflight=0",output)
    probe = re.search(r"QUEUEPROBE observed=(\d+) joined=(\d+) failures=0 idle=(\d+) scalar_spans=checked",output)
    gc = re.search(r"native_releases=(\d+) native_reentries=(\d+)",output)
    dispatch = re.search(r"NATIVEDISPATCH claims=(\d+) finishes=(\d+) active=0",output)
    if not dispatch or not int(dispatch[1]) or dispatch[1] != dispatch[2]:
        raise RuntimeError("managed dispatch ownership did not retire exactly once:\n"+output[-5000:])
    if (not queue or not probe or not gc or not int(queue[1]) or
            len({queue[1],queue[2],probe[1],probe[2],gc[1],gc[2]}) != 1):
        raise RuntimeError("native queue/root/OS-owner handoff failed:\n"+output[-5000:])
    if not int(probe[3]) or (not limited and int(probe[3]) >= int(probe[1])):
        raise RuntimeError("both ACTIVE and IDLE service detach paths were not exercised")
    if not limited and int(queue[1]) != 7:
        raise RuntimeError("the six sleep and final print continuations were not all observed")


# Each probe thread attaches to the production service, checks the real Prog
# while the controller is inactive, then fully retires before reentry. Negative
# option admission must fail before any managed execution. No user files change.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    rejected = subprocess.run([str(runtime),"--native-queue-probe",str(module)],
                              capture_output=True,text=True,timeout=15)
    if rejected.returncode != 2 or "requires --gc-worker" not in rejected.stderr:
        raise RuntimeError("queue probe incorrectly admitted without native heap ownership")
    for mode in ("--jit", "--no-jit"):
        for locked in (False, True):
            flags = ("--gc-worker", "--native-queue-probe")
            if locked:
                flags += ("--gc-locked-poll",)
            for quantum in (1, 2000):
                verify(runtime,module,mode,flags,quantum)
        verify(runtime,module,mode,("--gc-worker", "--native-queue-probe"),1,limited=True)
    print("PASS production queue probe: 8 normal and 2 managed-error runs; real extra OS owners, rooted frozen continuations, same-owner return")


if __name__ == "__main__":
    main()
