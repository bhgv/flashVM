implement Ideatlas;
include "ideatlas.m";
include "idetools.m";


# Return strip geometry from admitted IDE dimensions; no state or I/O is held.
extent(width, height: int): (int, int)
{
    return (width - 84 - Idetools->RightWidth, Pages * (height - 48));
}


# Preserve strip aspect until the half-height cap. The map is never translated
# by toolbar scrolling. Return a physical stage rectangle including its rim.
bounds(width, height: int): (int, int, int, int)
{
    (wide, high) := extent(width, height);
    mw := Idetools->RightWidth - 16;
    mh := mw * high / wide;
    if (mh > height / 2) {
        mh = height / 2;
    }
    # At the smallest admitted IDE height keep at least one whole tool usable.
    available := height - Idetools->ButtonTop - Idetools->ButtonHeight - 16;
    if (mh > available) {
        mh = available;
    }
    if (mh < 12) {
        mh = 12;
    }
    return (width - Idetools->RightWidth + 8, height - mh - 8, mw, mh);
}


# Return a finite physical scroll bound. Small zooms can show the entire strip.
maximum(height, zoom: int): int
{
    value := Pages * (height - 48) * zoom / 100 - (height - 48);
    if (value < 0) {
        value = 0;
    }
    return value;
}


# Project/clip logical desktop rectangles into the map's two-pixel inset.
# An entirely clipped window has zero width/height; no partial descriptor escapes.
rect(width, height, x, y, wide, high: int): (int, int, int, int)
{
    (tw, th) := extent(width, height);
    (mx, my, mw, mh) := bounds(width, height);
    x -= 84;
    y -= 48;
    right := x + wide;
    bottom := y + high;
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (right > tw) {
        right = tw;
    }
    if (bottom > th) {
        bottom = th;
    }
    if (right <= x || bottom <= y) {
        return (0, 0, 0, 0);
    }
    left := mx + 2 + x * (mw - 4) / tw;
    top := my + 2 + y * (mh - 4) / th;
    w := (right - x) * (mw - 4) / tw;
    h := (bottom - y) * (mh - 4) / th;
    if (w < 1) {
        w = 1;
    }
    if (h < 1) {
        h = 1;
    }
    return (left, top, w, h);
}


# Return the visible logical area projected through the same map transform.
viewport(width, height, zoom, pan, scroll: int): (int, int, int, int)
{
    (wide, unused) := extent(width, height);
    return rect(width, height, 84 - pan * 100 / zoom, 48 + scroll * 100 / zoom,
        wide * 100 / zoom, (height - 48) * 100 / zoom);
}


# Center a physical viewport on the tapped logical point and clamp both axes.
# Return copied native-camera values; callers retain sole layout ownership.
center(width, height, zoom, x, y: int): (int, int)
{
    (tw, th) := extent(width, height);
    (mx, my, mw, mh) := bounds(width, height);
    lx := (x - mx - 2) * tw / (mw - 4);
    ly := (y - my - 2) * th / (mh - 4);
    left := lx - tw * 50 / zoom;
    maximumleft := tw - tw * 100 / zoom;
    if (left > maximumleft) {
        left = maximumleft;
    }
    if (left < 0) {
        left = 0;
    }
    scroll := ly * zoom / 100 - (height - 48) / 2;
    if (scroll > maximum(height, zoom)) {
        scroll = maximum(height, zoom);
    }
    if (scroll < 0) {
        scroll = 0;
    }
    return (-left * zoom / 100, scroll);
}


# Classify a copied physical point; no thumbnail or document receives this hit.
contains(width, height, x, y: int): int
{
    (left, top, wide, high) := bounds(width, height);
    return x >= left && x < left + wide && y >= top && y < top + high;
}
