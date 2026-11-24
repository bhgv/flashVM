// Copyright (c) 2026 bhgv
// EN: Own the active stroke's viewport, brush style and source fence.
// RU: Удерживать область просмотра, стиль и ревизию активного штриха.


// EN: Capture from the displayed pane, using copied window pixels and page units.
// RU: Начать штрих в показанной панели по скопированным пикселям и единицам страницы.
function startDesktopInk() {
    desktopInkConsumed = true;
    desktopInkSlot = desktopPointerId;
    desktopInkTicket = desktopFrontTicket;
    desktopInkRevision = desktopInkStyle.substr(0, 8);
    desktopInkDownX = Math.floor(desktopPointerX);
    desktopInkDownY = Math.floor(desktopPointerY);
    desktopInkPoints = "";
    desktopInkCount = 0;
    desktopInkStartTime = desktopPointerTime;
    desktopInkElapsed = 0;
    desktopInkViewport = desktopPointerPane.getInstanceAtDepth(8);
    desktopInkClip = desktopInkViewport.createEmptyMovieClip("localInk", 3);
    desktopInkColor = ("0x" + desktopInkStyle.substr(8, 8)) * 1;
    desktopInkWidth = ("0x" + desktopInkStyle.substr(16, 8)) * desktopPointerScale;
    desktopInkWidth = Math.max(1, desktopInkWidth);
    desktopInkRight = desktopVectorPointerScale * 1000;
    desktopInkBottom = desktopVectorPointerScale * 600;
    desktopInkSampled = _root.appendDesktopInkSample();
    desktopInkPainted = _root.paintDesktopInkSample();
}
