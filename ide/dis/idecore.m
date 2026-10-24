# Portable document-state service. This interface uses no Draw, Tk, Prefab or crypto modules.
Idecore: module
{
    # Loader spelling is a relative module resource; the host chooses its namespace.
    PATH: con "idecore.dis";

    # Document and insertion lengths count Unicode characters, not UTF-8 bytes or screen columns.
    MaxCharacters: con 1048576;
    MaxRevision: con 2147483647;
    # Combined Undo/Redo snapshot budgets; current text is separate.
    MaxHistoryEntries: con 64;
    MaxHistoryCharacters: con 1048576;

    # Snapshot is read-only; all other operations use the expected revision as specified below.
    Open, Replace, Snapshot, MarkSaved, Close, Shutdown, Locate, Offset, Find, Undo, Redo: con iota;

    # Every accepted request with a non-nil reply channel receives exactly one response.
    Ok, Conflict, RangeError, TooLarge, Invalid, Closed, RevisionLimit, Unknown, NotFound: con iota;

    # Channel values copy these scalar/string/channel fields at rendezvous; no native handles.
    Request: adt
    {
        id: int;                 # Nonnegative caller correlation ID; no deduplication is implied.
        operation: int;          # One operation above; unknown values have no side effects.
        expected: int;           # Required current revision except for Snapshot.
        first: int;              # Replace/Locate/Find: start offset; Offset: one-based line.
        last: int;               # Replace/Find: exclusive end; Offset: character column.
        text: string;            # Open: initial text; Replace: insertion; Find: nonempty needle.
        reply: chan of Reply;    # Required rendezvous destination; nil requests are ignored.
    };

    # A complete snapshot accompanies both successful and rejected commands.
    Reply: adt
    {
        id: int;                 # Echoed correlation ID.
        status: int;             # One status above; errors preserve all document state.
        opened: int;             # Zero for no open document, one otherwise.
        revision: int;           # Monotonic service-lifetime content/session revision.
        saved: int;              # Revision acknowledged by MarkSaved, Open or Close.
        text: string;            # Current document; empty when closed.
        offset: int;             # Locate/Offset/Find result in characters; -1 for other replies/errors.
        line: int;               # Locate/Offset/Find one-based LF-delimited line; -1 otherwise.
        column: int;             # Locate/Offset/Find zero-based character column; -1 otherwise.
    };

    # Spawn one owner process and return its unbuffered request channel.
    # Each caller must receive its reply before abandoning a request; no timeout/cancel yet.
    # Shutdown acknowledgement is terminal: no caller may send to this service afterwards.
    start: fn(): chan of Request;
};
