# Pure picking of copied scene descriptors. Include idedesktop.m before this file.
Idepick: module {
    PATH: con "idepick.dis"; # Matching portable painter-order admission.
    # Return the topmost copied Item identity at canvas x/y, or0. No renderer access.
    topmost: fn(items: array of ref Idedesktop->Item, x, y, width, zoom, pan, scroll: int): int;
};
