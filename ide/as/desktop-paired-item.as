// Copyright (c) 2026 bhgv
// EN: Convert one staged desktop descriptor to standard MovieClip drawing operations.
// RU: Преобразовать один дескриптор desktop в обычные операции рисования MovieClip.

// EN: Paint the rectangular geometry in record order. The renderer owns all clips.
// RU: Рисовать геометрию прямоугольников в порядке записей. Клипы принадлежат рендереру.
function drawDesktopItem() {
    desktopId = ("0x" + desktopWire.substr(37, 8)) * 1;
    desktopParent = ("0x" + desktopWire.substr(53, 8)) * 1;
    desktopFlatLayers = ",0,1,2,3,4,";
    desktopX = ("0x" + desktopWire.substr(61, 8)) * 1;
    desktopY = ("0x" + desktopWire.substr(69, 8)) * 1;
    desktopW = ("0x" + desktopWire.substr(77, 8)) * 1;
    desktopH = ("0x" + desktopWire.substr(85, 8)) * 1;
    if (desktopSign.indexOf(desktopWire.substr(61, 1)) + 1) {
        desktopX = desktopX - 65536 * 65536;
    }
    if (desktopSign.indexOf(desktopWire.substr(69, 1)) + 1) {
        desktopY = desktopY - 65536 * 65536;
    }
    desktopFill = 15133941; // Pale paper, RGB #e6ecf5.
    desktopHeader = 4282960; // Slate pane header, RGB #415a50.
    desktopBand = false;
    if (!(desktopParent - 1)) {
        desktopX = desktopX * desktopScale + desktopPan;
        desktopY = (desktopY - desktopScroll) * desktopScale;
        desktopW = desktopW * desktopScale;
        desktopH = desktopH * desktopScale;
    }
    if (!(desktopParent - 3)) {
        // The right rail has the copied width of item 12.
        // Правая полка имеет ширину из записи 12.
        if (!(desktopId - 12)) {
            desktopRailWidth = desktopW;
        }
        desktopX = desktopX + desktopWidth - desktopRailWidth;
    }
    if (!(desktopId - 10)) {
        desktopFill = 13161436; // Stage RGB #c8d3dc.
    }
    if (!(desktopId - 11)) {
        desktopFill = 3159876; // Rail RGB #303744.
    }
    if (!(desktopId - 12)) {
        desktopFill = 3159876;
    }
    desktopPanes = ",1,2,3,4,5,6,7,8,";
    if (desktopPanes.indexOf("," + desktopId + ",") + 1) {
        desktopBand = true;
        desktopActiveStyles = "159d";
        if (desktopActiveStyles.indexOf(desktopWire.substr(100, 1)) + 1) {
            desktopHeader = 2448068; // Active blue, RGB #255ac4.
        }
    }
    if (desktopFlatLayers.indexOf("," + desktopParent + ",") + 1) {
        desktopPane = desktopPending.createEmptyMovieClip("item" + desktopId, desktopSeen);
        desktopPane._x = desktopX;
        desktopPane._y = desktopY;
        desktopPaint = desktopPane.beginFill(desktopFill);
        desktopPaint = desktopPane.moveTo(0, 0);
        desktopPaint = desktopPane.lineTo(desktopW, 0);
        desktopPaint = desktopPane.lineTo(desktopW, desktopH);
        desktopPaint = desktopPane.lineTo(0, desktopH);
        desktopPaint = desktopPane.lineTo(0, 0);
        desktopPaint = desktopPane.endFill();
        if (desktopBand) {
            desktopPaint = desktopPane.beginFill(desktopHeader);
            desktopPaint = desktopPane.moveTo(0, 0);
            desktopPaint = desktopPane.lineTo(desktopW, 0);
            desktopPaint = desktopPane.lineTo(desktopW, 24);
            desktopPaint = desktopPane.lineTo(0, 24);
            desktopPaint = desktopPane.lineTo(0, 0);
            desktopPaint = desktopPane.endFill();
        }
    }
}
