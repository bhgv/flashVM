// Copyright (c) 2026 bhgv
// EN: Convert one staged desktop descriptor to standard MovieClip drawing operations.
// RU: Преобразовать один дескриптор desktop в обычные операции рисования MovieClip.

// EN: Paint the rectangular geometry in record order. The renderer owns all clips.
// RU: Рисовать геометрию прямоугольников в порядке записей. Клипы принадлежат рендереру.
function drawDesktopItem() {
    desktopId = ("0x" + desktopWire.substr(37, 8)) * 1;
    desktopParent = ("0x" + desktopWire.substr(53, 8)) * 1;
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
    // Dis places hidden panes outside the legal raster coordinate domain.
    // Dis помечает скрытые панели координатами вне диапазона растера.
    if (desktopX - 65536) {
        desktopComposed = _root.composeDesktopItem();
    }
}
