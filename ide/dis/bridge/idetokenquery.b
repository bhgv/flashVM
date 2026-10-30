implement Idetokenquery;
include "idesyntax.m";
include "idetokenquery.m";


# Validate revision/profile and contiguous scalar intervals before querying external values.
# Return (status, inferred source length). Bounded by MaxTokens; no input is retained.
validate(catalog: Idesyntax->Result, revision, language: int): (int, int)
{
    if (revision < 0 || (language != Idesyntax->Limbo && language != Idesyntax->ActionScript)) {
        return (Invalid, 0);
    }
    if (catalog.status != Idesyntax->Ok || catalog.revision != revision || catalog.language != language) {
        return (Conflict, 0);
    }
    count := len catalog.tokens;
    if (count > Idesyntax->MaxTokens || catalog.scanned < 0 || catalog.scanned > count ||
        catalog.reused != count - catalog.scanned) {
        return (Invalid, 0);
    }
    last := 0;
    for (index := 0; index < count; index++) {
        token := catalog.tokens[index];
        if (token.kind < 0 || token.kind > 5 || token.first != last || token.last <= last ||
            token.last > Idesyntax->MaxCharacters) {
            return (Invalid, 0);
        }
        last = token.last;
    }
    if (catalog.restart < 0 || catalog.restart > last) {
        return (Invalid, 0);
    }
    return (Ok, last);
}


# Validate counted Unicode scalar values; NUL is ordinary data, not a terminator.
# text is borrowed and already length-bounded by the public helper. No allocations.
scalars(text: string): int
{
    for (index := 0; index < len text; index++) {
        value := text[index];
        if (value < 0 || value > 16r10ffff || (value >= 16rd800 && value <= 16rdfff)) {
            return 0;
        }
    }
    return 1;
}


# Locate the first interval ending after position; catalog has been validated.
# Returns len tokens at EOF. Only copied interval values are inspected.
lower(catalog: Idesyntax->Result, position: int): int
{
    low := 0;
    high := len catalog.tokens;
    while (low < high) {
        middle := low + (high - low) / 2;
        if (catalog.tokens[middle].last <= position) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}


# Observe a scalar token, preserving all document/view/caret state; see the public contract.
at(catalog: Idesyntax->Result, revision, language, position: int): (int, Idesyntax->Token)
{
    absent := Idesyntax->Token(-1, 0, 0);
    (status, length) := validate(catalog, revision, language);
    if (status != Ok) {
        return (status, absent);
    }
    if (position < 0 || position > length) {
        return (Invalid, absent);
    }
    if (position == length) {
        return (NotFound, absent);
    }
    return (Ok, catalog.tokens[lower(catalog, position)]);
}


# Search scalar data with linear KMP fallback and lexical-mask barriers, without re-lexing.
# The caller supplies the exact source revision. No wrapping, UI or native handle access.
find(catalog: Idesyntax->Result, revision, language: int,
     source, needle: string, first, kinds: int): (int, int, int)
{
    (status, length) := validate(catalog, revision, language);
    if (status != Ok) {
        return (status, -1, -1);
    }
    if (len source != length || !scalars(source) || len needle < 1 || len needle > MaxNeedle ||
        !scalars(needle) || first < 0 || first > length || kinds < 1 || kinds > AllKinds) {
        return (Invalid, -1, -1);
    }
    fallback := array[len needle] of { * => 0 };
    matched := 0;
    for (index := 1; index < len needle; index++) {
        while (matched > 0 && needle[index] != needle[matched]) {
            matched = fallback[matched-1];
        }
        if (needle[index] == needle[matched]) {
            matched++;
        }
        fallback[index] = matched;
    }
    matched = 0;
    token := lower(catalog, first);
    for (index = first; index < length; index++) {
        while (index >= catalog.tokens[token].last) {
            token++;
        }
        if ((kinds & (1 << catalog.tokens[token].kind)) == 0) {
            matched = 0;
            continue;
        }
        while (matched > 0 && source[index] != needle[matched]) {
            matched = fallback[matched-1];
        }
        if (source[index] == needle[matched]) {
            matched++;
        }
        if (matched == len needle) {
            return (Ok, index + 1 - matched, index + 1);
        }
    }
    return (NotFound, -1, -1);
}


# Collect bounded lexical identifiers without claiming scopes, types or symbol resolution.
# Insertion keeps at most limit sorted unique owned strings; original source stays unchanged.
complete(catalog: Idesyntax->Result, revision, language: int,
         source, prefix: string, limit: int): (int, array of string)
{
    (status, length) := validate(catalog, revision, language);
    if (status != Ok) {
        return (status, nil);
    }
    if (len source != length || !scalars(source) || len prefix > MaxPrefix || !scalars(prefix) ||
        limit < 1 || limit > MaxCandidates) {
        return (Invalid, nil);
    }
    candidates := array[limit] of string;
    count := 0;
    for (index := 0; index < len catalog.tokens; index++) {
        token := catalog.tokens[index];
        if (token.kind != 1 || token.last - token.first > MaxName ||
            token.last - token.first < len prefix) {
            continue;
        }
        name := source[token.first:token.last];
        if (name[0:len prefix] != prefix) {
            continue;
        }
        place := 0;
        while (place < count && candidates[place] < name) {
            place++;
        }
        if ((place < count && candidates[place] == name) || place == limit) {
            continue;
        }
        if (count < limit) {
            count++;
        }
        for (move := count - 1; move > place; move--) {
            candidates[move] = candidates[move-1];
        }
        candidates[place] = name;
    }
    result: array of string;
    if (count > 0) {
        result = array[count] of string;
        result[0:] = candidates[0:count];
    }
    return (Ok, result);
}
