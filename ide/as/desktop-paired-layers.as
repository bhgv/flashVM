// Copyright (c) 2026 bhgv
// EN: Create the five desktop layer MovieClips for one staged scene.
// RU: Создать пять слоёв MovieClip для одного снимка desktop.


// EN: Set the table transform from the same anchor and offsets used by Dis hit admission.
// RU: Настроить матрицу стола по той же точке и смещениям, что использует Dis.
function setupDesktopLayers() {
    desktopStageLayer = desktopPending.createEmptyMovieClip("stage", 1);
    desktopTableLayer = desktopPending.createEmptyMovieClip("table", 2);
    desktopLeftLayer = desktopPending.createEmptyMovieClip("leftRail", 3);
    desktopRightLayer = desktopPending.createEmptyMovieClip("rightRail", 4);
    desktopOverlayLayer = desktopPending.createEmptyMovieClip("overlay", 5);
    desktopTableLayer._xscale = 100 * desktopScale;
    desktopTableLayer._yscale = 100 * desktopScale;
    desktopTableX = 84 * (1 - desktopScale) + desktopPan;
    desktopTableY = 48 * (1 - desktopScale) - desktopScroll;
    desktopTableLayer._x = desktopTableX;
    desktopTableLayer._y = desktopTableY;
    desktopStagedGroups = "000000000000000000000000000000000000000000000000000000000000000000000000";
}
