# README.md / README.ru.md explain this source-built native-cell example and its limits.
implement TokenQueryDemo; # Implement the entry module declared below.
include "sys.m"; # Use the host's ordinary output capability, not a GUI or filesystem bridge.
include "idesyntax.m"; # Import immutable jobs, receiver-owned catalogs and generated-scanner workers.
include "idetokenquery.m"; # Import pure context, literal-search and lexical-candidate helpers.


# The matching DisVM host calls init without a Draw context or external emu.
TokenQueryDemo: module { # Declare only the portable demonstration entry point.
    init: fn(); # Run one complete worker/query/stop sequence.
}; # End the entry interface.


# Analyze two local snapshots and query the first without modifying it.
# There are no arguments or returned values. All started workers are acknowledged
# before a status failure is raised; no VM/font/native pointer crosses a channel.
init() # This example requires the matching shipped NC64 runtime and modules.
{ # Begin the owning demonstration process.
    sys := load Sys Sys->PATH; # Obtain output from this interpreter/JIT instance.
    syntax := load Idesyntax Idesyntax->PATH; # Resolve the portable analysis service.
    query := load Idetokenquery Idetokenquery->PATH; # Resolve independent catalog queries.
    if (sys == nil || syntax == nil || query == nil) { # Fail before starting a child if a dependency is absent.
        raise "fail: token-query example dependencies"; # The installed package must supply all three modules.
    } # Finish dependency admission.
    (jobs, answers) := syntax->start(); # Start a private generated-scanner worker, not a frame callback.
    if (jobs == nil || answers == nil) { # Missing Coco/R scanner modules produce no live capability.
        raise "fail: token-query example scanners"; # Do not invent a handwritten fallback lexer.
    } # Finish worker admission.
    source := "var alpha=1; var alpine=2; // hidden\nalpha++;"; # Keep the exact immutable source beside its catalog.
    jobs <-= syntax->Request(1, 1, syntax->ActionScript, source, 0); # Submit ticket1/revision1 to the actual worker.
    catalog := <-answers; # Receive exactly one complete result for this accepted job.
    good := catalog.status == syntax->Ok; # Remember status; cleanup still runs on rejection.
    if (good) { # Query only a successful, revision-bound catalog.
        (status, token) := query->at(catalog, 1, syntax->ActionScript, 4); # Scalar4 lies in the identifier alpha.
        good = status == query->Ok && token.kind == 1; # Require real identifier context, not guessed text colour.
        if (good) { # The source slice is meaningful only after context success.
            sys->print("Context: %s [%d,%d)\n", source[token.first:token.last], token.first, token.last); # Show scalar coordinates.
        } # Finish context output.
        (completion, names) := query->complete(catalog, 1, syntax->ActionScript, source, "al", 8); # Ask for at most8 lexical names.
        good = good && completion == query->Ok && len names == 2; # Duplicated alpha must appear only once.
        for (index := 0; index < len names; index++) { # Print the helper's stable sorted order.
            sys->print("Candidate: %s\n", names[index]); # These names have no inferred type or scope guarantee.
        } # Finish candidate output.
        (found, first, last) := query->find(catalog, 1, syntax->ActionScript, source, "alpha", 5, query->Names); # Skip the first occurrence.
        good = good && found == query->Ok; # Comments/strings and other lexical classes are excluded by this mask.
        if (found == query->Ok) { # Negative result coordinates must never be sliced.
            sys->print("Next name: %s [%d,%d)\n", source[first:last], first, last); # Show the later actual source match.
        } # Finish search output.
        changed := source; # Limbo string value semantics retain the original snapshot.
        changed[10] = '2'; # Edit the numeric literal in the local second snapshot only.
        jobs <-= syntax->Request(2, 2, syntax->ActionScript, changed, 0); # Submit a distinct correlation/revision.
        updated := <-answers; # This worker may reuse safe prefix and converged suffix records.
        good = good && updated.status == syntax->Ok && source[10] == '1'; # Analysis and local edits cannot alter the first snapshot.
        if (updated.status == syntax->Ok) { # Counters describe returned records, not wall-clock latency.
            sys->print("Update: scanned=%d reused=%d restart=%d\n", updated.scanned, updated.reused, updated.restart); # Report actual reuse.
        } # Finish incremental diagnostics.
    } # Finish the successful query path.
    jobs <-= syntax->Request(3, 2, syntax->ActionScript, "", 1); # Always stop the started worker, including after status failures.
    stopped := <-answers; # Shutdown is acknowledged before the parent can finish.
    if (!good || stopped.status != syntax->Ok) { # Preserve explicit failures after cleanup.
        sys->print("FAIL token-query example\n"); # Hosts that return zero on root exceptions still show a failure marker.
        raise "fail: token-query example status"; # Never present partial or stale data as a successful query.
    } # Finish checked cleanup.
    sys->print("PASS token-query example: actual Coco/R catalog, context/search/names and incremental reuse\n"); # Expected successful completion.
} # End the owning example; no child remains waiting for another request.
