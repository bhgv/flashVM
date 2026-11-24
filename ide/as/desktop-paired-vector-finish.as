// Copyright (c) 2026 bhgv
// EN: Present one completed drawing in its fixed pane viewport.
// RU: Показать законченный рисунок в неподвижной области просмотра панели.


// EN: Publish the pane camera and paint into a bounded viewport. The ordinary
// fitted view keeps vector paths. A changed camera clips through a native
// raster of the path clip using the same transform as the pointer inverse.
// RU: Опубликовать камеру панели и нарисовать в ограниченной области.
// Обычный вид сохраняет векторные контуры. При изменении камеры нативный
// растр обрезает контуры, используя то же преобразование, что и указатель.
function finishDesktopVector() {
    desktopVectorAt = (("0x" + desktopVectorSlot) * 1 - 1) * 25;
    desktopVectorMode = !(desktopVectorTool - 8);
    desktopVectorModeDigit = desktopVectorMode * 1;
    desktopStagedVectorCameras = desktopStagedVectorCameras.substr(0, desktopVectorAt)
        + desktopVectorCamera + desktopVectorModeDigit + desktopStagedVectorCameras.substr(desktopVectorAt + 25);
    desktopVectorScale = Math.min((desktopVectorPaneWidth - 16) / 1000, (desktopVectorPaneHeight - 32) / 600);
    desktopVectorScale = Math.max(1 / 1000, desktopVectorScale);
    desktopVectorWidth = 1000 * desktopVectorScale;
    desktopVectorHeight = 600 * desktopVectorScale;
    desktopVectorViewport._x = (desktopVectorPaneWidth - desktopVectorWidth) / 2;
    desktopVectorViewport._y = 28;
    desktopPaint = desktopVectorViewport.beginFill(16777215);
    desktopPaint = desktopVectorViewport.moveTo(0, 0);
    desktopPaint = desktopVectorViewport.lineTo(desktopVectorWidth, 0);
    desktopPaint = desktopVectorViewport.lineTo(desktopVectorWidth, desktopVectorHeight);
    desktopPaint = desktopVectorViewport.lineTo(0, desktopVectorHeight);
    desktopPaint = desktopVectorViewport.lineTo(0, 0);
    desktopPaint = desktopVectorViewport.endFill();
    if (Math.abs(desktopVectorZoom - 100) + Math.abs(desktopVectorCenterX - 500) + Math.abs(desktopVectorCenterY - 300)) {
        desktopVectorDisplayScale = desktopVectorScale * desktopVectorZoom / 100;
        desktopVectorShiftX = desktopVectorWidth / 2 - desktopVectorCenterX * desktopVectorDisplayScale;
        desktopVectorShiftY = desktopVectorHeight / 2 - desktopVectorCenterY * desktopVectorDisplayScale;
        desktopVectorPixels = desktopVectorClip.snapshotBitmap(Math.max(1, Math.floor(desktopVectorWidth)),
            Math.max(1, Math.floor(desktopVectorHeight)), desktopVectorDisplayScale, desktopVectorDisplayScale,
            desktopVectorShiftX, desktopVectorShiftY);
        desktopVectorRemoved = desktopVectorClip.removeMovieClip();
        if (desktopVectorPixels) {
            desktopVectorAttached = desktopVectorViewport.attachBitmap(desktopVectorPixels, 2, "never", false);
        }
    } else {
        desktopVectorClip._xscale = desktopVectorScale * 100;
        desktopVectorClip._yscale = desktopVectorScale * 100;
    }
    desktopVectorToolsDrawn = _root.paintDesktopVectorTools();
}
