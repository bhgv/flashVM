implement Idetools;
include "idetools.m";


# Return one bounded toolbar label. No mutable UI state or native data is kept.
name(index: int): string
{
    names := array[] of {"Open", "Save", "Build", "New", "Debug", "Help", "Quit", "Close", "Clip", "All", "Max/N", "Draw", "MDI", "SDI", "Files", "Edit", "Search", "Undo", "Redo", "Find", "Save as"};
    if (index < 1 || index > len names) {
        return "";
    }
    return names[index - 1];
}


# Preserve legacy shortcut indices while mapping added tools to explicit actions.
action(index: int): int
{
    actions := array[] of {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 17, 18, 19, Files, Edit, Search, Undo, Redo, Find, SaveAs};
    if (index < 1 || index > len actions) {
        return 0;
    }
    return actions[index - 1];
}


# Return whether an action owns a left-opening subpalette, not a document.
palette(value: int): int
{
    return value == Files || value == Edit || value == Search;
}


# Return fresh immutable labels; the app resolves recent paths, never the view.
rows(value: int): array of string
{
    if (value == Files) {
        return array[] of {"New Limbo", "New AS1", "New AS2", "New AS3", "New SVG", "Open", "Save", "Save as", "Undo", "Redo", "Recent 1", "Recent 2", "Recent 3", "Recent 4"};
    }
    if (value == Edit) {
        return array[] of {"Undo", "Redo", "Copy", "Cut", "Paste"};
    }
    if (value == Search) {
        return array[] of {"Find in file", "Replace all in file"};
    }
    return array[0] of string;
}


# Map the exact displayed row to its application command. Invalid rows are inert.
command(value, row: int): int
{
    commands: array of int;
    if (value == Files) {
        commands = array[] of {NewLimbo, NewAS1, NewAS2, NewAS3, 17, 1, 2, SaveAs, Undo, Redo, Recent, Recent + 1, Recent + 2, Recent + 3};
    } else if (value == Edit) {
        commands = array[] of {Undo, Redo, 14, 15, 16};
    } else if (value == Search) {
        commands = array[] of {Find, Replace};
    }
    if (row < 0 || row >= len commands) {
        return 0;
    }
    return commands[row];
}
