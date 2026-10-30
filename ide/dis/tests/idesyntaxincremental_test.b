implement IdesyntaxIncrementalTest;
include "sys.m";
include "idesyntax.m";
sys: Sys; # Test host diagnostics and bounded failure shutdown.
syntax: Idesyntax; # Actual worker/scanner service; no mirror lexer.
checks: int; # Always-active successful token comparisons.


# Run actual generated scanners and both private/incremental ownership paths.
IdesyntaxIncrementalTest: module {
    init: fn();
};


# Stop on an explicit predicate failure, including runtimes returning zero on exception.
require(ok: int, message: string)
{
    if (!ok) {
        sys->print("FAIL incremental syntax: %s\n", message);
        fd := sys->open("/dev/sysctl", Sys->OWRITE);
        if (fd != nil) {
            data := array of byte "halt";
            sys->write(fd, data, len data);
        }
        raise "fail: incremental syntax";
    }
    checks++;
}


# Acknowledge shutdown of exactly this worker; no pending result is abandoned.
stop(jobs: chan of syntax->Request, answers: chan of syntax->Result)
{
    jobs <-= syntax->Request(1, 0, syntax->Limbo, "", 1);
    result := <-answers;
    require(result.status == syntax->Ok, "worker stopped");
}


# Compare incremental output with a separate fresh worker running the complete scanner.
# job/source are immutable values. Return the incremental receiver-owned catalog.
compare(jobs: chan of syntax->Request, answers: chan of syntax->Result,
        job: syntax->Request): syntax->Result
{
    jobs <-= job;
    result := <-answers;
    (fulljobs, fullanswers) := syntax->start();
    require(fulljobs != nil, "fresh oracle loaded");
    fulljobs <-= job;
    full := <-fullanswers;
    require(result.status == syntax->Ok && full.status == syntax->Ok, "complete catalogs");
    require(result.ticket == job.ticket && result.revision == job.revision &&
            result.language == job.language, "fresh correlation");
    if (len result.tokens != len full.tokens) {
        sys->print("case ticket=%d source=[%s] inc=%d full=%d restart=%d scanned=%d reused=%d\n",
            job.ticket, job.text, len result.tokens, len full.tokens,
            result.restart, result.scanned, result.reused);
        for (debug := 0; debug < len result.tokens; debug++) {
            t := result.tokens[debug];
            sys->print("inc %d %d %d\n", t.kind, t.first, t.last);
        }
        for (debug = 0; debug < len full.tokens; debug++) {
            t := full.tokens[debug];
            sys->print("full %d %d %d\n", t.kind, t.first, t.last);
        }
    }
    require(len result.tokens == len full.tokens, "full token count");
    require(result.scanned + result.reused == len result.tokens &&
            full.reused == 0 && full.scanned == len full.tokens, "record accounting");
    for (i := 0; i < len full.tokens; i++) {
        a := result.tokens[i];
        b := full.tokens[i];
        require(a.kind == b.kind && a.first == b.first && a.last == b.last,
                "exact generated token class and scalar interval");
    }
    stop(fulljobs, fullanswers);
    return result;
}


# Cover merges/splits, comment delimiters, escaped/incomplete literals and Unicode deltas.
# Every step changes the same worker's baseline; profile switches must force full scanning.
init()
{
    sys = load Sys Sys->PATH;
    syntax = load Idesyntax Idesyntax->PATH;
    require(syntax != nil, "service loaded");
    (jobs, answers) := syntax->start();
    require(jobs != nil, "incremental worker loaded");
    texts := array[] of {
        "", "var a=1; var b=2; var c=3;", "var a=1; var b=22; var c=3;",
        "var a=1; var b=22; var c=3;", "var a=1;var b=22; var c=3;",
        "vara=1;var b=22; var c=3;", "var a=1;var b=22; var c=3;",
        "var π😀=1;var b=22; var c=3;", "var π😀=1; /* hi */ return 42;",
        "var π😀=1; /* hi / return 42;", "var π😀=1; /* hi */ return 42;",
        "var π😀=1; // hi\nreturn 42;", "var π😀=1; / hi\nreturn 42;",
        "var π😀=1; // hi\nreturn 42;", "var π😀=1; // hi return 42;",
        "var π😀=1; // hi\nreturn 42;", "var x=\"a\\\"/*b\"; return 42;",
        "var x=\"a\\\"/*b; return 42;", "var x=\"a\\\"/*b\"; return 42;",
        "var x='\\''; return 42;", "var x='\\'; return 42;", "var x=1.25; return 42;",
        "var x=1.2; return 42;", "var x=1.25; return 42;", "", "return", "returnx", "return"
    };
    for (i := 0; i < len texts; i++) {
        result := compare(jobs, answers, syntax->Request(i+1, i, syntax->ActionScript, texts[i], 0));
        if (i == 2) {
            require(result.reused > 0 && result.scanned <= 3 && result.restart > 0,
                    "middle edit reuses prefix and converged suffix");
        }
        if (i == 3) {
            require(result.scanned == 0 && result.reused == len result.tokens,
                    "unchanged source has no generated rescan");
        }
        # Returned arrays may be edited by the receiver, never the retained baseline.
        if (len result.tokens != 0) {
            result.tokens[0] = syntax->Token(99, -1, -1);
        }
    }
    limbo := array[] of {
        "fn x() { # old\n return 42; }", "fn x() { # old return 42; }",
        "fn x() { # old\n return 42; }", "fn x() { \"# old\"; return 42; }",
        "fn x() { \"# old; return 42; }", "fn x() { \"# old\"; return 42; }"
    };
    for (i = 0; i < len limbo; i++) {
        result := compare(jobs, answers, syntax->Request(100+i, 100+i, syntax->Limbo, limbo[i], 0));
        if (i == 0) {
            require(result.reused == 0, "profile switch discards reuse");
        }
    }
    text := "";
    for (i = 0; i < 96; i++) {
        text += "var x=1;\n";
    }
    compare(jobs, answers, syntax->Request(200, 200, syntax->ActionScript, text, 0));
    changed := text;
    changed[47*9+6] = '2';
    result := compare(jobs, answers, syntax->Request(201, 201, syntax->ActionScript, changed, 0));
    require(result.scanned <= 3 && result.reused > 600 && result.restart > 400,
            "large unchanged prefix/tail bypass generated scanning");
    jobs <-= syntax->Request(202, -1, syntax->ActionScript, changed, 0);
    result = <-answers;
    require(result.status == syntax->Invalid && len result.tokens == 0, "invalid job is atomic");
    oversized := array[syntax->MaxCharacters+1] of { * => byte 'x' };
    jobs <-= syntax->Request(203, 203, syntax->ActionScript, string oversized, 0);
    result = <-answers;
    require(result.status == syntax->TooLarge && len result.tokens == 0, "quota job is atomic");
    jobs <-= syntax->Request(204, 204, syntax->ActionScript, changed, 0);
    result = <-answers;
    require(result.status == syntax->Ok && result.scanned == 0 && result.reused > 600,
            "rejected jobs preserve the private successful baseline");
    stop(jobs, answers);
    sys->print("PASS incremental Coco/R syntax: %d predicates; exact fresh-worker oracle, private arrays, Unicode and safe convergence\n", checks);
}
