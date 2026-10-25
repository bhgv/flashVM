# Portable document-state service. This interface uses no Draw, Tk, Prefab or crypto modules.
Idecore: module
{
    # Loader spelling is a relative module resource; the host chooses its namespace.
    PATH: con "idecore.dis";  # Relative resource resolved by the host module namespace.

    # Interface 0.5: clients must be recompiled with this header.
    # Document and insertion lengths count Unicode characters, not UTF-8 bytes or screen columns.
    MaxCharacters: con 1048576;   # Maximum document/insertion length in Unicode characters.
    MaxRevision: con 2147483647;  # Last representable service revision; never wrap.
    # Combined Undo/Redo snapshot budgets; current text is separate.
    MaxHistoryEntries: con 64;          # Combined maximum number of Undo and Redo snapshots.
    MaxHistoryCharacters: con 1048576;  # Combined history character budget, excluding current text.

    # Snapshot is read-only; all other operations use the expected revision as specified below.
    Open,                # Open a closed document and establish its initial text.
    Replace,             # Replace one half-open character range in the current revision.
    Snapshot,            # Read current state without requiring an expected revision.
    MarkSaved,           # Acknowledge that the current revision has been saved externally.
    Close,               # Close the document and clear its current text and history.
    Shutdown,            # Acknowledge and terminate the owner service.
    Locate,              # Convert a character offset to one-based line and zero-based column.
    Offset,              # Convert a line/column pair to a character offset.
    Find,                # Find the first literal match in a bounded character range.
    Undo,                # Restore a prior snapshot as a new revision.
    Redo,                # Restore an undone snapshot as a new revision.
    ReplaceAll,          # Atomically replace all nonoverlapping literal matches.
    Utf8Span: con iota;  # Convert original UTF-8 byte boundaries to a character selection.

    # Every accepted request with a non-nil reply channel receives exactly one response.
    Ok,                  # The request completed successfully.
    Conflict,            # The expected revision no longer matches current state.
    RangeError,          # The requested interval or coordinate is outside the document.
    TooLarge,            # An admitted document/insertion length would exceed its limit.
    Invalid,             # The request or source scalar/UTF-8 boundary is invalid.
    Closed,              # The operation requires an open document.
    RevisionLimit,       # A new revision would exceed MaxRevision.
    Unknown,             # The operation ID is outside the supported range.
    NotFound: con iota;  # No match/history entry exists for the requested operation.



    # Channel values copy these scalar/string/channel fields at rendezvous; no native handles.
    Request: adt
    {
        id: int;         # Nonnegative caller correlation ID; no deduplication is implied.
        operation: int;  # One operation above; unknown values have no side effects.
        expected: int;   # Required current revision except for Snapshot.
        first: int;      # Replace/ReplaceAll/Locate/Find: start offset; Offset: one-based line.
        last: int;       # Replace/ReplaceAll/Find: exclusive end; Offset: character column.
        text: string;    # Open: initial text; Replace: insertion; Find/ReplaceAll: nonempty needle.
        # Utf8Span reuses first/last for a half-open ORIGINAL UTF-8 byte range.
        replacement: string;   # ReplaceAll only: literal replacement; text is the nonempty needle.
        reply: chan of Reply;  # Required rendezvous destination; nil requests are ignored.
    };



    # A complete snapshot accompanies both successful and rejected commands.
    Reply: adt
    {
        id: int;             # Echoed correlation ID.
        status: int;         # One status above; errors preserve all document state.
        opened: int;         # Zero for no open document, one otherwise.
        revision: int;       # Monotonic service-lifetime content/session revision.
        saved: int;          # Revision acknowledged by MarkSaved, Open or Close.
        text: string;        # Current document; empty when closed.
        offset: int;         # Locate/Offset/Find/Utf8Span result in characters; -1 for other replies/errors.
        line: int;           # Locate/Offset/Find/Utf8Span one-based LF-delimited line; -1 otherwise.
        column: int;         # Locate/Offset/Find/Utf8Span zero-based character column; -1 otherwise.
        matches: int;        # ReplaceAll success: nonoverlapping matches; -1 for other replies.
        selection_end: int;  # Utf8Span success: exclusive character end; -1 otherwise.
    };



    # Spawn one owner process and return its unbuffered request channel.
    # Each caller must receive its reply before abandoning a request; no timeout/cancel yet.
    # Shutdown acknowledgement is terminal: no caller may send to this service afterwards.
    start: fn(): chan of Request;  # No parameters; returns the new service request channel.
};
