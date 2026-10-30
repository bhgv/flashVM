implement Idesyntax;
include "idesyntax.m";
include "limbolex.m";
include "aslex.m";


# Resolve independent scanner module instances before spawning. No handwritten fallback.
# Returned channels carry copied values; each scanner's mutable tables have one owner.
start(): (chan of Request, chan of Result)
{
    (jobs, answers, unused) := startcancellable();
    return (jobs, answers);
}


# Start an ordinary worker plus one buffered cancellation capability. A positive
# ticket cancels only that job at a cooperative checkpoint; callers own sends.
# Missing generated dependencies return three nil channels. No frame/OS timer.
startcancellable(): (chan of Request, chan of Result, chan of int)
{
    limbo := load LimbolexParser LimbolexParser->PATH;
    actionscript := load AslexParser AslexParser->PATH;
    if (limbo == nil || actionscript == nil) {
        return (nil, nil, nil);
    }
    jobs := chan of Request;
    answers := chan of Result;
    cancel := chan[1] of int;
    spawn serve(jobs, answers, cancel, limbo, actionscript);
    return (jobs, answers, cancel);
}


# Detach the receiver's mutable array from worker/lazy-owner retained catalogs.
# Value fields are copied; nil/error catalogs allocate no token array. No callbacks.
copyresult(result: Result): Result
{
    tokens: array of Token;
    if (len result.tokens != 0) {
        tokens = array[len result.tokens] of Token;
        tokens[0:] = result.tokens;
    }
    return Result(result.ticket, result.revision, result.language, result.status,
                  tokens, result.scanned, result.reused, result.restart);
}


# Resolve dependencies before returning a lazy command capability; nil means unavailable.
# One owning Dis process serializes client updates and immutable completed results.
startlazy(notices: chan of int): chan of Command
{
    (jobs, answers, cancel) := startcancellable();
    if (jobs == nil) {
        return nil;
    }
    commands := chan of Command;
    spawn lazyowner(commands, notices, jobs, answers, cancel);
    return commands;
}


# Keep only the latest pending snapshot while one worker owns an active immutable job.
# Update acknowledgements precede scanning. Stale ticket/revision/profile results are dropped.
# [GC:] The owner retains at most active/latest snapshots and one completed catalog.
lazyowner(commands: chan of Command, notices: chan of int,
          jobs: chan of Request, answers: chan of Result, cancel: chan of int)
{
    latest := Request(0, 0, Limbo, "", 0);
    cached := Result(0, 0, Limbo, Pending, nil, 0, 0, 0);
    working := 0;
    active := 0;
    dirty := 0;
    for (;;) {
        if (!working && dirty) {
            jobs <-= latest;
            working = 1;
            active = latest.ticket;
            dirty = 0;
        }
        alt {
        command := <-commands =>
            if (command.reply == nil) {
                continue;
            }
            if (command.operation == Shutdown) {
                if (working) {
                    requestcancel(cancel, active);
                    <-answers;
                }
                jobs <-= Request(latest.ticket, latest.revision, latest.language, "", 1);
                <-answers;
                command.reply <-= Result(latest.ticket, latest.revision, latest.language, Ok, nil, 0, 0, 0);
                return;
            }
            if (command.operation == Update) {
                job := command.job;
                if (job.ticket <= latest.ticket || job.revision < 0 || job.stop ||
                    (job.language != Limbo && job.language != ActionScript)) {
                    command.reply <-= Result(job.ticket, job.revision, job.language, Invalid, nil, 0, 0, 0);
                    continue;
                }
                if (len job.text > MaxCharacters) {
                    command.reply <-= Result(job.ticket, job.revision, job.language, TooLarge, nil, 0, 0, 0);
                    continue;
                }
                if (working) {
                    requestcancel(cancel, active);
                }
                latest = job;
                cached = Result(job.ticket, job.revision, job.language, Pending, nil, 0, 0, 0);
                dirty = 1;
                command.reply <-= copyresult(cached);
            } else if (command.operation == Snapshot) {
                command.reply <-= copyresult(cached);
            } else {
                command.reply <-= Result(latest.ticket, latest.revision, latest.language, Invalid, nil, 0, 0, 0);
            }
        result := <-answers =>
            working = 0;
            if (result.ticket != latest.ticket || result.revision != latest.revision ||
                result.language != latest.language) {
                continue;
            }
            cached = result;
            if (notices != nil) {
                alt {
                notices <-= result.ticket => ;
                * => ;
                }
            }
        }
    }
}


# Replace the sole pending cancellation with this active ticket. The lazy owner
# is the only producer, so its buffered send cannot wait for scanner completion.
# Draining a previous value is harmless even if the worker consumed it first.
requestcancel(cancel: chan of int, ticket: int)
{
    alt {
    <-cancel => ;
    * => ;
    }
    cancel <-= ticket;
}


# Consume at most one buffered value without blocking. Only an exact positive
# active ticket cancels: a late old request never aborts the following snapshot.
# The worker has sole receive ownership; no clock or UI callback is involved.
cancelled(cancel: chan of int, ticket: int): int
{
    alt {
    target := <-cancel =>
        return target > 0 && target == ticket;
    * =>
        return 0;
    }
}


# Validate scalar input, translate generated UTF-8 byte spans, return one complete catalog.
# baseline/catalog are private last-success values. Safe prefix restart plus generated
# token convergence can reuse an unchanged suffix; otherwise scanning continues to EOF.
# Scanner/boundary storage remain worker-local; failure has no partial tokens.
scan(job, baseline: Request, catalog: Result, cancel: chan of int,
     limbo: LimbolexParser, actionscript: AslexParser): Result
{
    result := Result(job.ticket, job.revision, job.language, Invalid, nil, 0, 0, 0);
    if (job.ticket <= 0 || job.revision < 0 ||
        (job.language != Limbo && job.language != ActionScript)) {
        return result;
    }
    if (len job.text > MaxCharacters) {
        result.status = TooLarge;
        return result;
    }
    if (cancelled(cancel, job.ticket)) {
        result.status = Cancelled;
        return result;
    }
    for (i := 0; i < len job.text; i++) {
        if (i % 256 == 0 && cancelled(cancel, job.ticket)) {
            result.status = Cancelled;
            return result;
        }
        scalar := job.text[i];
        if (scalar < 0 || scalar > 16r10ffff || (scalar >= 16rd800 && scalar <= 16rdfff)) {
            return result;
        }
    }
    reusable := catalog.status == Ok && baseline.language == job.language;
    if (reusable && baseline.text == job.text) {
        return Result(job.ticket, job.revision, job.language, Ok, catalog.tokens,
                      0, len catalog.tokens, len job.text);
    }
    prefix := 0;
    suffix := 0;
    retained := 0;
    restart := 0;
    if (reusable) {
        while (prefix < len baseline.text && prefix < len job.text &&
               baseline.text[prefix] == job.text[prefix]) {
            prefix++;
            if (prefix % 256 == 0 && cancelled(cancel, job.ticket)) {
                result.status = Cancelled;
                return result;
            }
        }
        while (suffix < len baseline.text - prefix && suffix < len job.text - prefix &&
               baseline.text[len baseline.text - suffix - 1] == job.text[len job.text - suffix - 1]) {
            suffix++;
            if (suffix % 256 == 0 && cancelled(cancel, job.ticket)) {
                result.status = Cancelled;
                return result;
            }
        }
        # Include the token ending exactly at the edit: identifiers/numbers or
        # two-byte comment openers may merge across that old boundary.
        while (retained < len catalog.tokens && catalog.tokens[retained].last < prefix) {
            retained++;
        }
        # Generated gaps combine adjacent whitespace and comments into one record.
        # A changed punctuation token may become a skipped comment and join that gap.
        if (retained > 0 && catalog.tokens[retained-1].kind == 5) {
            retained--;
        }
        if (retained < len catalog.tokens) {
            restart = catalog.tokens[retained].first;
        }
    }
    delta := len job.text - len baseline.text;
    suffixfirst := len job.text - suffix;
    bytes := array of byte job.text;
    if (len bytes > MaxBytes) {
        result.status = TooLarge;
        return result;
    }
    boundaries := array[len bytes + 1] of { * => -1 };
    position := 0;
    startbyte := 0;
    for (i = 0; i < len job.text; i++) {
        if (i % 256 == 0 && cancelled(cancel, job.ticket)) {
            result.status = Cancelled;
            return result;
        }
        boundaries[position] = i;
        if (i == restart) {
            startbyte = position;
        }
        scalar := job.text[i];
        if (scalar < 16r80) {
            position++;
        } else if (scalar < 16r800) {
            position += 2;
        } else if (scalar < 16r10000) {
            position += 3;
        } else {
            position += 4;
        }
    }
    boundaries[len bytes] = len job.text;
    resetok: int;
    if (job.language == Limbo) {
        resetok = limbo->resetat(bytes, startbyte);
    } else {
        resetok = actionscript->resetat(bytes, startbyte);
    }
    if (!resetok) {
        return result;
    }
    storage := array[MaxTokens] of Token;
    if (retained != 0) {
        storage[0:] = catalog.tokens[0:retained];
    }
    count := retained;
    reused := retained;
    scanned := 0;
    previous := startbyte;
    for (;;) {
        if (cancelled(cancel, job.ticket)) {
            result.status = Cancelled;
            return result;
        }
        kind, first, last: int;
        if (job.language == Limbo) {
            (kind, first, last) = limbo->next();
        } else {
            (kind, first, last) = actionscript->next();
        }
        if (kind == -1) {
            if (first != len bytes || last != first || previous != first) {
                return result;
            }
            break;
        }
        if (kind < 0 || kind > 5 || first != previous || last <= first ||
            last > len bytes || boundaries[first] < 0 || boundaries[last] < 0) {
            return result;
        }
        if (count == MaxTokens) {
            result.status = TooLarge;
            return result;
        }
        token := Token(kind, boundaries[first], boundaries[last]);
        storage[count++] = token;
        scanned++;
        previous = last;
        if (reusable && token.first >= suffixfirst) {
            # Equal token extent/class in the unchanged suffix proves the same
            # generated normal state after that token; only then reuse its tail.
            wanted := token.first - delta;
            low := 0;
            high := len catalog.tokens;
            while (low < high) {
                middle := low + (high - low) / 2;
                if (catalog.tokens[middle].first < wanted) {
                    low = middle + 1;
                } else {
                    high = middle;
                }
            }
            if (low < len catalog.tokens && catalog.tokens[low].first == wanted &&
                catalog.tokens[low].last == token.last - delta && catalog.tokens[low].kind == kind) {
                remaining := len catalog.tokens - low - 1;
                if (remaining > MaxTokens - count) {
                    result.status = TooLarge;
                    return result;
                }
                for (tail := low + 1; tail < len catalog.tokens; tail++) {
                    old := catalog.tokens[tail];
                    storage[count++] = Token(old.kind, old.first + delta, old.last + delta);
                }
                reused += remaining;
                break;
            }
        }
    }
    result.tokens = array[count] of Token;
    result.tokens[0:] = storage[0:count];
    result.status = Ok;
    result.scanned = scanned;
    result.reused = reused;
    result.restart = restart;
    return result;
}


# Serially serve immutable jobs independently of timeline frames; stop is acknowledged.
# [GC:] Channels retain snapshots/results until rendezvous; no cross-heap native pointers.
serve(jobs: chan of Request, answers: chan of Result, cancel: chan of int,
      limbo: LimbolexParser, actionscript: AslexParser)
{
    baseline := Request(0, 0, 0, "", 0);
    catalog := Result(0, 0, 0, Invalid, nil, 0, 0, 0);
    for (;;) {
        job := <-jobs;
        if (job.stop) {
            answers <-= Result(job.ticket, job.revision, job.language, Ok, nil, 0, 0, 0);
            return;
        }
        result := scan(job, baseline, catalog, cancel, limbo, actionscript);
        if (result.status == Ok) {
            baseline = job;
            catalog = result;
        }
        answers <-= copyresult(result);
    }
}


# Binary-search the visible catalog slice, then visit only intersecting records.
# Authentic worker catalogs are ordered. Invalid touched records reject the whole palette.
project(result: Result, revision, language, first, count: int): string
{
    if (result.status != Ok || result.revision != revision || result.language != language ||
        first < 0 || first > MaxCharacters || count < 0 || count > 200 ||
        count > MaxCharacters - first) {
        return "";
    }
    palette := array[count] of { * => byte '0' };
    low := 0;
    high := len result.tokens;
    while (low < high) {
        middle := low + (high - low) / 2;
        if (result.tokens[middle].last <= first) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    end := first + count;
    for (i := low; i < len result.tokens; i++) {
        token := result.tokens[i];
        if (token.first >= end) {
            break;
        }
        if (token.kind < 0 || token.kind > 5 || token.first < 0 || token.last <= token.first ||
            token.last > MaxCharacters) {
            return "";
        }
        begin := token.first;
        limit := token.last;
        if (begin < first) {
            begin = first;
        }
        if (limit > end) {
            limit = end;
        }
        for (offset := begin; offset < limit; offset++) {
            palette[offset-first] = byte ('0' + token.kind);
        }
    }
    return string palette;
}
