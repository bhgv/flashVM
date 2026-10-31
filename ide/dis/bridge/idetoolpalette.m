# Pure geometry for a compact, owner-anchored command bubble.
Idetoolpalette: module
{
    PATH: con "idetoolpalette.dis"; # Matched implementation.
    columns: fn(height: int): int; # Extra column on short canvases.
    legend: fn(height: int): int; # Footer height; omit on tiny canvases.
    bounds: fn(owner_y, count, notes, width, height: int): (int, int, int, int); # Body rectangle; arrow is separate.
    cell: fn(row, x, y, height: int): (int, int); # Button top-left, shared 44x27 extent.
    hit: fn(px, py, x, y, count, height: int): int; # Row or -1, gaps/notes never execute.
    contains: fn(px, py, x, y, wide, high: int): int; # Body admission only.
};
