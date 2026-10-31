# Toolbar and subpalette policy; commands are values delivered to the app owner.
Idetools: module
{
    PATH: con "idetools.dis"; # Matching module resource.
    Count: con 11; # Palettes plus the explicit frequent-command exceptions.
    RightWidth: con 60; # Physical right shelf including its graduated inner edge.
    ButtonWidth: con 44; # Common button width.
    ButtonHeight: con 27; # Common button height.
    ButtonStep: con 32; # Vertical spacing including gaps.
    ButtonTop: con 56; # First button below the fixed title bar.
    ButtonLeft: con 10; # Shelf-local left edge; six pixels remain to its right.
    Windows: con 36; # View/group commands, outside the four recent-path action slots.
    identity: fn(index: int): int; # Stable MovieClip ID for one toolbar ordinal.
    index: fn(action: int): int; # Toolbar ordinal for a command, zero if palette-only.
    Files, Edit, Search, Undo, Redo, Find, Replace, SaveAs, NewLimbo, NewAS1, NewAS2, NewAS3, Recent: con iota + 20; # App actions; existing1..19 stay unchanged.
    name: fn(index: int): string; # One-based toolbar caption; empty outside bounds.
    action: fn(index: int): int; # One-based toolbar action; zero outside bounds.
    palette: fn(action: int): int; # Whether this action opens a command palette.
    rows: fn(action: int): array of string; # Copied row labels, including session recent entries.
    command: fn(action, row: int): int; # Row action; zero means invalid.
};
