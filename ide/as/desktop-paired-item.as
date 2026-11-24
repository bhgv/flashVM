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
    desktopMirror = ("0x" + desktopWire.substr(109, 8)) * 1;
    if (desktopSign.indexOf(desktopWire.substr(61, 1)) + 1) {
        desktopX = desktopX - 65536 * 65536;
    }
    if (desktopSign.indexOf(desktopWire.substr(69, 1)) + 1) {
        desktopY = desktopY - 65536 * 65536;
    }
    if (desktopMirror) {
        desktopComposed = _root.drawDesktopMirror();
    } else {
        if (!(desktopId - 62)) {
            desktopComposed = _root.drawDesktopAtlasViewport();
        } else {
            desktopItemVisible = desktopX - 65536;
            desktopItemDrawable = desktopItemVisible;
            if (!desktopItemVisible) {
                // Hidden documents still own a source clip for the shelf.
                // Скрытый документ сохраняет исходный клип для полки.
                desktopSourceIds = ",1,2,3,4,5,6,7,8,";
                desktopItemDrawable = desktopSourceIds.indexOf("," + desktopId + ",") + 1;
                desktopX = 0;
                desktopY = 0;
            }
            if (desktopItemDrawable) {
                if (desktopItemVisible) {
                    if (!(desktopParent - 1)) {
                        desktopCulled = _root.cullDesktopTableItem();
                    }
                }
                desktopComposed = _root.composeDesktopItem();
            }
        }
    }
}
