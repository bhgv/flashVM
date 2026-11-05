implement Tables; # Define the runnable table example.
include "sys.m"; # Import console output.
include "bigtable.m"; # Import the hosted typed table module.

Tables: module { # Declare the hosted entry.
    init: fn(); # DisVM starts here.
}; # End the module interface.

init() # Exercise keys, statuses and copied snapshots.
{ # Open the entry function.
    sys := load Sys Sys->PATH; # Resolve the hosted console.
    api := load Bigtable Bigtable->PATH; # Load the local table implementation.
    table := api->create(); # Allocate a managed handle.
    api->requireok(api->setindex(table, big 0, big 12)); # Insert index zero in the dense prefix.
    api->requireok(api->settext(table, "score", big 0)); # Store a real zero under a text key.
    (status, value) := api->gettext(table, "score"); # Read presence and value separately.
    (missing, ignored) := api->gettext(table, "absent"); # Read a key that was never written.
    if (status != Bigtable->Ok || value != big 0 || missing != Bigtable->Missing) # Distinguish zero from absence.
        raise "table statuses"; # Report an unexpected status.
    keys := api->keysof(table); # Capture an independent immutable snapshot of the keys.
    copy := api->slice(table, big 0, big 1); # Copy the half-open dense prefix.
    api->requireok(api->deltext(table, "score")); # Remove the key from the original table.
    (copied, number) := api->getindex(copy, big 0); # Read the independently copied value.
    if (api->keylen(keys) != big 2 || api->count(table) != big 1 || copied != Bigtable->Ok || number != big 12) # Check snapshot isolation.
        raise "table snapshot"; # Report an unexpected snapshot.
    sys->print("PASS DisVM Bigtable\n"); # Print the success marker.
} # Close the entry function.
