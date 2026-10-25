implement Idesearch;
include "idecore.m";
include "idesearch.m";


# Perform one synchronous borrowed channel operation; returns the acknowledged owner value.
# All scalar/range/text values are copied, with no native pointer or global mutable state.
call(core: Idecore, document: chan of Idecore->Request, operation, revision, first, last: int,
     text, replacement: string): Idecore->Reply
{
    response := chan of Idecore->Reply;
    document <-= Idecore->Request(1, operation, revision, first, last, text, replacement, response);
    return <-response;
}


# Query the acknowledged source and find from position, wrapping once only after NotFound.
# Public ownership/error semantics follow idesearch.m; every search still checks revision at the owner.
next(core: Idecore, document: chan of Idecore->Request,
     revision, position: int, needle: string): Idecore->Reply
{
    snapshot := call(core, document, core->Snapshot, 0, 0, 0, "", "");
    result := call(core, document, core->Find, revision, position, len snapshot.text, needle, "");
    if (result.status == core->NotFound && position > 0) {
        return call(core, document, core->Find, revision, 0, len snapshot.text, needle, "");
    }
    return result;
}


# Forward a single whole-document ReplaceAll to the existing bounded history owner.
# The snapshot supplies a range only; expected revision remains the caller's value.
replace(core: Idecore, document: chan of Idecore->Request,
        revision: int, needle, replacement: string): Idecore->Reply
{
    snapshot := call(core, document, core->Snapshot, 0, 0, 0, "", "");
    return call(core, document, core->ReplaceAll, revision, 0, len snapshot.text, needle, replacement);
}
