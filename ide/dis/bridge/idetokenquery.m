# Pure bounded queries over a completed Coco/R catalog and its exact source snapshot.
Idetokenquery: module
{
    PATH: con "idetokenquery.dis"; # Portable Limbo library; no renderer or filesystem.
    Ok, Invalid, Conflict, NotFound: con iota; # Success, malformed input, stale/unready, no match.
    AllKinds: con 63; # Bit k admits lexical class k (0 punctuation through 5 trivia).
    Names: con 2; # Only identifier records; excludes strings/comments/keywords.
    MaxNeedle: con 256; # Unicode scalar limit for one literal search.
    MaxPrefix: con 64; # Scalar bound for a completion prefix.
    MaxName: con 128; # Larger lexical names are not offered as bounded candidates.
    MaxCandidates: con 32; # Maximum sorted unique candidate count.


    # Return the half-open token covering a scalar; EOF returns NotFound and sentinel kind=-1.
    # All helpers validate the complete catalog once per call, including revision/profile.
    # Caller must serialize mutation; helpers do not retain or edit source/catalog arrays.
    at: fn(catalog: Idesyntax->Result, revision, language, position: int): (int, Idesyntax->Token);


    # Find one case-sensitive literal at/after first, without wrapping or editing selection.
    # source MUST be the exact analyzed snapshot; length/scalars are checked, not authenticated.
    # Every matched scalar must belong to an admitted kind; disallowed records reset KMP state.
    # Class5 combines whitespace/comments, so it cannot select comments independently of whitespace.
    # Returns (status, first, last); non-Ok positions are -1. No scanner or callback runs here.
    find: fn(catalog: Idesyntax->Result, revision, language: int,
             source, needle: string, first, kinds: int): (int, int, int);


    # Return <=limit sorted, unique lexical names beginning with prefix, case-sensitive.
    # source is the exact snapshot. limit=1..32; empty prefix allowed; overlong names skipped.
    # These are lexical candidates, NOT scope/type/declaration-aware completion. Receiver owns array.
    complete: fn(catalog: Idesyntax->Result, revision, language: int,
                 source, prefix: string, limit: int): (int, array of string);
};
