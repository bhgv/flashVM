// Copyright (c) 2026 bhgv
// EN: Route the vector pane's independent viewport controls.
// RU: Передавать события независимых органов управления векторной панелью.


// EN: Use the committed tool state for wheel routing. Primary clicks on the
// two visible controls use the existing Limbo toggle and reset proposals.
// RU: Направлять колесо по опубликованному состоянию инструмента. Основная
// кнопка мыши использует действующие запросы Limbo для лупы и сброса.
function routeDesktopVectorPointer() {
    desktopPointerCameraAt = (("0x" + desktopPointerId) * 1 - 1) * 25;
    desktopPointerCamera = desktopFrontVectorCameras.substr(desktopPointerCameraAt, 25);
    if (!(desktopPointerCode - 1)) {
        desktopZoomButton = desktopPointerPane.getInstanceAtDepth(9);
        desktopResetButton = desktopPointerPane.getInstanceAtDepth(10);
        desktopOnZoomButton = desktopZoomButton.hitTest(desktopPointerX, desktopPointerY, false);
        desktopOnResetButton = desktopResetButton.hitTest(desktopPointerX, desktopPointerY, false);
        if (desktopOnZoomButton) {
            desktopPointerOffset = 0 - 21;
        } else {
            if (desktopOnResetButton) {
                desktopPointerOffset = 0 - 21;
                if (!(desktopPointerPhase - 1)) {
                    desktopPointerCode = 12;
                }
            } else {
                desktopVectorPointPlaced = _root.placeDesktopVectorPointer();
                if (!desktopVectorPointerInside) {
                    desktopPointerId = "00000000";
                }
            }
        }
    } else {
        desktopWheelCodes = "|4|5|8|9|";
        if ((desktopWheelCodes.indexOf("|" + desktopPointerCode + "|") + 1) * (desktopPointerCamera.substr(24, 1) * 1)) {
            desktopVectorPointPlaced = _root.placeDesktopVectorPointer();
        }
    }
}
