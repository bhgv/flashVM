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

    Panel: con 65543;     # View prompt mode 0..10 (9 browser, 10 directory path), select-all flag, counted UTF-8 text.
    Find: con 65544;      # Revision-bound literal FindNext request; text replaces the current query.
    SaveAs: con 65545;    # Revision-bound explicit destination; adopted only after a successful write.
    Open: con 65546;      # Revision-bound strict source load; dirty documents are never discarded.

    Zoom: con 65547;      # Ticket/revision-bound zoom proposal: first +/-1, second measured rows.
    Style: con 65548;     # View font size 6..48 and logical rows 1..128, both chosen/admitted by Dis.
    Pinch: con 65550;     # Raw input pointer-code extension: first is a normalized +/-1 pinch step.

    GoTo: con 65551;      # Revision-bound one-based line/column in first/second, empty text.

    Resize: con 65552;    # Request width/height in pixels; text is ticket+row-capacity hex, flags=revision.
    Canvas: con 65553;    # View admitted width 240..2048 and height 160..2048 before Style.

    ClipboardWrite: con 65554; # View authorized source selection; flags=revision, first/last, text=ticket.
    ClipboardRead: con 65556; # View request for a bounded native UTF-8 clipboard snapshot, text=ticket.
    ClipboardData: con 65557; # Request ordered paste fragment: offset/total bytes, text=ticket+body.
    ClipboardAck: con 65558; # Request write/read error acknowledgement: first=status, text=ticket.

    ClipboardContinue: con 65559; # View fragment credit: first=next offset, second=total, text=original ticket.

    PanelSelection: con 65560; # View field caret/anchor as Unicode scalars, flags=document revision.

    ClipboardFieldWrite: con 65561; # View copy of a completed modal field range, same revision/ticket binding.
    FieldPlace: con 65562; # Request field scalar/phase 1..3 from actual glyph geometry, revision/ticket bound.

    Gutter: con 65563; # View-only line-number visibility in first=0/1, flags=document revision.

    Browse: con 65564; # Request directory path in text; first=0, second=completed-view ticket, flags=revision.
    BrowseMove: con 65565; # Request selected-entry delta -1/+1; second=ticket, empty text, flags=revision.
    BrowseNext: con 65566; # Request the next native batch; first=0, second=ticket, empty text.
    BrowseChoose: con 65567; # Navigate/open selected entry under dirty-source admission; same ticket/revision.
    BrowseClose: con 65568; # Close the browser cursor; first=0, second=ticket, empty text.

    # Read the initial --line-numbers 0/1 preference; subsequent visibility belongs to Dis.
    linenumbers: fn(): int;

    # Actual initial font geometry, returned as a bounded copied value without document access.
    rows: fn(): int;

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
