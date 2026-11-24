// Copyright (c) 2026 bhgv
// EN: Stage one coherent desktop snapshot and layer graph.
// RU: Подготовить целостный снимок desktop и его слои.


// EN: Read BeginView from the current wire record and prepare an invisible scene.
// RU: Прочитать BeginView и подготовить невидимый снимок.
function beginDesktopScene() {
    // Reuse the hidden depth. Keep the preceding scene visible during transfer.
    // Переиспользовать скрытую глубину, оставляя предыдущую сцену видимой.
    desktopDepth = 21 - desktopFrontDepth;
    desktopPending = _root.createEmptyMovieClip("desktopScene" + desktopDepth, desktopDepth);
    desktopPending._visible = false;
    desktopTicket = desktopWire.substr(53, 8);
    desktopExpected = ("0x" + desktopWire.substr(61, 8)) * 1;
    desktopScroll = ("0x" + desktopWire.substr(69, 8)) * 1;
    desktopScale = ("0x" + desktopWire.substr(77, 8)) / 100;
    desktopPan = ("0x" + desktopWire.substr(85, 8)) * 1;
    if (desktopSign.indexOf(desktopWire.substr(85, 1)) + 1) {
        desktopPan = desktopPan - 65536 * 65536;
    }
    desktopWidth = ("0x" + desktopWire.substr(37, 8)) * 1;
    desktopHeight = ("0x" + desktopWire.substr(45, 8)) * 1;
    desktopLayersSet = _root.setupDesktopLayers();
    desktopSeen = 0;
    desktopStagedOrder = "";
    desktopOpen = desktopTicket;
}
