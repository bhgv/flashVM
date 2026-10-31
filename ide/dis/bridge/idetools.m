# Toolbar and subpalette policy; commands are values delivered to the app owner.
Idetools: module
{
    PATH: con "idetools.dis"; # Matching module resource.
    Count: con 21; # Existing fourteen shortcuts plus seven frequent/palette tools.
    Files, Edit, Search, Undo, Redo, Find, Replace, SaveAs, NewLimbo, NewAS1, NewAS2, NewAS3, Recent: con iota + 20; # App actions; existing1..19 stay unchanged.
    name: fn(index: int): string; # One-based toolbar caption; empty outside bounds.
    action: fn(index: int): int; # One-based toolbar action; zero outside bounds.
    palette: fn(action: int): int; # Whether this action opens a command palette.
    rows: fn(action: int): array of string; # Copied row labels, including session recent entries.
    command: fn(action, row: int): int; # Row action; zero means invalid.
};
