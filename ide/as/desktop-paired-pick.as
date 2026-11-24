// Copyright (c) 2026 bhgv
// EN: Pick within the displayed layer graph in top-to-bottom order.
// RU: Определить цель в показанном графе слоёв сверху вниз.


// EN: Resolve the first real item, allowing gaps in an upper layer to pass through.
// RU: Найти первый реальный элемент, пропуская пустоты в верхнем слое.
function pickDesktopPane() {
    desktopPointerId = "00000000";
    desktopPointerPane = false;
    desktopPointerLayerDepth = 5;
    desktopLayerPicked = _root.pickDesktopLayer();
    desktopPointerLayerDepth = 4;
    desktopLayerPicked = _root.pickDesktopLayer();
    desktopPointerLayerDepth = 3;
    desktopLayerPicked = _root.pickDesktopLayer();
    desktopPointerLayerDepth = 2;
    desktopLayerPicked = _root.pickDesktopLayer();
    desktopPointerLayerDepth = 1;
    desktopLayerPicked = _root.pickDesktopLayer();
}
