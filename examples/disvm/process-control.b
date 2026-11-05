implement Processcontrol; # Define the guest lifecycle example.
include "sys.m"; # Import console output.
include "processctl.m"; # Import the DisVM process-control extension.

Processcontrol: module { # Declare the runnable interface.
    init: fn(); # Expose the entry point.
}; # End the interface.

child(ids, gate: chan of int) # Send a PID, then wait on a private gate.
{ # Open the child body.
    control := load Processctl Processctl->PATH; # Load guest lifecycle operations.
    ids <-= control->pid(); # Publish the child's positive PID.
    <-gate; # Block until a sender arrives or the guest is terminated.
    raise "child escaped"; # Detect an unexpected return from the gate.
} # Close the child body.

init() # Terminate and reap a direct child.
{ # Open the entry function.
    sys := load Sys Sys->PATH; # Resolve the hosted console.
    control := load Processctl Processctl->PATH; # Resolve the hosted lifecycle module.
    ids := chan of int; # Create the PID rendezvous.
    gate := chan of int; # Create a gate that nobody will send to.
    spawn child(ids, gate); # Start the child guest.
    pid := <-ids; # Receive its PID.
    if (pid <= 0 || control->killwait(pid) != Processctl->Retired) # Await retirement.
        raise "child did not retire"; # Report the failed operation.
    if (control->killwait(pid) != Processctl->Denied) # Check that completion is consumed once.
        raise "retirement replayed"; # Report an invalid repeated completion.
    sys->print("PASS DisVM process control\n"); # Print the expected success marker.
} # Close the entry function.
