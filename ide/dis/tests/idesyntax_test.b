implement IdesyntaxTest;
include "sys.m";
include "idesyntax.m";
sys: Sys; # Test-only output and process termination capability.
syntax: Idesyntax; # Value-only service under test.


# Native test entry: no GUI, renderer, frame timer or mock scanner.
IdesyntaxTest: module {
    init: fn();
};


# Report explicit failure and stop the test host; runtime exceptions alone may exit zero.
# ok is the predicate; message identifies the actual contract being checked.
require(ok: int, message: string)
{
    if (!ok) {
        sys->print("FAIL %s\n", message);
        control := sys->open("/dev/sysctl", Sys->OWRITE);
        data := array of byte "halt";
        if (control != nil) {
            sys->write(control, data, len data);
        }
        raise "fail: syntax test";
    }
}


# Perform one lazy-owner operation; caller retains its reply capability until acknowledgement.
# Returns immediately on Update/Snapshot relative to unfinished scanner work.
call(owner: chan of syntax->Command, operation: int, job: syntax->Request): syntax->Result
{
    reply := chan of syntax->Result;
    owner <-= syntax->Command(operation, job, reply);
    return <-reply;
}


# Submit an edit burst while an expensive old scan is pending; only the newest may publish.
# No output subscription is needed: Snapshot remains authoritative even with nil notices.
lazychecks()
{
    owner := syntax->startlazy(nil);
    require(owner != nil, "lazy owner dependencies");
    filler := array[512] of { * => byte '+' };
    job := syntax->Request(1, 1, syntax->Limbo, string filler, 0);
    result := call(owner, syntax->Update, job);
    require(result.status == syntax->Pending, "update acknowledges before expensive scan");
    for (i := 2; i <= 20; i++) {
        job = syntax->Request(i, i, syntax->ActionScript, "var 最新😀 = 42;", 0);
        result = call(owner, syntax->Update, job);
        require(result.status == syntax->Pending && result.ticket == i,
                "edit burst receives immediate revision-bound acknowledgement");
    }
    result = call(owner, syntax->Update, job);
    require(result.status == syntax->Invalid, "duplicate correlation rejected");
    for (i = 0; i < 3000; i++) {
        result = call(owner, syntax->Snapshot, job);
        require(result.ticket == 20 && result.revision == 20 &&
                result.language == syntax->ActionScript, "old scan never replaces newest snapshot");
        if (result.status != syntax->Pending) {
            break;
        }
    }
    require(result.status == syntax->Ok, "latest coalesced result completes");
    palette := syntax->project(result, 20, syntax->ActionScript, 0, len job.text);
    require(palette[0:3] == "444" && palette[4:7] == "111", "latest Unicode catalog projected");
    job = syntax->Request(21, 21, syntax->Limbo, string filler, 0);
    call(owner, syntax->Update, job);
    result = call(owner, syntax->Shutdown, job);
    require(result.status == syntax->Ok, "shutdown drains active work without nil-channel alt");
}


# Execute real generated scanner jobs and check source/correlation/staleness/Unicode bounds.
# Each accepted request has exactly one receive, including acknowledged shutdown.
init()
{
    sys = load Sys Sys->PATH;
    syntax = load Idesyntax Idesyntax->PATH;
    require(syntax != nil, "load service");
    (jobs, answers) := syntax->start();
    require(jobs != nil && answers != nil, "generated dependencies loaded");
    source := "fn α😀() { # π😀\n x := \"#\"; }";
    jobs <-= syntax->Request(17, 8, syntax->Limbo, source, 0);
    result := <-answers;
    require(result.status == syntax->Ok && result.ticket == 17 && result.revision == 8,
            "complete correlated Limbo result");
    palette := syntax->project(result, 8, syntax->Limbo, 0, len source);
    require(len palette == len source && palette[0:2] == "44", "keyword scalar palette");
    require(palette[3:5] == "11", "supplementary identifier scalar boundaries");
    require(palette[10:14] == "5555", "generated skipped comment span");
    require(syntax->project(result, 9, syntax->Limbo, 0, 2) == "", "stale revision rejected");
    require(syntax->project(result, 8, syntax->ActionScript, 0, 2) == "", "stale profile rejected");
    require(syntax->project(result, 8, syntax->Limbo, -1, 2) == "", "negative viewport rejected");
    require(syntax->project(result, 8, syntax->Limbo, 0, 201) == "", "bounded viewport");
    saved := palette;
    source = "var π😀 = \"a\\\"//b\"; /* comment😀 */ return 42;";
    jobs <-= syntax->Request(18, 9, syntax->ActionScript, source, 0);
    second := <-answers;
    require(second.status == syntax->Ok, "AS literal escapes and comments");
    palette = syntax->project(second, 9, syntax->ActionScript, 0, len source);
    require(palette[0:3] == "444" && palette[4:6] == "11", "AS keyword and Unicode");
    require(syntax->project(result, 8, syntax->Limbo, 0, len saved) == saved,
            "earlier catalog unchanged by next scanner reset");
    jobs <-= syntax->Request(19, 10, syntax->ActionScript, "/* unfinished😀", 0);
    second = <-answers;
    require(second.status == syntax->Ok && len second.tokens == 1 &&
            second.tokens[0].kind == 5 && second.tokens[0].last == 14,
            "unfinished comment remains complete tolerant trivia");
    jobs <-= syntax->Request(20, 11, syntax->Limbo, "\"unfinished\\", 0);
    second = <-answers;
    require(second.status == syntax->Ok && second.tokens[0].kind == 3,
            "unfinished escape does not block diagnostics");
    jobs <-= syntax->Request(21, 12, syntax->Limbo, "", 0);
    second = <-answers;
    require(second.status == syntax->Ok && len second.tokens == 0, "empty source");
    jobs <-= syntax->Request(22, -1, syntax->Limbo, "x", 0);
    second = <-answers;
    require(second.status == syntax->Invalid && len second.tokens == 0, "invalid revision");
    oversized := array[syntax->MaxCharacters+1] of { * => byte 'x' };
    jobs <-= syntax->Request(23, 13, syntax->Limbo, string oversized, 0);
    second = <-answers;
    require(second.status == syntax->TooLarge && len second.tokens == 0, "snapshot quota");
    repeated := array[syntax->MaxTokens*2+2] of { * => byte '+' };
    jobs <-= syntax->Request(24, 14, syntax->Limbo, string repeated, 0);
    second = <-answers;
    require(second.status == syntax->TooLarge && len second.tokens == 0, "token quota is atomic");
    jobs <-= syntax->Request(25, 15, syntax->Limbo, "", 1);
    second = <-answers;
    require(second.status == syntax->Ok && second.ticket == 25, "shutdown acknowledged");
    lazychecks();
    sys->print("PASS Coco/R syntax: worker channels, scalar spans, immutable catalogs, stale projection, incomplete text and quotas\n");
}
