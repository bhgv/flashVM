implement Idetools;
include "idetools.m";


# Return one bounded toolbar label. No mutable UI state or native data is kept.
name(index: int): string
{
    names := array[] of {"Files", "Save", "Build", "Edit", "Debug", "Help", "Search", "Find", "Undo", "Redo", "View"};
    if (index < 1 || index > len names) {
        return "";
    }
    return names[index - 1];
}


# Preserve legacy shortcut indices while mapping added tools to explicit actions.
action(index: int): int
{
    actions := array[] of {Files, 2, 3, Edit, 5, 6, Search, Find, Undo, Redo, Windows};
    if (index < 1 || index > len actions) {
        return 0;
    }
    return actions[index - 1];
}


# Return whether an action owns a left-opening subpalette, not a document.
palette(value: int): int
{
    return value == Files || value == Edit || value == Search || value == Windows;
}


# Return fresh immutable labels; the app resolves recent paths, never the view.
rows(value: int): array of string
{
    if (value == Files) {
        return array[] of {"Limbo", "AS1", "AS2", "AS3", "SVG", "Open", "Save", "SaveAs", "Undo", "Redo", "R1", "R2", "R3", "R4", "Close", "Quit"};
    }
    if (value == Edit) {
        return array[] of {"Undo", "Redo", "Copy", "Cut", "Paste"};
    }
    if (value == Search) {
        return array[] of {"Find", "Repl."};
    }
    if (value == Windows) {
        return array[] of {"All", "Max/N", "MDI", "SDI", "Close"};
    }
    return array[0] of string;
}


# Map the exact displayed row to its application command. Invalid rows are inert.
command(value, row: int): int
{
    commands: array of int;
    if (value == Files) {
        commands = array[] of {NewLimbo, NewAS1, NewAS2, NewAS3, 17, 1, 2, SaveAs, Undo, Redo, Recent, Recent + 1, Recent + 2, Recent + 3, 8, 7};
    } else if (value == Edit) {
        commands = array[] of {Undo, Redo, 14, 15, 16};
    } else if (value == Search) {
        commands = array[] of {Find, Replace};
    }
    if (value == Windows) {
        commands = array[] of {10, 11, 18, 19, 8};
    }
    if (row < 0 || row >= len commands) {
        return 0;
    }
    return commands[row];
}


# Preserve MovieClip identities when toolbar order is simplified. Invalid index
# returns zero; values never identify a document or transfer ownership.
identity(index: int): int
{
    identities := array[] of {64, 42, 43, 65, 45, 46, 66, 69, 67, 68, 70};
    if (index < 1 || index > len identities) {
        return 0;
    }
    return identities[index - 1];
}


# Locate the visible command that owns a palette. Return zero when it is absent.
index(value: int): int
{
    for (ordinal := 1; ordinal <= Count; ordinal++) {
        if (action(ordinal) == value) {
            return ordinal;
        }
    }
    return 0;
}
