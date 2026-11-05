implement Workers; # Define a hosted DisVM module.
include "sys.m"; # Import console output.
include "processctl.m"; # Import hosted guest lifecycle operations.

Workers: module { # Declare the hosted entry.
    init: fn(); # DisVM calls this function once.
}; # End the module interface.

worker(jobs, results: chan of int) # Accept typed channels from the parent.
{ # Open the worker body.
    job := <-jobs; # Block until the parent sends a job.
    results <-= job * job; # Send the computed value back.
} # Close the worker body.

init() # Run a request/reply with a child guest.
{ # Open the entry function.
    sys := load Sys Sys->PATH; # Resolve the hosted console.
    control := load Processctl Processctl->PATH; # Resolve the hosted process-control module.
    jobs := chan of int; # Create an unbuffered request channel.
    results := chan[1] of int; # Create a reply channel with one buffered slot.
    spawn worker(jobs, results); # Schedule a child guest.
    jobs <-= 7; # Send the request and rendezvous with the worker.
    alt { # Wait for a ready channel guard.
        value := <-results => # Receive the result from the child.
            if (value != 49 || control->pid() <= 0) # Validate the result and current guest identity.
                raise "worker result"; # Report an unexpected result.
    } # Close the ALT statement.
    sys->print("PASS DisVM workers\n"); # Print the success marker.
} # Close the entry function.
