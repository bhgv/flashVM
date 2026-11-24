// Copyright (c) 2026 bhgv
// EN: Draw checked, copied path commands in the staged vector MovieClip.
// RU: Нарисовать проверенные команды контуров в подготовленном MovieClip.


// EN: Begin a shape with its own fill and stroke, then consume its node count.
// RU: Начать контур с собственной заливкой и обводкой и принять число узлов.
function beginDesktopVectorPath() {
    desktopVectorColor = ("0x" + desktopVectorRecord.substr(0, 8)) * 1;
    desktopVectorWidth = ("0x" + desktopVectorRecord.substr(8, 8)) * 1;
    desktopVectorFill = ("0x" + desktopVectorRecord.substr(16, 8)) * 1;
    desktopVectorNodes = ("0x" + desktopVectorRecord.substr(24, 8)) * 1;
    desktopVectorStartedPath = false;
    if (!desktopVectorNodes + Math.max(0, desktopVectorNodes - 4096) + Math.max(0, desktopVectorColor - 16777215)
        + Math.max(0, desktopVectorWidth - 256) + Math.max(0, desktopVectorFill - 1)) {
        desktopVectorFailed = true;
    } else {
        if (desktopVectorFill) {
            desktopPaint = desktopVectorClip.lineStyle();
            desktopPaint = desktopVectorClip.beginFill(desktopVectorColor);
        } else {
            desktopPaint = desktopVectorClip.lineStyle(desktopVectorWidth, desktopVectorColor, 100);
        }
        desktopVectorStep = 4;
        desktopVectorNeed = desktopVectorStride;
    }
}
