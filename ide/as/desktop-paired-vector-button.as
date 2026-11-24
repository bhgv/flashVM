// Copyright (c) 2026 bhgv
// EN: Shared shape for the vector viewport buttons.
// RU: Общая форма кнопок области просмотра рисунка.


// EN: Paint the fixed button backdrop on the current child clip.
// RU: Нарисовать неподвижный фон кнопки в текущем дочернем клипе.
function paintDesktopVectorButton() {
    desktopPaint = desktopVectorButton.beginFill(desktopVectorButtonColor);
    desktopPaint = desktopVectorButton.moveTo(0, 0);
    desktopPaint = desktopVectorButton.lineTo(20, 0);
    desktopPaint = desktopVectorButton.lineTo(20, 18);
    desktopPaint = desktopVectorButton.lineTo(0, 18);
    desktopPaint = desktopVectorButton.lineTo(0, 0);
    desktopPaint = desktopVectorButton.endFill();
}
