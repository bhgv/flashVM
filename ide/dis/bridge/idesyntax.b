implement Idesyntax;
include "idesyntax.m";
include "limbolex.m";
include "aslex.m";


# Resolve independent scanner module instances before spawning. No handwritten fallback.
# Returned channels carry copied values; each scanner's mutable tables have one owner.
start(): (chan of Request, chan of Result)
{
    limbo := load LimbolexParser LimbolexParser->PATH;
    actionscript := load AslexParser AslexParser->PATH;
    if (limbo == nil || actionscript == nil) {
        return (nil, nil);
    }
    jobs := chan of Request;
    answers := chan of Result;
    spawn serve(jobs, answers, limbo, actionscript);
    return (jobs, answers);
}


# Resolve dependencies before returning a lazy command capability; nil means unavailable.
# One owning Dis process serializes client updates and immutable completed results.
startlazy(notices: chan of int): chan of Command
{
    (jobs, answers) := start();
    if (jobs == nil) {
        return nil;
    }
    commands := chan of Command;
    spawn lazyowner(commands, notices, jobs, answers);
    return commands;
}


# Keep only the latest pending snapshot while one worker owns an active immutable job.
# Update acknowledgements precede scanning. Stale ticket/revision/profile results are dropped.
# [GC:] The owner retains at most active/latest snapshots and one completed catalog.
lazyowner(commands: chan of Command, notices: chan of int,
          jobs: chan of Request, answers: chan of Result)
{
    latest := Request(0, 0, Limbo, "", 0);
    cached := Result(0, 0, Limbo, Pending, nil);
    working := 0;
    dirty := 0;
    for (;;) {
        if (!working && dirty) {
            jobs <-= latest;
            working = 1;
            dirty = 0;
        }
        alt {
        command := <-commands =>
            if (command.reply == nil) {
                continue;
            }
            if (command.operation == Shutdown) {
                if (working) {
                    <-answers;
                }
                jobs <-= Request(latest.ticket, latest.revision, latest.language, "", 1);
                <-answers;
                command.reply <-= Result(latest.ticket, latest.revision, latest.language, Ok, nil);
                return;
            }
            if (command.operation == Update) {
                job := command.job;
                if (job.ticket <= latest.ticket || job.revision < 0 || job.stop ||
                    (job.language != Limbo && job.language != ActionScript)) {
                    command.reply <-= Result(job.ticket, job.revision, job.language, Invalid, nil);
                    continue;
                }
                if (len job.text > MaxCharacters) {
                    command.reply <-= Result(job.ticket, job.revision, job.language, TooLarge, nil);
                    continue;
                }
                latest = job;
                cached = Result(job.ticket, job.revision, job.language, Pending, nil);
                dirty = 1;
                command.reply <-= cached;
            } else if (command.operation == Snapshot) {
                command.reply <-= cached;
            } else {
                command.reply <-= Result(latest.ticket, latest.revision, latest.language, Invalid, nil);
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


# Validate scalar input, translate generated UTF-8 byte spans, return one complete catalog.
# Scanner instances and boundary storage remain worker-local; failure has no partial tokens.
scan(job: Request, limbo: LimbolexParser, actionscript: AslexParser): Result
{
    result := Result(job.ticket, job.revision, job.language, Invalid, nil);
    if (job.ticket <= 0 || job.revision < 0 ||
        (job.language != Limbo && job.language != ActionScript)) {
        return result;
    }
    if (len job.text > MaxCharacters) {
        result.status = TooLarge;
        return result;
    }
    for (i := 0; i < len job.text; i++) {
        scalar := job.text[i];
        if (scalar < 0 || scalar > 16r10ffff || (scalar >= 16rd800 && scalar <= 16rdfff)) {
            return result;
        }
    }
    bytes := array of byte job.text;
    if (len bytes > MaxBytes) {
        result.status = TooLarge;
        return result;
    }
    boundaries := array[len bytes + 1] of { * => -1 };
    position := 0;
    for (i = 0; i < len job.text; i++) {
        boundaries[position] = i;
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
    if (job.language == Limbo) {
        limbo->reset(bytes);
    } else {
        actionscript->reset(bytes);
    }
    storage := array[MaxTokens] of Token;
    count := 0;
    previous := 0;
    for (;;) {
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
        storage[count++] = Token(kind, boundaries[first], boundaries[last]);
        previous = last;
    }
    result.tokens = array[count] of Token;
    result.tokens[0:] = storage[0:count];
    result.status = Ok;
    return result;
}


# Serially serve immutable jobs independently of timeline frames; stop is acknowledged.
# [GC:] Channels retain snapshots/results until rendezvous; no cross-heap native pointers.
serve(jobs: chan of Request, answers: chan of Result,
      limbo: LimbolexParser, actionscript: AslexParser)
{
    for (;;) {
        job := <-jobs;
        if (job.stop) {
            answers <-= Result(job.ticket, job.revision, job.language, Ok, nil);
            return;
        }
        answers <-= scan(job, limbo, actionscript);
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
