# Pure scroll-range policy shared by both desktop shelves.
Iderails: module
{
    PATH: con "iderails.dis"; # Matching module resource.
    clamp: fn(offset, count, spacing, top, height: int): int; # Clamp copied pixels to reachable content.
    region: fn(x, y, width: int): int; # 2 left, 3 right, zero outside rail content.
};
