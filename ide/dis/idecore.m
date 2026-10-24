# Portable document-state service. This interface uses no Draw, Tk, Prefab or crypto modules.
Idecore: module
{
    # Loader spelling is a relative module resource; the host chooses its namespace.
    PATH: con "idecore.dis";

    # Document and insertion lengths count Unicode characters, not UTF-8 bytes or screen columns.
    MaxCharacters: con 1048576;
    MaxRevision: con 2147483647;

    # Snapshot is read-only; all other operations use the expected revision as specified below.
    Open, Replace, Snapshot, MarkSaved, Close, Shutdown: con iota;

    # Every accepted request with a non-nil reply channel receives exactly one response.
    Ok, Conflict, RangeError, TooLarge, Invalid, Closed, RevisionLimit, Unknown: con iota;

    # Channel values copy these scalar/string/channel fields at rendezvous; no native handles.
    Request: adt
    {
        id: int;                 # Nonnegative caller correlation ID; no deduplication is implied.
        operation: int;          # One operation above; unknown values have no side effects.
        expected: int;           # Required current revision except for Snapshot.
        first: int;              # Inclusive character offset for Replace.
        last: int;               # Exclusive character offset for Replace.
        text: string;            # Complete initial text for Open, replacement text for Replace.
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
    };

    # Spawn one owner process and return its unbuffered request channel.
    # Each caller must receive its reply before abandoning a request; no timeout/cancel yet.
    # Shutdown acknowledgement is terminal: no caller may send to this service afterwards.
    start: fn(): chan of Request;
};
