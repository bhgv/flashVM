implement IdetokenqueryTest;
include "sys.m";
include "idesyntax.m";
include "idetokenquery.m";
sys: Sys; # Host-only output and failure shutdown.
syntax: Idesyntax; # Actual generated scanner worker.
query: Idetokenquery; # Pure catalog consumers under test.
checks: int; # Always-active predicate count, including in Release.


# Exercise portable lexical context/search/candidate APIs on actual Coco/R results.
IdetokenqueryTest: module {
    init: fn();
};


# Make failure visible even when the host returns zero after a root exception.
require(ok: int, message: string)
{
    if (!ok) {
        sys->print("FAIL token queries: %s\n", message);
        fd := sys->open("/dev/sysctl", Sys->OWRITE);
        if (fd != nil) {
            data := array of byte "halt";
            sys->write(fd, data, len data);
        }
        raise "fail: token queries";
    }
    checks++;
}


# Independent small literal oracle for all-class fixtures, using direct scalar substrings.
# This intentionally has no KMP table or lexical filtering implementation.
literal(source, needle: string, first: int): int
{
    for (index := first; index + len needle <= len source; index++) {
        if (source[index:index + len needle] == needle) {
            return index;
        }
    }
    return -1;
}


# Run named token positions, masks, Unicode, overlapping literals and bounded candidates.
# Catalog/source are never modified by the query module; no source selection is edited.
init()
{
    sys = load Sys Sys->PATH;
    syntax = load Idesyntax Idesyntax->PATH;
    query = load Idetokenquery Idetokenquery->PATH;
    require(syntax != nil && query != nil, "modules loaded");
    (jobs, answers) := syntax->start();
    require(jobs != nil, "generated scanners loaded");
    source := "var alpha=1; var alpine=2; var beta=\"hidden\"; // hidden😀\nalpha++; πName;";
    jobs <-= syntax->Request(1, 7, syntax->ActionScript, source, 0);
    catalog := <-answers;
    require(catalog.status == syntax->Ok, "actual catalog available");
    (status, token) := query->at(catalog, 7, syntax->ActionScript, 0);
    require(status == query->Ok && token.kind == 4 && token.first == 0 && token.last == 3,
            "keyword context");
    (status, token) = query->at(catalog, 7, syntax->ActionScript, 4);
    require(status == query->Ok && token.kind == 1 && token.first == 4 && token.last == 9,
            "identifier context");
    position := literal(source, "😀", 0);
    (status, token) = query->at(catalog, 7, syntax->ActionScript, position);
    require(status == query->Ok && token.kind == 5, "supplementary scalar inside trivia context");
    (status, token) = query->at(catalog, 7, syntax->ActionScript, len source);
    require(status == query->NotFound && token.kind == -1, "EOF is not an invented token");
    (status, token) = query->at(catalog, 8, syntax->ActionScript, 0);
    require(status == query->Conflict, "stale revision");
    (status, token) = query->at(catalog, 7, syntax->Limbo, 0);
    require(status == query->Conflict, "stale profile");
    (status, token) = query->at(catalog, 7, syntax->ActionScript, -1);
    require(status == query->Invalid, "negative source position");
    first, last: int;
    (status, first, last) = query->find(catalog, 7, syntax->ActionScript, source, "alpha", 0, query->Names);
    require(status == query->Ok && first == 4 && last == 9, "identifier-only literal search");
    (status, first, last) = query->find(catalog, 7, syntax->ActionScript, source, "alpha", 5, query->Names);
    require(status == query->Ok && first == literal(source, "alpha", 5), "search starts after current match");
    (status, first, last) = query->find(catalog, 7, syntax->ActionScript, source, "hidden", 0, query->Names);
    require(status == query->NotFound && first == -1 && last == -1, "names exclude strings/comments");
    (status, first, last) = query->find(catalog, 7, syntax->ActionScript, source, "hidden", 0, 1 << 3);
    require(status == query->Ok && first == literal(source, "hidden", 0), "literal-only mask");
    (status, first, last) = query->find(catalog, 7, syntax->ActionScript, source, "a=1; var", 0, query->AllKinds);
    require(status == query->Ok && first == literal(source, "a=1; var", 0), "match across admitted categories");
    (status, first, last) = query->find(catalog, 7, syntax->ActionScript, source, "a=1; var", 0, query->Names);
    require(status == query->NotFound, "disallowed category resets partial match");
    (status, first, last) = query->find(catalog, 7, syntax->ActionScript, source, "", 0, query->AllKinds);
    require(status == query->Invalid, "empty needle rejected");
    (status, first, last) = query->find(catalog, 7, syntax->ActionScript, source, "x", 0, 64);
    require(status == query->Invalid, "unknown lexical mask bit");
    (status, first, last) = query->find(catalog, 7, syntax->ActionScript, source[1:], "x", 0, query->AllKinds);
    require(status == query->Invalid, "wrong source extent rejected");
    names: array of string;
    (status, names) = query->complete(catalog, 7, syntax->ActionScript, source, "al", 8);
    require(status == query->Ok && len names == 2 && names[0] == "alpha" && names[1] == "alpine",
            "sorted unique lexical candidates");
    sys->print("Lexical candidates (not typed completion): %s, %s\n", names[0], names[1]);
    names[0] = "changed by receiver";
    (status, names) = query->complete(catalog, 7, syntax->ActionScript, source, "al", 1);
    require(status == query->Ok && len names == 1 && names[0] == "alpha", "bounded detached candidates");
    (status, names) = query->complete(catalog, 7, syntax->ActionScript, source, "h", 8);
    require(status == query->Ok && len names == 0, "comments/strings are not candidate names");
    (status, names) = query->complete(catalog, 7, syntax->ActionScript, source, "π", 8);
    require(status == query->Ok && len names == 1 && names[0] == "πName", "Unicode prefix");
    (status, names) = query->complete(catalog, 7, syntax->ActionScript, source, "", 0);
    require(status == query->Invalid, "zero candidate limit");
    (status, names) = query->complete(catalog, 8, syntax->ActionScript, source, "al", 8);
    require(status == query->Conflict, "stale completion rejected");
    catalog.tokens[0] = syntax->Token(99, 0, 3);
    (status, token) = query->at(catalog, 7, syntax->ActionScript, 0);
    require(status == query->Invalid, "forged token class rejected");
    jobs <-= syntax->Request(2, 8, syntax->ActionScript, source, 0);
    catalog = <-answers;
    (status, token) = query->at(catalog, 8, syntax->ActionScript, 0);
    require(status == query->Ok && token.kind == 4 && catalog.scanned == 0,
            "receiver mutation cannot corrupt private incremental baseline");
    source = "ababababx";
    jobs <-= syntax->Request(3, 9, syntax->ActionScript, source, 0);
    catalog = <-answers;
    (status, first, last) = query->find(catalog, 9, syntax->ActionScript, source, "ababx", 0, query->AllKinds);
    require(status == query->Ok && first == 4 && last == 9, "overlapping literal KMP fallback");
    jobs <-= syntax->Request(4, 10, syntax->ActionScript, "", 0);
    catalog = <-answers;
    (status, token) = query->at(catalog, 10, syntax->ActionScript, 0);
    require(status == query->NotFound, "empty catalog context");
    (status, names) = query->complete(catalog, 10, syntax->ActionScript, "", "", 8);
    require(status == query->Ok && len names == 0, "empty source candidates");
    jobs <-= syntax->Request(5, 10, syntax->ActionScript, "", 1);
    catalog = <-answers;
    require(catalog.status == syntax->Ok, "worker shutdown");
    sys->print("PASS Coco/R token queries: %d predicates; context, masks, Unicode search and lexical candidates\n", checks);
}
