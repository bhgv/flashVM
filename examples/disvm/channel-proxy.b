implement Channelproxy; # Define an explicit local variable proxy.
include "sys.m"; # Import console output.

Channelproxy: module { # Declare the hosted entry point.
    init: fn(); # Export a zero-argument entry.
}; # End the interface.

Read, Write, Stop: con iota; # Define the three protocol operations.

Request: adt { # Define one typed protocol message.
    operation: int; # Select the action.
    value: int; # Carry the requested write value.
    reply: chan of int; # Carry the reply endpoint for this request.
}; # End the protocol record.

owner(inbox: chan of Request) # Keep the actual variable in one guest.
{ # Open the owner function.
    current := 10; # Initialize the private value.
    for (;;) { # Wait for requests until shutdown.
        request := <-inbox; # Receive the next typed message.
        case request.operation { # Dispatch the protocol operation.
            Read => request.reply <-= current; # Return the current value.
            Write => # Handle a write request.
                current = request.value; # Update the owner's local variable.
                request.reply <-= current; # Acknowledge the completed update.
            Stop => # Handle orderly shutdown.
                request.reply <-= current; # Acknowledge before ending the guest.
                return; # Leave the owner function.
        } # Close the operation switch.
    } # Close the receive loop.
} # Close the owner function.

call(inbox: chan of Request, operation: int, value: int): int # Invoke one explicit operation.
{ # Open the client helper.
    reply := chan[1] of int; # Allocate a per-request reply channel.
    inbox <-= Request(operation, value, reply); # Send the operation and reply endpoint.
    return <-reply; # Wait for the owner's acknowledgement.
} # Close the client helper.

init() # Verify local request/reply semantics.
{ # Open the entry function.
    sys := load Sys Sys->PATH; # Resolve the hosted console.
    endpoint := chan[1] of Request; # Create the proxy's request endpoint.
    spawn owner(endpoint); # Start the guest that owns the actual variable.
    if (call(endpoint, Read, 0) != 10) # Read the initial state.
        raise "initial value"; # Report an incorrect initial state.
    if (call(endpoint, Write, 41) != 41 || call(endpoint, Read, 0) != 41) # Write then read.
        raise "updated value"; # Report a failed round trip.
    if (call(endpoint, Stop, 0) != 41) # Wait for the final acknowledgement.
        raise "shutdown"; # Report an unexpected shutdown reply.
    sys->print("PASS DisVM channel proxy\n"); # Print the expected success marker.
} # Close the entry function.
