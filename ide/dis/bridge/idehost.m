# Native window boundary: only copied values and ordinary Dis byte-array channels.
Idehost: module
{
    PATH: con "$Idehost"; # Builtin supplied only by the graphical runtime executable.
    Save: con 65537;      # Save current revision to the configured native file path.
    Reload: con 65538;    # Reload that path only when the current document is clean.
    Quit: con 65539;      # Stop after a clean-state check; flags=1 explicitly discards edits.

    Viewport: con 65540;  # View metadata: first/last source scalars; payload top/total/ticket hex words.
    Scroll: con 65541;    # Request logical-line delta in first; flags=revision, payload=ticket hex.
    Place: con 65542;     # Request source scalar in first, phase 1/2/3 in second; same revision/ticket.

    Panel: con 65543;     # View prompt mode in first, select-all flag in second, counted UTF-8 text.
    Find: con 65544;      # Revision-bound literal FindNext request; text replaces the current query.
    SaveAs: con 65545;    # Revision-bound explicit destination; adopted only after a successful write.
    Open: con 65546;      # Revision-bound strict source load; dirty documents are never discarded.

    # Borrow ports while init's stack roots remain live; no cross-heap pointers escape.
    # Returns zero on registration, minus one for invalid/repeated registration.
    attach: fn(input: chan of array of byte, output: chan of array of byte): int;

    # Stop native channel borrowing before the entry stack releases its roots.
    # The controller calls this only after all private services acknowledge shutdown.
    detach: fn();

    # Return one only for explicit --new PATH; the controller skips initial file reading.
    fresh: fn(): int;

    # Return a copied configured document path. Empty means no file was selected.
    # This is configuration only; all document I/O and save policy remain in Limbo.
    path: fn(): string;
};
