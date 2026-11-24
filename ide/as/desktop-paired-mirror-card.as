// Copyright (c) 2026 bhgv
// EN: Decorate the card and the optional readable left-shelf caption.
// RU: Нарисовать карточку и при необходимости читаемый заголовок на левой полке.


// EN: Fill the visible target; cap the title inside a complete shelf header.
// RU: Залить видимую область и ограничить подпись целой полосой заголовка.
function paintDesktopMirrorCard() {
    desktopMirrorFill = 4274800;
    desktopMirrorStyle = desktopWire.substr(100, 1);
    if (!desktopMirrorStyle.indexOf("1")) {
        desktopMirrorFill = 2448068;
    }
    if (desktopShelfMirror) {
        desktopPaint = desktopPane.beginFill(desktopMirrorFill);
        desktopPaint = desktopPane.moveTo(0, 0);
        desktopPaint = desktopPane.lineTo(desktopW, 0);
        desktopPaint = desktopPane.lineTo(desktopW, desktopH);
        desktopPaint = desktopPane.lineTo(0, desktopH);
        desktopPaint = desktopPane.lineTo(0, 0);
        desktopPaint = desktopPane.endFill();
    }
    if (desktopShelfTitle) {
        if (TextField) {
            desktopMirrorTitle = desktopPane.createTextField("title", 2, 3, 2, desktopW - 6, 16);
            desktopMirrorFormat = desktopMirrorTitle.getNewTextFormat();
            desktopMirrorFormat.size = 9;
            desktopMirrorFormat.color = 16185078;
            desktopMirrorFormatted = desktopMirrorTitle.setNewTextFormat(desktopMirrorFormat);
            desktopMirrorTitle.text = desktopWire.substr(125);
        }
    }
}
