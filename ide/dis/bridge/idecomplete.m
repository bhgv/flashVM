# Channel-owned completion index, populated from compiler/module declaration records.
Idecomplete: module
{
    PATH: con "idecomplete.dis";   # Relative service module resource.
    MaxEntries: con 4096;          # Bounded published candidates per service instance.
    MaxResults: con 64;            # Maximum copied candidates in one query response.
    MaxParameters: con 16;         # Bounded parameter descriptors per signature.
    MaxCatalogBytes: con 4194304;  # Aggregate copied text bytes admitted in one generation.
    MaxFieldBytes: con 512;        # UTF-8 limit for each name/signature/documentation field.
    Publish,                       # Atomically replace the admitted symbol generation.
    Query,                         # Return a bounded prefix lookup for an exact generation.
    Shutdown: con iota;            # Acknowledge and stop this owner.
    Ok,                            # The operation completed successfully.
    Invalid,                       # The request/batch violates the service contract.
    Conflict,                      # Publication or query uses a stale generation.
    TooLarge: con iota;            # A field, batch or query exceeds the declared quota.


    # Structured short help for one argument, suitable for an active-parameter call tip.
    Parameter: adt
    {
        name: string;       # Parameter spelling, including a final ...args when applicable.
        type_name: string;  # Human-readable source-language type.
        optional: int;      # Boolean; optional/default/rest arguments are marked explicitly.
        summary: string;    # Brief plain-text explanation, not executable markup.
    };


    # Value metadata supplied by compiler/module tooling, never inferred by a second source parser.
    Item: adt
    {
        name: string;                        # Qualified, case-sensitive completion spelling.
        kind: int;                           # Nonnegative provider-defined symbol/category identifier.
        signature: string;                   # Display/call signature supplied by the owning frontend.
        documentation: string;               # Bounded brief help, supplied as text rather than executable markup.
        parameters: array of ref Parameter;  # Structured call tip, copied independently at both boundaries.
    };


    # A caller retains its reply channel until exactly one result arrives.
    Request: adt
    {
        id: int;                   # Nonnegative correlation chosen by the caller.
        operation: int;            # Publish, Query or Shutdown.
        generation: int;           # Publish: strictly newer index generation; Query: exact expected generation.
        prefix: string;            # Query: canonical case-sensitive name prefix.
        limit: int;                # Query: 1..MaxResults; no unbounded result list.
        items: array of ref Item;  # Publish: provider records, copied before acknowledgement.
        reply: chan of Reply;      # Required result rendezvous; caller must consume it.
    };


    # Arrays and Item records are copied before publication; receiver mutation cannot alter the index.
    Reply: adt
    {
        id: int;                   # Echoed request correlation.
        status: int;               # One result above; errors retain the previous index.
        generation: int;           # Current admitted generation, including on failure.
        items: array of ref Item;  # Owned bounded result records; nil for operations without candidates.
    };


    # Spawn one owner and return an ordinary unbuffered Dis request channel.
    # Returns a new channel; caller must acknowledge Shutdown before dropping the last owner.
    start: fn(): chan of Request;
};
