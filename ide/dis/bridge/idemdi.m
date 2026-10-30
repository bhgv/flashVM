# MDI membership on ONE shared desktop; presentation/native parenting is separate.
Idemdi: module
{
    PATH: con "idemdi.dis"; # Portable registry owner, never a workspace switcher.
    Source, Graphics: con iota + 1; # Tool families; a container accepts exactly one.
    Snapshot, Register, Create, Join, Leave, Remove, Dissolve, Shutdown: con iota; # FIFO commands.
    Ok, Invalid, Conflict: con iota; # Admission; refusal preserves membership.
    Limit: con 8; # Stable document/container identities in this first registry.


    # One copied command; no source/file/renderer ownership travels through it.
    Request: adt
    {
        id: int; # Nonnegative correlation identity.
        revision: int; # Exact current membership epoch; Snapshot ignores it.
        operation: int; # Command above.
        document: int; # Register/Join/Leave/Remove: stable pane identity1..Limit.
        container: int; # Create/Join/Dissolve: stable container identity1..Limit.
        family: int; # Register/Create: Source or Graphics; otherwise zero.
        reply: chan of Reply; # Unique required acknowledgement; caller must consume it.
    };


    # Receiver-owned membership copy. Zero container means SDI on the same desk.
    # Group tools derive from family, never from currently focused foreign content.
    Reply: adt
    {
        id: int; # Original correlation identity.
        revision: int; # Monotonic mutation epoch.
        status: int; # Admission result.
        families: array of int; # Document family by identity-1; zero means absent.
        containers: array of int; # Document container identity; zero means SDI.
        tools: array of int; # Container family by identity-1; zero means absent.
    };

    # Start one isolated membership owner. Removing/dissolving groups NEVER shuts
    # down documents, discards changes or changes their file identities. This
    # foundation is not an implemented MovieClip parent/tab/drag adapter yet.
    start: fn(): chan of Request;
};
