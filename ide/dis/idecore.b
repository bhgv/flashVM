implement Idecore;
include "idecore.m";


# [GC:] Private immutable snapshot stacks belong to the document owner. Replies
# contain text values, never mutable History references or list nodes.
History: adt
{
    undo: list of string;  # Most recent previous text first.
    redo: list of string;  # Most recent undone text first.
};


# Private state belongs to one service process; it is never exposed as a mutable UI object.
Document: adt
{
    opened: int;           # Whether text represents an open document.
    revision: int;         # Never reset by close/reopen, so stale edits cannot acquire a new session.
    saved: int;            # The last explicitly acknowledged clean revision.
    text: string;          # Character-indexed document content, bounded by MaxCharacters.
    history: ref History;  # Private bounded Undo/Redo roots; cleared across sessions.
};


# Create a typed channel boundary; no native UI or host-runtime call occurs here.
# Parameters: none. Returns a new unbuffered Request channel whose service owns
# one document. Each caller must receive its reply before abandoning a request.
start(): chan of Request
{
    commands := chan of Request;
    document := ref Document(0, 0, 0, "", ref History(nil, nil));
    spawn serve(commands, document);
    return commands;
}


# Copy observable state into a response without transferring the private mutable document.
# Parameters: document is borrowed owner state; id and status are copied into
# the reply. Returns a value snapshot; non-location result fields are -1.
snapshot(document: ref Document, id: int, status: int): Reply
{
    return Reply(id, status, document.opened, document.revision, document.saved, document.text, -1, -1, -1, -1, -1);
}


# Validate and apply one serialized command; the second result requests terminal shutdown.
# Parameters: document is mutable state owned by this service process; request
# supplies operation, expected revision and payload. Returns (reply, stop), where
# stop is one only for an accepted Shutdown. Rejected edits preserve state.
handle(document: ref Document, request: Request): (Reply, int)
{
    if(request.id < 0)
    {
        return (snapshot(document, request.id, Invalid), 0);
    }
    if(request.operation < Open || request.operation > Utf8Span)
    {
        return (snapshot(document, request.id, Unknown), 0);
    }
    if(request.operation == Snapshot)
    {
        return (snapshot(document, request.id, Ok), 0);
    }
    if(request.expected != document.revision)
    {
        return (snapshot(document, request.id, Conflict), 0);
    }
    if(request.operation == Shutdown)
    {
        return (snapshot(document, request.id, Ok), 1);
    }
    if(request.operation == Open)
    {
        if(document.opened)
        {
            return (snapshot(document, request.id, Invalid), 0);
        }
        if(len request.text > MaxCharacters)
        {
            return (snapshot(document, request.id, TooLarge), 0);
        }
        if(document.revision == MaxRevision)
        {
            return (snapshot(document, request.id, RevisionLimit), 0);
        }
        document.revision++;
        document.opened = 1;
        document.saved = document.revision;
        document.text = request.text;
        document.history = ref History(nil, nil);
        return (snapshot(document, request.id, Ok), 0);
    }
    if(!document.opened)
    {
        return (snapshot(document, request.id, Closed), 0);
    }
    if(request.operation == Undo || request.operation == Redo)
    {
        return (travel(document, request), 0);
    }
    if(request.operation == Utf8Span)
    {
        return (utf8span(document, request), 0);
    }
    if(request.operation == ReplaceAll)
    {
        return (replaceall(document, request), 0);
    }
    if(request.operation == Find)
    {
        return (find(document, request), 0);
    }
    if(request.operation == Locate || request.operation == Offset)
    {
        return (coordinates(document, request), 0);
    }
    if(request.operation == MarkSaved)
    {
        document.saved = document.revision;
        return (snapshot(document, request.id, Ok), 0);
    }
    if(request.operation == Replace)
    {
        if(request.first < 0 || request.last < request.first || request.last > len document.text)
        {
            return (snapshot(document, request.id, RangeError), 0);
        }
        remaining := len document.text - (request.last - request.first);
        if(len request.text > MaxCharacters - remaining)
        {
            return (snapshot(document, request.id, TooLarge), 0);
        }
        if(document.text[request.first:request.last] == request.text)
        {
            return (snapshot(document, request.id, Ok), 0);
        }
        if(document.revision == MaxRevision)
        {
            return (snapshot(document, request.id, RevisionLimit), 0);
        }
        # [GC:] Prepare text and bounded private history before publishing either.
        # Allocation traps belong to the runtime; no recovery guarantee is implied.
        text := document.text[:request.first] + request.text + document.text[request.last:];
        (_, undo) := pack(nil, document.text :: document.history.undo);
        history := ref History(undo, nil);
        document.text = text;
        document.history = history;
        document.revision++;
        return (snapshot(document, request.id, Ok), 0);
    }
    # Close retires the old session even when its text was already empty.
    if(document.revision == MaxRevision)
    {
        return (snapshot(document, request.id, RevisionLimit), 0);
    }
    document.revision++;
    document.opened = 0;
    document.saved = document.revision;
    document.text = "";
    document.history = ref History(nil, nil);
    return (snapshot(document, request.id, Ok), 0);
}


# Count logical snapshots/characters, including empty strings as entries.
# The current document and list/heap overhead are outside these logical budgets.
# Parameters: stack is a borrowed immutable snapshot list. Returns (entries,
# characters), counting Unicode string characters rather than UTF-8 bytes.
size(stack: list of string): (int, int)
{
    entries := 0;
    characters := 0;
    for(; stack != nil; stack = tl stack)
    {
        entries++;
        characters += len hd stack;
    }
    return (entries, characters);
}


# Drop the farthest snapshot without changing existing persistent list nodes.
# [GC:] Rebuilt nodes are private; retained text values remain runtime-owned.
# Parameters: stack is an immutable newest-first history list. Returns a list
# without its oldest entry; does not alter the supplied list or its strings.
dropoldest(stack: list of string): list of string
{
    if(stack == nil)
    {
        return nil;
    }
    reverse: list of string;
    for(; tl stack != nil; stack = tl stack)
    {
        reverse = hd stack :: reverse;
    }
    result: list of string;
    for(; reverse != nil; reverse = tl reverse)
    {
        result = hd reverse :: result;
    }
    return result;
}


# Prefer the newly pushed inverse snapshot: evict farthest source entries first,
# then farthest target entries. Work is bounded by MaxHistoryEntries + one.
# Parameters: source and target are immutable history lists. Returns budgeted
# lists after keeping newest entries within both history limits.
pack(source, target: list of string): (list of string, list of string)
{
    (se, sc) := size(source);
    (te, tc) := size(target);
    while(se + te > MaxHistoryEntries || sc + tc > MaxHistoryCharacters)
    {
        if(source != nil)
        {
            source = dropoldest(source);
            (se, sc) = size(source);
        }
        else
        {
            target = dropoldest(target);
            (te, tc) = size(target);
        }
    }
    return (source, target);
}


# Undo/Redo restores text as a NEW revision; old IDs never become valid again.
# saved remains the acknowledged disk revision, so text equality does not mark clean.
# Parameters: document is the service-owned state; request selects Undo or Redo
# and supplies the reply ID. Returns the resulting snapshot or a limit/invalid
# reply without partially publishing a history transition.
travel(document: ref Document, request: Request): Reply
{
    source := document.history.undo;
    target := document.history.redo;
    if(request.operation == Redo)
    {
        source = document.history.redo;
        target = document.history.undo;
    }
    if(source == nil)
    {
        return snapshot(document, request.id, NotFound);
    }
    if(document.revision == MaxRevision)
    {
        return snapshot(document, request.id, RevisionLimit);
    }
    text := hd source;
    (source, target) = pack(tl source, document.text :: target);
    history := ref History(source, target);
    if(request.operation == Redo)
    {
        history = ref History(target, source);
    }
    # [GC:] Publish only completed text/history; old Reply values keep their text.
    document.text = text;
    document.history = history;
    document.revision++;
    return snapshot(document, request.id, Ok);
}


# Resolve coordinates against the serialized revision without changing any state.
# LF alone starts a line; CR remains a character so every offset round-trips,
# including the position between CR and LF. No UTF-8/UTF-16/display conversion.
# Parameters: document supplies text; request selects Locate or Offset and the
# requested character offset or line/column. Returns coordinates or a range error.
# Lines are one-based; columns and string offsets count Unicode characters.
coordinates(document: ref Document, request: Request): Reply
{
    result := snapshot(document, request.id, RangeError);
    offset := 0;
    line := 1;
    column := 0;
    if(request.operation == Locate)
    {
        if(request.first < 0 || request.first > len document.text)
        {
            return result;
        }
        offset = request.first;
        for(index := 0; index < offset; index++)
        {
            if(document.text[index] == '\n')
            {
                line++;
                column = 0;
            }
            else
            {
                column++;
            }
        }
    }
    else
    {
        if(request.first < 1 || request.last < 0)
        {
            return result;
        }
        while(line < request.first && offset < len document.text)
        {
            if(document.text[offset] == '\n')
            {
                line++;
            }
            offset++;
        }
        if(line != request.first)
        {
            return result;
        }
        # Compare against the available line span before adding the column.
        end := offset;
        while(end < len document.text && document.text[end] != '\n')
        {
            end++;
        }
        if(request.last > end - offset)
        {
            return result;
        }
        column = request.last;
        offset += column;
    }
    result.status = Ok;
    result.offset = offset;
    result.line = line;
    result.column = column;
    return result;
}


# Count canonical UTF-8 bytes for one Unicode scalar, rejecting surrogate halves
# and values outside Unicode. Limbo string indices themselves count characters.
# Parameters: character is a character value. Returns its UTF-8 byte width;
# returns zero for a negative, surrogate or out-of-range scalar value.
utf8width(character: int): int
{
    if(character < 0 || character > 16r10ffff ||
    (character >= 16rd800 && character <= 16rdfff))
    {
        return 0;
    }
    if(character < 16r80)
    {
        return 1;
    }
    if(character < 16r800)
    {
        return 2;
    }
    if(character < 16r10000)
    {
        return 3;
    }
    return 4;
}


# Convert a compiler's original UTF-8 half-open span to revision-bound editor
# coordinates. Both endpoints must be exact scalar boundaries, including EOF.
# Scan the whole snapshot to reject non-Unicode Limbo strings consistently.
# [GC:] Only scalar counters are temporary; text/reply retain the usual owner roots.
# Parameters: document supplies decoded text; request.first/last form a half-open
# ORIGINAL UTF-8 byte span. Returns the corresponding character selection or an
# error when a boundary cuts an encoded character or is outside the text.
utf8span(document: ref Document, request: Request): Reply
{
    if(request.first < 0 || request.last < request.first)
    {
        return snapshot(document, request.id, RangeError);
    }
    first := -1;
    last := -1;
    bytes := 0;
    for(index := 0; index <= len document.text; index++)
    {
        if(bytes == request.first)
        {
            first = index;
        }
        if(bytes == request.last)
        {
            last = index;
        }
        if(index == len document.text)
        {
            break;
        }
        width := utf8width(document.text[index]);
        if(width == 0)
        {
            return snapshot(document, request.id, Invalid);
        }
        # MaxCharacters * 4 fits a signed int; each admitted scalar adds at most 4.
        bytes += width;
    }
    if(first < 0 || last < 0)
    {
        return snapshot(document, request.id, RangeError);
    }
    location := request;
    location.operation = Locate;
    location.first = first;
    result := coordinates(document, location);
    result.selection_end = last;
    return result;
}


# Compute failure links once per literal needle; callers admit nonempty lengths.
# [GC:] The returned private array is owned by the active serialized request.
# Parameters: needle is the nonempty literal search string. Returns a private
# prefix-function array indexed by Unicode character position; no state is published.
prefixes(needle: string): array of int
{
    prefix := array[len needle] of int;
    # Scalar array storage is unspecified for newa. KMP can read prefix[0]
    # after the first partial mismatch, before any loop iteration writes it.
    if(len needle != 0)
    {
        prefix[0] = 0;
    }
    matched := 0;
    for(index := 1; index < len needle; index++)
    {
        while(matched > 0 && needle[index] != needle[matched])
        {
            matched = prefix[matched - 1];
        }
        if(needle[index] == needle[matched])
        {
            matched++;
        }
        prefix[index] = matched;
    }
    return prefix;
}


# Build the checked result with balanced concatenations, rather than appending
# once per match (quadratic on long documents). Depth is O(log(matches + 1)).
# [GC:] All source/replacement/hit roots remain private to this request. Limbo
# values own the intermediate strings until their parent concatenation completes.
# Parameters: text is the original document; replacement is inserted at each
# hit; width is the matched character length; hits holds ordered match offsets.
# low/high select a half-open hit interval; first/last bound the source slice.
# Returns newly assembled text without mutating the original string.
render(text, replacement: string, width: int, hits: array of int,
       low, high, first, last: int): string
{
    if(low == high)
    {
        return text[first:last];
    }
    middle := low + (high - low) / 2;
    left := render(text, replacement, width, hits, low, middle, first, hits[middle]);
    right := render(text, replacement, width, hits, middle + 1, high, hits[middle] + width, last);
    joined := left + replacement;
    return joined + right;
}


# Replace nonoverlapping matches in the ORIGINAL half-open character range.
# Newly inserted text is never searched. Validate size/revision before rendering,
# then publish one new revision and one Undo snapshot. No-match is NotFound.
# Parameters: document is service-owned mutable state; request supplies a range,
# nonempty needle, literal replacement and reply ID. Returns an atomic edit result
# with match count, or an unchanged error snapshot when limits/ranges fail.
replaceall(document: ref Document, request: Request): Reply
{
    if(request.first < 0 || request.last < request.first || request.last > len document.text)
    {
        return snapshot(document, request.id, RangeError);
    }
    needle := request.text;
    if(len needle == 0)
    {
        return snapshot(document, request.id, Invalid);
    }
    if(len needle > request.last - request.first)
    {
        return snapshot(document, request.id, NotFound);
    }
    prefix := prefixes(needle);
    # [GC:] At most one integer per possible nonoverlapping match. The document
    # bounds this scratch table; neither it nor the prefix array leaves the owner.
    hits := array[(request.last - request.first) / len needle] of int;
    matched := 0;
    count := 0;
    length := len document.text;
    growth := len request.replacement - len needle;
    for(index := request.first; index < request.last; index++)
    {
        while(matched > 0 && document.text[index] != needle[matched])
        {
            matched = prefix[matched - 1];
        }
        if(document.text[index] == needle[matched])
        {
            matched++;
        }
        if(matched == len needle)
        {
            # Check before adding: count*growth could overflow a Limbo int.
            if(growth > 0 && growth > MaxCharacters - length)
            {
                return snapshot(document, request.id, TooLarge);
            }
            length += growth;
            hits[count++] = index + 1 - matched;
            matched = 0;
        }
    }
    if(count == 0)
    {
        return snapshot(document, request.id, NotFound);
    }
    if(needle != request.replacement)
    {
        if(document.revision == MaxRevision)
        {
            return snapshot(document, request.id, RevisionLimit);
        }
        text := render(document.text, request.replacement, len needle, hits,
        0, count, 0, len document.text);
        (_, undo) := pack(nil, document.text :: document.history.undo);
        history := ref History(undo, nil);
        # [GC:] Publish completed values together; retained replies keep old text.
        document.text = text;
        document.history = history;
        document.revision++;
    }
    result := snapshot(document, request.id, Ok);
    result.matches = count;
    return result;
}


# Search one revision's explicit half-open character range without wrapping.
# Prefix fallback gives linear time even for repeated near-matches; the table
# is bounded by the admitted document length and belongs only to this request.
# Parameters: document is borrowed current state; request supplies the bounded
# character range and nonempty literal needle. Returns the first match coordinates,
# NotFound, or an argument error; document and history are not modified.
find(document: ref Document, request: Request): Reply
{
    if(request.first < 0 || request.last < request.first || request.last > len document.text)
    {
        return snapshot(document, request.id, RangeError);
    }
    needle := request.text;
    if(len needle == 0)
    {
        return snapshot(document, request.id, Invalid);
    }
    if(len needle > request.last - request.first)
    {
        return snapshot(document, request.id, NotFound);
    }
    # [GC:] The owner roots document/needle and this private prefix table while
    # scanning. No array, mutable state or native pointer crosses a reply channel.
    prefix := prefixes(needle);
    matched := 0;
    for(index := request.first; index < request.last; index++)
    {
        while(matched > 0 && document.text[index] != needle[matched])
        {
            matched = prefix[matched - 1];
        }
        if(document.text[index] == needle[matched])
        {
            matched++;
        }
        if(matched == len needle)
        {
            location := request;
            location.operation = Locate;
            location.first = index + 1 - matched;
            return coordinates(document, location);
        }
    }
    return snapshot(document, request.id, NotFound);
}


# Own request ordering and reply backpressure; shutdown returns only after its acknowledgement.
# Parameters: commands is the owned request channel; document is private mutable
# state. Returns only after sending an accepted Shutdown reply. Receives and reply
# sends block; a nil reply channel is ignored without changing document state.
serve(commands: chan of Request, document: ref Document)
{
    for(;;)
    {
        request := <-commands;
        # [GC:] Channel values retain their own string/channel references. The owner keeps
        # the document rooted while waiting; replies never expose the mutable Document ADT.
        # A nil reply cannot be acknowledged, so it must not change or stop the service.
        if(request.reply == nil)
        {
            continue;
        }
        (reply, stop) := handle(document, request);
        request.reply <-= reply;
        if(stop)
        {
            return;
        }
    }
}
