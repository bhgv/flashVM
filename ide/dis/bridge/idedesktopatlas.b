# Compose the finite overview from copied live source references. The caller
# owns windows and the returned descriptors; no file, channel or renderer is used.
# Hidden/inactive MDI children do not duplicate their visible container. Fixed
# overlays are represented at the current logical viewport, not a false home row.
atlasitems(windows: array of ref Window, width, height, scroll, focus, zoom, pan: int): array of ref Item
{
    items := array[10] of ref Item;
    used := 0;
    (mx, my, mw, mh) := atlas->bounds(width, height);
    items[used++] = ref Item(60, 3, 0, mx - width + tools->RightWidth, my, mw, mh, 2, "", -1, -1);
    for (mapindex := 0; mapindex < len windows; mapindex++) {
        window := windows[mapindex];
        if (window == nil || window.hidden || (window.container != nil &&
            (window.container.hidden || window.container.rank != mapindex + 1))) {
            continue;
        }
        (wx, wy, ww, wh) := position(mapindex, width, height, window);
        if (window.container != nil) {
            (wx, wy, ww, wh) = position(window.container.root - 1, width, height, window.container);
        }
        if (viewportfixed(window)) {
            wx = 84 - 100 * pan / zoom;
            wy = 48 + 100 * scroll / zoom;
            ww = ww * 100 / zoom;
            wh = wh * 100 / zoom;
        }
        (px, py, pw, ph) := atlas->rect(width, height, wx, wy, ww, wh);
        if (pw > 0 && ph > 0) {
            identity := 89 + mapindex;
            if (mapindex == 7) {
                identity = 61;
            }
            items[used++] = ref Item(identity, 3, mapindex + 1, px - width + tools->RightWidth, py,
                pw, ph, mapindex + 1 == focus, "", -1, -1);
        }
    }
    (vx, vy, vw, vh) := atlas->viewport(width, height, zoom, pan, scroll);
    if (vw > 0 && vh > 0) {
        items[used++] = ref Item(62, 3, 0, vx - width + tools->RightWidth, vy, vw, vh, 4, "", -1, -1);
    }
    return items[:used];
}
