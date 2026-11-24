// Copyright (c) 2026 bhgv
// EN: Paint one visible desktop item on its actual MovieClip owner.
// RU: Нарисовать видимый элемент в его настоящем родителе MovieClip.


// EN: Apply the wire style and publish geometry/text to the staged layer or group.
// RU: Применить стиль и добавить геометрию и текст в подготовленный слой или группу.
function composeDesktopItem() {
    desktopFill = 15133941; // Pale paper, RGB #e6ecf5.
    desktopHeader = 4282960; // Slate pane header, RGB #415a50.
    desktopBand = false;
    desktopHeaderHeight = 24;
    desktopActiveStyles = "159d";
    desktopActiveItem = desktopActiveStyles.indexOf(desktopWire.substr(100, 1)) + 1;
    desktopGroupSet = _root.registerDesktopGroup();
    if (desktopGroup) {
        desktopBand = true;
        desktopHeaderHeight = 28;
    }
    if (!(desktopId - 10)) {
        desktopFill = 13161436; // Stage RGB #c8d3dc.
    }
    if (!(desktopId - 11)) {
        desktopFill = 3159876; // Rail RGB #303744.
    }
    if (!(desktopId - 12)) {
        desktopFill = 3159876;
        desktopRightLayer._x = desktopWidth - desktopW;
    }
    desktopPanes = ",1,2,3,4,5,6,7,8,";
    if (desktopPanes.indexOf("," + desktopId + ",") + 1) {
        desktopBand = true;
        if (desktopActiveItem) {
            desktopHeader = 2448068; // Active blue, RGB #255ac4.
        }
    }
    desktopTabs = ",30,31,32,33,34,35,36,37,";
    desktopActiveTab = false;
    if (desktopTabs.indexOf("," + desktopId + ",") + 1) {
        if (desktopActiveItem) {
            desktopFill = 2448068;
            desktopActiveTab = true;
        }
    }
    desktopPlaced = _root.placeDesktopItem();
    if (desktopOwner) {
        desktopPane = desktopOwner.createEmptyMovieClip("item" + desktopId, desktopSeen);
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
            desktopPaint = desktopPane.lineTo(desktopW, desktopHeaderHeight);
            desktopPaint = desktopPane.lineTo(0, desktopHeaderHeight);
            desktopPaint = desktopPane.lineTo(0, 0);
            desktopPaint = desktopPane.endFill();
        }
        if (TextField) {
            desktopTextDrawn = _root.drawDesktopText();
        }
    }
}
