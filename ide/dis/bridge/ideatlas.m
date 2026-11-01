# Pure geometry for admitted IDE sizes240..2048 x160..2048 and zoom10..180.
# The finite strip and fixed overview own no document/native handles.
Ideatlas: module
{
    PATH: con "ideatlas.dis"; # Matching geometry module; no native/GUI dependency.
    Pages: con 7; # Logical viewport-height pages in the finite vertical strip.
    extent: fn(width, height: int): (int, int); # Logical strip size, excluding fixed rails/top bar.
    bounds: fn(width, height: int): (int, int, int, int); # Physical overview rectangle, fixed bottom right.
    maximum: fn(height, zoom: int): int; # Maximum physical table scroll at the current zoom.
    rect: fn(width, height, x, y, wide, high: int): (int, int, int, int); # Clip/project logical window bounds.
    viewport: fn(width, height, zoom, pan, scroll: int): (int, int, int, int); # Physical frame over the map.
    center: fn(width, height, zoom, x, y: int): (int, int); # Tap -> checked physical pan/scroll.
    contains: fn(width, height, x, y: int): int; # Whether a physical point belongs to the fixed map.
};
