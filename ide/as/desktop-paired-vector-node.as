// Copyright (c) 2026 bhgv
// EN: Draw checked, copied path commands in the staged vector MovieClip.
// RU: Нарисовать проверенные команды контуров в подготовленном MovieClip.


// EN: Consume a node of the current shape and close the fill at its exact end.
// RU: Принять узел текущего контура и завершить заливку на его точной границе.
function paintDesktopVectorNode() {
    desktopVectorVerb = ("0x" + desktopVectorRecord.substr(0, 8)) * 1;
    desktopVectorX = ("0x" + desktopVectorRecord.substr(8, 8)) * 1;
    desktopVectorY = ("0x" + desktopVectorRecord.substr(16, 8)) * 1;
    desktopVectorCX = ("0x" + desktopVectorRecord.substr(24, 8)) * 1;
    desktopVectorCY = ("0x" + desktopVectorRecord.substr(32, 8)) * 1;
    if (Math.max(0, desktopVectorVerb - 4) + Math.max(0, desktopVectorX - 1000) + Math.max(0, desktopVectorY - 600)
        + Math.max(0, desktopVectorCX - 1000) + Math.max(0, desktopVectorCY - 600)
        + !desktopVectorStartedPath * desktopVectorVerb) {
        desktopVectorFailed = true;
    } else {
        if (!desktopVectorVerb) {
            desktopVectorStartX = desktopVectorX;
            desktopVectorStartY = desktopVectorY;
            desktopVectorStartedPath = true;
            desktopPaint = desktopVectorClip.moveTo(desktopVectorX, desktopVectorY);
        }
        if (!(desktopVectorVerb - 1)) {
            desktopPaint = desktopVectorClip.lineTo(desktopVectorX, desktopVectorY);
        }
        if (!(desktopVectorVerb - 2)) {
            desktopPaint = desktopVectorClip.curveTo(desktopVectorCX, desktopVectorCY, desktopVectorX, desktopVectorY);
        }
        if (!(desktopVectorVerb - 3)) {
            desktopPaint = desktopVectorClip.lineTo(desktopVectorStartX, desktopVectorStartY);
        }
        if (!(desktopVectorVerb - 4)) {
            if (desktopVectorStride - 56) {
                desktopVectorFailed = true;
            } else {
                desktopPaintedCubic = _root.paintDesktopVectorCubic();
            }
        }
        desktopVectorLastX = desktopVectorX;
        desktopVectorLastY = desktopVectorY;
        desktopVectorNodes = desktopVectorNodes - 1;
        if (!desktopVectorNodes) {
            if (desktopVectorFill) {
                desktopPaint = desktopVectorClip.endFill();
            }
            desktopVectorShapes = desktopVectorShapes - 1;
            desktopVectorStep = 3;
            desktopVectorNeed = 0;
            if (desktopVectorShapes) {
                desktopVectorNeed = 32;
            }
        }
    }
}
