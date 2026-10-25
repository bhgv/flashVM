implement DisConsoleCheck; # Portable Dis console program; no graphical module is imported.
include "sys.m";          # Use the host's actual Sys interface for diagnostic output.


# A minimal entry module for an initialized Dis host that can load $Sys.
DisConsoleCheck: module
{ # Begin the deliberately small executable interface.
    init: fn(); # Run the test, print its result and return after both channel messages.
}; # End the interface without a Draw or window-system dependency.


# Send two integers through a real blocking Dis channel.
# destination is borrowed until both rendezvous complete. Returns nothing.
producer(destination: chan of int)
{ # Each send blocks until the receiving Dis process accepts its value.
    destination <-= 21; # Transfer the first half of the expected result.
    destination <-= 21; # Transfer the second half before allowing this producer to exit.
} # The producer retains no channel work after the second acknowledgement.


# Load actual Sys, spawn a producer, consume both values and print their sum.
# No parameters. Returns after the program's processes complete; the embedding
# host decides whether its last-program return terminates the native application.
init()
{ # This is actual Limbo code, compiled into a Dis module by the source compiler.
    sys := load Sys Sys->PATH; # Resolve the genuine host builtin, never a successful empty stub.
    if (sys == nil) { # Missing Sys means the host is not ready to run this program.
        raise "fail: console-check requires an initialized Sys module"; # Report the real integration failure.
    } # Continue only with a valid module handle.
    values := chan of int; # Allocate a native Dis rendezvous channel.
    spawn producer(values); # Start a separate Limbo process using the Dis scheduler.
    first := <-values; # Receive and acknowledge the first blocking send.
    second := <-values; # Receive and acknowledge the final blocking send.
    sys->print("PASS Dis channels: %d\n", first + second); # The expected numeric result is 42.
} # Every program-owned channel exchange has completed before return.
