// Copyright (c) 2026 bhgv
// EN: Resolve the frontmost child of one layer and descend through any MDI root.
// RU: Найти передний элемент слоя и при необходимости пройти в корень MDI.


// EN: Leave the current result empty when this layer has no item at the point.
// RU: Оставить результат пустым, если в этой точке слой не содержит элементов.
function pickDesktopLayer() {
    if (!desktopPointerPane) {
        desktopPointerLayer = desktopFront.getInstanceAtDepth(desktopPointerLayerDepth);
        desktopPointerDepth = desktopPointerLayer.getChildDepthAtPoint(desktopPointerX, desktopPointerY);
        if (desktopPointerDepth) {
            desktopPointerId = desktopFrontOrder.substr((desktopPointerDepth - 1) * 8, 8);
            desktopPointerPane = desktopPointerLayer.getInstanceAtDepth(desktopPointerDepth);
            desktopPointerNumericId = ("0x" + desktopPointerId) * 1;
            desktopPointerGroup = desktopGroupIds.indexOf("|" + desktopPointerNumericId + "|") + 1;
            if (desktopPointerGroup) {
                desktopPointerChild = desktopPointerPane.getChildDepthAtPoint(desktopPointerX, desktopPointerY);
                if (desktopPointerChild) {
                    desktopPointerId = desktopFrontOrder.substr((desktopPointerChild - 1) * 8, 8);
                    desktopPointerPane = desktopPointerPane.getInstanceAtDepth(desktopPointerChild);
                }
            }
        }
    }
}
