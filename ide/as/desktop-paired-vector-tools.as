// Copyright (c) 2026 bhgv
// EN: Paint independent zoom and reset buttons in the vector pane header.
// RU: Нарисовать отдельные кнопки масштаба и сброса в заголовке векторной панели.


// EN: Use the pane's fixed geometry and current magnifier mode.
// RU: Использовать неподвижную геометрию панели и текущее состояние лупы.
function paintDesktopVectorTools() {
    desktopVectorCaption = desktopVectorPane.getInstanceAtDepth(1);
    if (desktopVectorCaption) {
        desktopVectorCaption._width = Math.max(1, desktopVectorPaneWidth - 77);
    }
    desktopVectorButton = desktopVectorPane.createEmptyMovieClip("zoom", 9);
    desktopVectorButton._x = desktopVectorPaneWidth - 73;
    desktopVectorButton._y = 3;
    desktopVectorButtonColor = 3224633;
    if (desktopVectorMode) {
        desktopVectorButtonColor = 5865934;
    }
    desktopVectorButtonDrawn = _root.paintDesktopVectorButton();
    desktopPaint = desktopVectorButton.lineStyle(1, 16777215, 100);
    desktopPaint = desktopVectorButton.moveTo(5, 5);
    desktopPaint = desktopVectorButton.lineTo(12, 5);
    desktopPaint = desktopVectorButton.lineTo(12, 12);
    desktopPaint = desktopVectorButton.lineTo(5, 12);
    desktopPaint = desktopVectorButton.lineTo(5, 5);
    desktopPaint = desktopVectorButton.moveTo(12, 12);
    desktopPaint = desktopVectorButton.lineTo(16, 15);
    desktopVectorButton = desktopVectorPane.createEmptyMovieClip("resetZoom", 10);
    desktopVectorButton._x = desktopVectorPaneWidth - 51;
    desktopVectorButton._y = 3;
    desktopVectorButtonColor = 3224633;
    desktopVectorButtonDrawn = _root.paintDesktopVectorButton();
    desktopPaint = desktopVectorButton.lineStyle(1, 16777215, 100);
    desktopPaint = desktopVectorButton.moveTo(14, 6);
    desktopPaint = desktopVectorButton.lineTo(6, 6);
    desktopPaint = desktopVectorButton.lineTo(6, 13);
    desktopPaint = desktopVectorButton.lineTo(14, 13);
    desktopPaint = desktopVectorButton.lineTo(14, 9);
    desktopPaint = desktopVectorButton.moveTo(9, 3);
    desktopPaint = desktopVectorButton.lineTo(6, 6);
    desktopPaint = desktopVectorButton.lineTo(9, 9);
}
