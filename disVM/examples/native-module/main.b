implement NativeModuleTutorial; # Compile a real bytecode client, not a host-side mock.
include "sys.m"; # Sys supplies ordinary checked output; no Draw/Tk module is needed.
include "nativedemo.m"; # This is the same contract used to generate the C frames and export table.
NativeModuleTutorial: module # Keep the launcher entry independent of a GUI context.
{ # Declare both routines exported in this module's own bytecode.
    init: fn(); # The experimental NC64 launcher supports a parameterless entry.
    service: fn(native: Nativedemo, requests: chan of (int, big, big, chan of big)); # A typed Limbo wrapper around synchronous C calls.
}; # Module data and process roots are owned by Dis, not C worker threads.


# Serve correlated private reply channels on one Limbo process; no raw UI/heap pointer escapes.
# native and requests are retained by this process frame; opcode 1 replies before shutdown.
service(native: Nativedemo, requests: chan of (int, big, big, chan of big)) # The wrapper may block on ordinary Dis channels.
{ # Native functions themselves remain short and synchronous on the VM owner.
    for (;;) { # Keep servicing until the explicit terminal operation arrives.
        (operation, left, right, reply) := <-requests; # Receive copied scalar inputs and a typed reply capability.
        if (operation == 1) { # Stop is an acknowledged protocol operation, not a detached native thread.
            reply <-= big 0; # Drain this acknowledgement before the caller ends its lifetime.
            return; # Release process-frame references after the terminal exchange.
        } # All other tutorial requests perform the scalar native operation.
        reply <-= native->add(left, right); # C computes the value; Limbo owns the blocking response and scheduling.
    } # No poll loop or OS-thread access to the Dis heap is required.
} # The channel wrapper does not bypass the existing DisVM-to-Flash channel route.


# Exercise real native loading, GC-visible string returns, errors and a channel-owned service.
# No parameters; failures propagate to the runner, successful output is independently checked.
init() # Every native module is loaded through its declared typed interface.
{ # Keep module references live while their exports are in use.
    sys := load Sys Sys->PATH; # Resolve the runtime's actual output module.
    native := load Nativedemo Nativedemo->PATH; # Resolve the optional builtin registered by C.
    if (sys == nil || native == nil) { # Missing exports are a real deployment error.
        raise "fail: native tutorial modules"; # Never substitute demonstration results.
    } # The loader has checked the interface signatures before calls become possible.
    maximum := big "9223372036854775807"; # Build the signed-big boundary without relying on host int width.
    minimum := big "-9223372036854775808"; # This is the independent expected wrapping result.
    if (native->add(maximum, big 1) != minimum) { # Test C's defined unsigned wrap through the real frame.
        raise "fail: native tutorial big wrap"; # Detect accidental signed overflow or a mismatched ABI.
    } # A normal scalar call may also be exposed through a typed Limbo service.
    requests := chan of (int, big, big, chan of big); # The wrapper owns request dispatch and lifetime.
    answers := chan of big; # Only this caller receives each operation's response.
    spawn service(native, requests); # Spawn a Limbo process, not an unsafe native VM thread.
    requests <-= (0, big 19, big 23, answers); # Send values; no frame pointer is a channel payload.
    if (<-answers != big 42) { # Observe the actual native result delivered through Dis channels.
        raise "fail: native tutorial channel sum"; # A wrong result fails the example process.
    } # The caller can keep using the module directly for short synchronous operations.
    text := ""; # This variable roots the most recent Dis-owned returned string.
    for (index := 0; index < 2048; index++) { # Exercise repeated return-slot replacement and collector scheduling.
        text = native->greeting(); # The C helper allocates; Dis owns the returned string and old-value release.
        if (text != "native module ready") { # Independently verify every result, not only the last call.
            raise "fail: native tutorial string"; # Detect an invalid return slot or ownership transfer.
        } # No C allocation or worker pointer is retained in this Limbo variable.
    } # At quantum 1 the scheduler/GC runs throughout this loop.
    caught := 0; # Record that the native exception reaches its intended Limbo handler.
    { # The following protected block deliberately invokes the failing native export.
        native->check(0); # A native error is not a successful void return.
    } exception message { # Recovery occurs in the caller's own Dis execution context.
        "native demo: rejected" => caught = 1; # Accept only the documented native diagnostic.
        * => raise message; # Preserve unexpected errors rather than hiding them.
    } # Subsequent native calls must still work after the error unwinds.
    if (!caught) { # Reaching here without recovery would violate the tutorial's error contract.
        raise "fail: native tutorial missing exception"; # Fail visibly on a swallowed native error.
    } # Successful void calls require no fabricated result value.
    native->check(1); # Verify the ordinary successful path after recovery.
    requests <-= (1, big 0, big 0, answers); # Ask the channel owner to stop explicitly.
    if (<-answers != big 0) { # Wait for its final reply before releasing the protocol endpoints.
        raise "fail: native tutorial shutdown"; # An unacknowledged stop is not accepted as cleanup.
    } # All tutorial service activity has now been retired through channels.
    sys->print("PASS native module: wrap=ok sum=42 strings=2048 caught=1 shutdown=ok\n"); # Print observed success only after every check.
} # The VM reclaims module/process/string roots under its normal lifecycle.
