// Copyright (c) 2026 bhgv
// EN: Draw one preview segment in fixed viewport pixels.
// RU: Нарисовать один сегмент предпросмотра в пикселях неподвижного холста.


// EN: Keep round stroke caps inside the paper. The packed point still uses
// the original logical coordinates for the authoritative Limbo contour.
// RU: Оставить круглые окончания внутри листа. Упакованная точка сохраняет
// исходные логические координаты для итогового контура Limbo.
function paintDesktopInkSample() {
    desktopInkRadius = desktopInkWidth / 2;
    desktopInkX = Math.max(desktopInkRadius, Math.min(desktopInkRight - desktopInkRadius, desktopVectorMouseX));
    desktopInkY = Math.max(desktopInkRadius, Math.min(desktopInkBottom - desktopInkRadius, desktopVectorMouseY));
    if (!(desktopInkCount - 1)) {
        desktopInkDotPainted = _root.paintDesktopInkDot();
        desktopInkStyled = desktopInkClip.lineStyle(desktopInkWidth, desktopInkColor, 100);
        desktopInkMoved = desktopInkClip.moveTo(desktopInkX, desktopInkY);
    } else {
        desktopInkDrawn = desktopInkClip.lineTo(desktopInkX, desktopInkY);
    }
    desktopInkPresented = _root.publishDesktopInk();
}
