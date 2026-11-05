implement Tableworkers; # Define the shared-table example.
include "sys.m"; # Import console output.
include "bigtable.m"; # Import table operations and leases.

Tableworkers: module { # Declare the hosted entry point.
    init: fn(); # Export the zero-argument entry.
}; # End the interface.

worker(api: Bigtable, table: ref Bigtable->Handle, done: chan of int) # Borrow a shared handle.
{ # Open the worker function.
    api->acquire(table); # Wait for exclusive access to the table.
    (status, current) := api->getindex(table, big 0); # Read within the lease.
    api->requireok(status); # Require the seeded key.
    api->requireok(api->setindex(table, big 0, current + big 1)); # Publish the increment.
    api->release(table); # Release the lease before sending.
    done <-= 1; # Signal completion to the parent.
} # Close the worker function.

init() # Run two guests against the same entry.
{ # Open the entry function.
    sys := load Sys Sys->PATH; # Resolve console output.
    api := load Bigtable Bigtable->PATH; # Resolve the hosted table module.
    table := api->create(); # Create a shared managed handle.
    done := chan[2] of int; # Reserve two independent completion slots.
    api->requireok(api->setindex(table, big 0, big 0)); # Seed the counter.
    spawn worker(api, table, done); # Start the first increment.
    spawn worker(api, table, done); # Start the second increment.
    first := <-done; # Await one completed update.
    second := <-done; # Await the other update.
    (status, result) := api->getindex(table, big 0); # Read the final value.
    if (first + second != 2 || status != Bigtable->Ok || result != big 2) # Check both increments.
        raise "lost table update"; # Report a broken read-modify-write sequence.
    sys->print("PASS DisVM table workers\n"); # Print the expected success marker.
} # Close the entry function.
