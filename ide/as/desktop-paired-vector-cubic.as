// Copyright (c) 2026 bhgv
// EN: Draw checked, copied path commands in the staged vector MovieClip.
// RU: Нарисовать проверенные команды контуров в подготовленном MovieClip.


// EN: Approximate an SVG cubic using points on the same curve in page units.
// RU: Приблизить кубическую кривую SVG точками на ней в единицах страницы.
function paintDesktopVectorCubic() {
    desktopCubicX = ("0x" + desktopVectorRecord.substr(40, 8)) * 1;
    desktopCubicY = ("0x" + desktopVectorRecord.substr(48, 8)) * 1;
    desktopCubicT = 1;
    if (Math.max(0, desktopCubicX - 1000) + Math.max(0, desktopCubicY - 600)) {
        desktopVectorFailed = true;
        desktopCubicT = 17;
    }
    while (Math.max(0, 17 - desktopCubicT)) {
        desktopCubicU = desktopCubicT / 16;
        desktopCubicV = 1 - desktopCubicU;
        desktopCubicA = desktopCubicV * desktopCubicV * desktopCubicV;
        desktopCubicB = 3 * desktopCubicV * desktopCubicV * desktopCubicU;
        desktopCubicC = 3 * desktopCubicV * desktopCubicU * desktopCubicU;
        desktopCubicD = desktopCubicU * desktopCubicU * desktopCubicU;
        desktopPaint = desktopVectorClip.lineTo(
            desktopCubicA * desktopVectorLastX + desktopCubicB * desktopVectorCX
            + desktopCubicC * desktopCubicX + desktopCubicD * desktopVectorX,
            desktopCubicA * desktopVectorLastY + desktopCubicB * desktopVectorCY
            + desktopCubicC * desktopCubicY + desktopCubicD * desktopVectorY);
        desktopCubicT = desktopCubicT + 1;
    }
}
