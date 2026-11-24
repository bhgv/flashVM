// Copyright (c) 2026 bhgv
// EN: Show the initial brush nib as soon as the pointer is pressed.
// RU: Показать первый отпечаток кисти сразу при нажатии.


// EN: Paint a round dot at the clamped viewport point, using eight short quadratics.
// RU: Нарисовать круг на ограниченной точке холста восемью короткими квадратичными кривыми.
function paintDesktopInkDot() {
    desktopInkDiag = desktopInkRadius * 707 / 1000;
    desktopInkTangent = desktopInkRadius * 414 / 1000;
    desktopInkDrawn = desktopInkClip.beginFill(desktopInkColor);
    desktopInkDrawn = desktopInkClip.moveTo(desktopInkX + desktopInkRadius, desktopInkY);
    desktopInkDrawn = desktopInkClip.curveTo(desktopInkX + desktopInkRadius, desktopInkY + desktopInkTangent,
        desktopInkX + desktopInkDiag, desktopInkY + desktopInkDiag);
    desktopInkDrawn = desktopInkClip.curveTo(desktopInkX + desktopInkTangent, desktopInkY + desktopInkRadius,
        desktopInkX, desktopInkY + desktopInkRadius);
    desktopInkDrawn = desktopInkClip.curveTo(desktopInkX - desktopInkTangent, desktopInkY + desktopInkRadius,
        desktopInkX - desktopInkDiag, desktopInkY + desktopInkDiag);
    desktopInkDrawn = desktopInkClip.curveTo(desktopInkX - desktopInkRadius, desktopInkY + desktopInkTangent,
        desktopInkX - desktopInkRadius, desktopInkY);
    desktopInkDrawn = desktopInkClip.curveTo(desktopInkX - desktopInkRadius, desktopInkY - desktopInkTangent,
        desktopInkX - desktopInkDiag, desktopInkY - desktopInkDiag);
    desktopInkDrawn = desktopInkClip.curveTo(desktopInkX - desktopInkTangent, desktopInkY - desktopInkRadius,
        desktopInkX, desktopInkY - desktopInkRadius);
    desktopInkDrawn = desktopInkClip.curveTo(desktopInkX + desktopInkTangent, desktopInkY - desktopInkRadius,
        desktopInkX + desktopInkDiag, desktopInkY - desktopInkDiag);
    desktopInkDrawn = desktopInkClip.curveTo(desktopInkX + desktopInkRadius, desktopInkY - desktopInkTangent,
        desktopInkX + desktopInkRadius, desktopInkY);
    desktopInkDrawn = desktopInkClip.endFill();
}
