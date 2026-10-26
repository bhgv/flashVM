// Optional AS1/AS2 conversion client. Load this library explicitly; no native AS coercion is replaced.
// request is an adapter-owned function accepting (operation, reference, Limbo type, completion).
// It must copy validated descriptors, schedule work outside the UI owner, and complete exactly once.
// References stay opaque here: helpers never inspect object fields, call getters or stringify a target.
function makeProjectionValues(request) {
    var api = {};

    // Ask the owner for a byte value in 0..255; completion receives copied value or a typed error.
    api.tobyte = function(reference, completion) {
        return request("readAs", reference, "byte", completion);
    };

    // Ask for the selected target profile's int width; no AS Number precision is assumed for NC64.
    api.toint = function(reference, completion) {
        return request("readAs", reference, "int", completion);
    };

    // Ask for exact signed 64-bit big; an AS adapter must retain exact limbs/decimal text, not Number.
    api.tobig = function(reference, completion) {
        return request("readAs", reference, "big", completion);
    };

    // Ask for binary64 real. Converting big explicitly may round; this is not a binary32 operation.
    api.toreal = function(reference, completion) {
        return request("readAs", reference, "real", completion);
    };
    api.tofloat = api.toreal; // Convenience alias for the same Limbo real request and semantics.

    // Ask for a copied Unicode string; only the owner decides whether user conversion code may run.
    api.tostring = function(reference, completion) {
        return request("readAs", reference, "string", completion);
    };

    // Extension beyond native Limbo types: request normalized logical int 0/1.
    api.tobool = function(reference, completion) {
        return request("readAs", reference, "bool", completion);
    };

    // Extension beyond native Limbo types: request one Unicode scalar, never a UTF-16 half surrogate.
    api.tochar = function(reference, completion) {
        return request("readAs", reference, "char", completion);
    };

    // Explicit shallow data snapshot, without prototype/method cloning or implicit getter execution.
    api.totable = function(reference, completion) {
        return request("snapshot", reference, "table", completion);
    };
    return api;
}
