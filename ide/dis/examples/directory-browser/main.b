implement DirectoryBrowser; # Name the executable Limbo module.
include "sys.m"; # Use the matching SDK module declaration.
include "idefiles.m"; # Use the matching typed module interface.
include "idebrowser.m"; # Use the same exclusive browser model as the graphical controller. # Use the matching SDK module declaration.

DirectoryBrowser: module # Begin this local scope/interface.
{ # Begin this local scope/interface.
    init: fn(); # Matched native DisVM entry; no external Inferno emulator is used.
}; # Finish this scope and its temporary values.

sys: Sys; # Borrow native services only through their module interface.
files: Idefiles; # Actual channel-owned implementation shared with the IDE.
commands: chan of Idefiles->DirectoryRequest; # Endpoint scopes all cursor identities.
nextid := 1; # Correlate each synchronous exchange independently of cursor IDs.
root: con FIXTURE_ROOT; # The driver substitutes a quoted, private Unicode fixture path.


# Fail on an evaluated oracle mismatch, including optimized execution.
# condition/message are copied values; raise is visible to the host driver.
check(condition: int, message: string) # Require this stated oracle; fail visibly on mismatch.
{ # Begin this local scope/interface.
    if (!condition) { # Choose behavior from the actual validated result.
        raise "fail: directory browser " + message; # Complete this checked declaration, expression or ownership step.
    } # Finish this scope and its temporary values.
} # Finish this scope and its temporary values.


# Borrow operation/cursor/path until the blocking reply arrives.
# Return independent VM values and verify exact request correlation.
exchange(operation: int, cursor: big, path: string): Idefiles->DirectoryReply # Consume one correlated response before the next request.
{ # Begin this local scope/interface.
    answers := chan of Idefiles->DirectoryReply; # Keep this value local; no native handle is exported.
    identity := nextid; # Keep this value local; no native handle is exported.
    nextid++; # Advance the bounded count or copied presentation value.
    commands <-= Idefiles->DirectoryRequest(identity, operation, cursor, path, answers); # Transfer a typed request/reply through the blocking channel.
    response := <-answers; # Keep this value local; no native handle is exported.
    check(response.id == identity, "correlation"); # Require this stated oracle; fail visibly on mismatch.
    return response; # Return the independently owned result or finish this helper.
} # Finish this scope and its temporary values.


# Exercise metadata, stream interleaving, quota and shutdown on real native files.
# The driver owns the files. This program owns only service endpoints and values;
# every opened cursor is closed or retired by acknowledged terminal Shutdown.
init() # Complete this checked declaration, expression or ownership step.
{ # Begin this local scope/interface.
    sys = load Sys Sys->PATH; # Complete this checked declaration, expression or ownership step.
    files = load Idefiles Idefiles->PATH; # Complete this checked declaration, expression or ownership step.
    check(sys != nil && files != nil, "module load"); # Require this stated oracle; fail visibly on mismatch.
    absent: Sys; # Complete this checked declaration, expression or ownership step.
    check(files->startdirs(absent) == nil, "nil Sys admission"); # Require this stated oracle; fail visibly on mismatch.
    commands = files->startdirs(sys); # Complete this checked declaration, expression or ownership step.
    check(commands != nil, "owner start"); # Require this stated oracle; fail visibly on mismatch.
    missing := exchange(files->NextDir, big 1, ""); # Consume one correlated response before the next request.
    check(missing.status == files->Stale && missing.entries == nil, "unknown cursor"); # Require this stated oracle; fail visibly on mismatch.
    info := exchange(files->StatPath, big 0, root+"/данные😀.bin"); # Consume one correlated response before the next request.
    check(info.status == files->Ok && info.metadata.length == big 4, "copied file metadata"); # Require this stated oracle; fail visibly on mismatch.
    large := exchange(files->StatPath, big 0, root+"/large.bin"); # Consume one correlated response before the next request.
    check(large.status == files->Ok && large.metadata.length == big 4294967303, "big file length"); # Require this stated oracle; fail visibly on mismatch.
    ordinary := exchange(files->OpenDir, big 0, root+"/данные😀.bin"); # Consume one correlated response before the next request.
    check(ordinary.status == files->Failed && ordinary.cursor == big 0, "ordinary file is not a cursor"); # Require this stated oracle; fail visibly on mismatch.
    nul := "bad bad"; # Keep this value local; no native handle is exported.
    nul[3] = 0; # Complete this checked declaration, expression or ownership step.
    invalid := exchange(files->OpenDir, big 0, nul); # Consume one correlated response before the next request.
    check(invalid.status == files->Invalid, "NUL path admission"); # Require this stated oracle; fail visibly on mismatch.
    invalid = exchange(files->OpenDir, big 0, "content://documents/item"); # Consume one correlated response before the next request.
    check(invalid.status == files->Failed, "provider URI rejection"); # Require this stated oracle; fail visibly on mismatch.
    longpath := ""; # Keep this value local; no native handle is exported.
    for (pathindex := 0; pathindex <= files->MaxPathBytes; pathindex++) { # Repeat only within this explicit test or output bound.
        longpath += "x"; # Advance the bounded count or copied presentation value.
    } # Finish this scope and its temporary values.
    invalid = exchange(files->OpenDir, big 0, longpath); # Consume one correlated response before the next request.
    check(invalid.status == files->Invalid, "bounded path admission"); # Require this stated oracle; fail visibly on mismatch.
    invalid = exchange(files->StatPath, big 1, root); # Consume one correlated response before the next request.
    check(invalid.status == files->Invalid, "path operation does not accept a cursor"); # Require this stated oracle; fail visibly on mismatch.
    empty := exchange(files->OpenDir, big 0, root+"/подкаталог"); # Consume one correlated response before the next request.
    emptyend := exchange(files->NextDir, empty.cursor, ""); # Consume one correlated response before the next request.
    emptyclose := exchange(files->CloseDir, empty.cursor, ""); # Consume one correlated response before the next request.
    check(empty.status == files->Ok && emptyend.status == files->End && # Require this stated oracle; fail visibly on mismatch.
          emptyclose.status == files->Ok, "empty native directory"); # Complete this checked declaration, expression or ownership step.
    broken := exchange(files->OpenDir, big 0, root+"/../invalid-names"); # Consume one correlated response before the next request.
    brokenread := exchange(files->NextDir, broken.cursor, ""); # Consume one correlated response before the next request.
    brokenagain := exchange(files->NextDir, broken.cursor, ""); # Consume one correlated response before the next request.
    check(broken.status == files->Ok && brokenread.status == files->Failed && # Require this stated oracle; fail visibly on mismatch.
          brokenread.entries == nil && brokenagain.status == files->Stale, # Complete this checked declaration, expression or ownership step.
          "failed native batch retires cursor without publishing entries"); # Complete this checked declaration, expression or ownership step.
    unknown := exchange(77, big 0, root); # Consume one correlated response before the next request.
    check(unknown.status == files->Invalid && unknown.entries == nil, "unknown operation"); # Require this stated oracle; fail visibly on mismatch.
    commands <-= Idefiles->DirectoryRequest(1, files->Shutdown, big 0, "", nil); # Transfer a typed request/reply through the blocking channel.
    info = exchange(files->StatPath, big 0, root); # Consume one correlated response before the next request.
    check(info.status == files->Ok, "nil reply cannot shut down owner"); # Require this stated oracle; fail visibly on mismatch.
    answers := chan of Idefiles->DirectoryReply; # Keep this value local; no native handle is exported.
    commands <-= Idefiles->DirectoryRequest(-1, files->Shutdown, big 0, "", answers); # Transfer a typed request/reply through the blocking channel.
    negative := <-answers; # Keep this value local; no native handle is exported.
    check(negative.id == -1 && negative.status == files->Invalid, "invalid terminal identity"); # Require this stated oracle; fail visibly on mismatch.

    first := exchange(files->OpenDir, big 0, root); # Consume one correlated response before the next request.
    second := exchange(files->OpenDir, big 0, root); # Consume one correlated response before the next request.
    check(first.status == files->Ok && second.status == files->Ok && # Require this stated oracle; fail visibly on mismatch.
          first.cursor != second.cursor && (first.metadata.mode & Sys->DMDIR) != 0, # Complete this checked declaration, expression or ownership step.
          "independent cursor opens"); # Complete this checked declaration, expression or ownership step.
    batch := exchange(files->NextDir, first.cursor, ""); # Consume one correlated response before the next request.
    check(batch.status == files->Ok && len batch.entries == files->MaxBatch, "first bounded batch"); # Require this stated oracle; fail visibly on mismatch.
    total := len batch.entries; # Keep this value local; no native handle is exported.
    batch.entries[0].name = "client-owned mutation"; # Complete this checked declaration, expression or ownership step.
    other := exchange(files->NextDir, second.cursor, ""); # Consume one correlated response before the next request.
    check(other.status == files->Ok && len other.entries == files->MaxBatch && # Require this stated oracle; fail visibly on mismatch.
          other.entries[0].name != "client-owned mutation", "independent transferred arrays"); # Complete this checked declaration, expression or ownership step.
    retained := other.entries; # Keep this value local; no native handle is exported.
    closed := exchange(files->CloseDir, second.cursor, ""); # Consume one correlated response before the next request.
    check(closed.status == files->Ok && len retained == files->MaxBatch, "values survive close"); # Require this stated oracle; fail visibly on mismatch.
    batches := 1; # Keep this value local; no native handle is exported.
    for (;;) { # Repeat only within this explicit test or output bound.
        part := exchange(files->NextDir, first.cursor, ""); # Consume one correlated response before the next request.
        if (part.status == files->End) { # Choose behavior from the actual validated result.
            check(part.entries == nil, "empty EOF result"); # Require this stated oracle; fail visibly on mismatch.
            break; # Stop only at the validated terminal condition.
        } # Finish this scope and its temporary values.
        check(part.status == files->Ok && len part.entries > 0 && # Require this stated oracle; fail visibly on mismatch.
              len part.entries <= files->MaxBatch, "bounded complete stream"); # Complete this checked declaration, expression or ownership step.
        total += len part.entries; # Advance the bounded count or copied presentation value.
        batches++; # Advance the bounded count or copied presentation value.
    } # Finish this scope and its temporary values.
    check(total == 74 && batches == 3, "all entries across three batches"); # Require this stated oracle; fail visibly on mismatch.
    ended := exchange(files->NextDir, first.cursor, ""); # Consume one correlated response before the next request.
    check(ended.status == files->End, "stable EOF without native reread"); # Require this stated oracle; fail visibly on mismatch.
    closed = exchange(files->CloseDir, first.cursor, ""); # Consume one correlated response before the next request.
    stale := exchange(files->NextDir, first.cursor, ""); # Consume one correlated response before the next request.
    check(closed.status == files->Ok && stale.status == files->Stale, "retired cursor"); # Require this stated oracle; fail visibly on mismatch.
    stale = exchange(files->CloseDir, first.cursor, ""); # Consume one correlated response before the next request.
    check(stale.status == files->Stale, "duplicate close"); # Require this stated oracle; fail visibly on mismatch.

    identities := array[files->MaxDirectories] of big; # Keep this value local; no native handle is exported.
    for (index := 0; index < len identities; index++) { # Repeat only within this explicit test or output bound.
        opened := exchange(files->OpenDir, big 0, root); # Consume one correlated response before the next request.
        check(opened.status == files->Ok, "fill quota"); # Require this stated oracle; fail visibly on mismatch.
        identities[index] = opened.cursor; # Complete this checked declaration, expression or ownership step.
    } # Finish this scope and its temporary values.
    overflow := exchange(files->OpenDir, big 0, root); # Consume one correlated response before the next request.
    check(overflow.status == files->Full, "owner quota"); # Require this stated oracle; fail visibly on mismatch.
    info = exchange(files->StatPath, big 0, root); # Consume one correlated response before the next request.
    check(info.status == files->Ok, "stat does not consume a cursor"); # Require this stated oracle; fail visibly on mismatch.
    old := identities[0]; # Keep this value local; no native handle is exported.
    closed = exchange(files->CloseDir, old, ""); # Consume one correlated response before the next request.
    replacement := exchange(files->OpenDir, big 0, root); # Consume one correlated response before the next request.
    check(closed.status == files->Ok && replacement.status == files->Ok && # Require this stated oracle; fail visibly on mismatch.
          replacement.cursor > identities[len identities-1], "identity never reused"); # Complete this checked declaration, expression or ownership step.
    stale = exchange(files->NextDir, old, ""); # Consume one correlated response before the next request.
    check(stale.status == files->Stale, "stale identity cannot access reused slot"); # Require this stated oracle; fail visibly on mismatch.
    identities[0] = replacement.cursor; # Complete this checked declaration, expression or ownership step.
    for (index = 0; index < len identities; index++) { # Repeat only within this explicit test or output bound.
        closed = exchange(files->CloseDir, identities[index], ""); # Consume one correlated response before the next request.
        check(closed.status == files->Ok, "release full quota"); # Require this stated oracle; fail visibly on mismatch.
    } # Finish this scope and its temporary values.
    for (index = 0; index < 80; index++) { # Repeat only within this explicit test or output bound.
        opened := exchange(files->OpenDir, big 0, root); # Consume one correlated response before the next request.
        check(opened.status == files->Ok, "repeat open"); # Require this stated oracle; fail visibly on mismatch.
        closed = exchange(files->CloseDir, opened.cursor, ""); # Consume one correlated response before the next request.
        check(closed.status == files->Ok, "repeat close"); # Require this stated oracle; fail visibly on mismatch.
    } # Finish this scope and its temporary values.
    # More than the native global FD limit must open across terminal owner cycles.
    for (round := 0; round < 32; round++) { # Repeat only within this explicit test or output bound.
        for (index = 0; index < files->MaxDirectories; index++) { # Repeat only within this explicit test or output bound.
            opened := exchange(files->OpenDir, big 0, root); # Consume one correlated response before the next request.
            check(opened.status == files->Ok, "terminal lifecycle opens round=" + string round + " slot=" + string index + # Require this stated oracle; fail visibly on mismatch.
                  " status=" + string opened.status + " " + opened.message); # Complete this checked declaration, expression or ownership step.
        } # Finish this scope and its temporary values.
        stopped := exchange(files->Shutdown, big 0, ""); # Consume one correlated response before the next request.
        check(stopped.status == files->Ok, "shutdown closes all cursors before acknowledgement"); # Require this stated oracle; fail visibly on mismatch.
        commands = files->startdirs(sys); # Complete this checked declaration, expression or ownership step.
    } # Finish this scope and its temporary values.
    stopped := exchange(files->Shutdown, big 0, ""); # Consume one correlated response before the next request.
    check(stopped.status == files->Ok, "final shutdown"); # Require this stated oracle; fail visibly on mismatch.
    logic := load Idebrowser Idebrowser->PATH; # Load the actual shared widget policy.
    check(logic != nil, "browser model module"); # Require this lifecycle or ownership invariant.
    model := logic->create(files, sys); # Keep this state exclusive to the test controller.
    check(model != nil, "browser model owner"); # Require this lifecycle or ownership invariant.
    check(logic->open(files, model, root) == files->Ok, "model initial batch"); # Require this lifecycle or ownership invariant.
    preserved := model.cursor; # Capture identity before candidate admission can fail.
    check(logic->open(files, model, root+"/missing") == files->Failed && # Require this lifecycle or ownership invariant.
          model.cursor == preserved && model.path == root, "model failed candidate preserves cursor"); # Complete this checked controller or copied-value step.
    check(logic->open(files, model, root+"/../invalid-names") == files->Failed && # Require this lifecycle or ownership invariant.
          model.cursor == preserved, "first batch failure preserves old cursor"); # Complete this checked controller or copied-value step.
    check(logic->move(model, 2) == files->Invalid, "model rejects invalid selection step"); # Require this lifecycle or ownership invariant.
    caption := logic->summary(sys, model); # Only this bounded copied label reaches the renderer.
    check(len array of byte caption <= 1024, "renderer label byte bound"); # Require this lifecycle or ownership invariant.
    for (entryindex := 0; entryindex < len model.entries; entryindex++) { # Complete this checked controller or copied-value step.
        if ((model.entries[entryindex].mode & Sys->DMDIR) == 0) { # Admit only the current validated state or event.
            model.selected = entryindex; # Exclusive controller state may select a known local record.
            break; # Complete this checked controller or copied-value step.
        } # Keep this controller scope explicit.
    } # Keep this controller scope explicit.
    (kind, target) := logic->choose(files, model); # File choice returns a path, never document I/O.
    check(kind == logic->PickFile && target == root+"/"+model.entries[model.selected].name, # Require this lifecycle or ownership invariant.
          "model file choice preserves full path"); # Complete this checked controller or copied-value step.
    model.sequence = logic->MaxRequest-1; # Leave too little room for candidate admission/cleanup.
    check(logic->open(files, model, root) == files->Invalid && model.cursor == preserved, # Require this lifecycle or ownership invariant.
          "near-exhaustion cannot strand a candidate cursor"); # Complete this checked controller or copied-value step.
    model.sequence = logic->MaxRequest; # Force exhaustion without billions of synthetic requests.
    check(logic->open(files, model, root) == files->Invalid, "reserved terminal identity"); # Require this lifecycle or ownership invariant.
    check(logic->shutdown(files, model) == files->Ok && model.endpoint == nil && # Require this lifecycle or ownership invariant.
          model.cursor == big 0, "exhausted model can still shut down"); # Complete this checked controller or copied-value step.
    check(logic->shutdown(files, model) == files->Ok, "model terminal acknowledgement is idempotent"); # Require this lifecycle or ownership invariant.
    for (modelround := 0; modelround < 32; modelround++) { # Complete this checked controller or copied-value step.
        model = logic->create(files, sys); # Each endpoint has an independent identity namespace.
        check(logic->open(files, model, root) == files->Ok, "repeated model open"); # Require this lifecycle or ownership invariant.
        check(logic->shutdown(files, model) == files->Ok, "repeated model shutdown closes FDs"); # Require this lifecycle or ownership invariant.
    } # Keep this controller scope explicit.
    sys->print("PASS directory browser: channels, batches, identities, quota and shutdown\n"); # Present bounded plain text through actual Sys output.
} # Finish this scope and its temporary values.
