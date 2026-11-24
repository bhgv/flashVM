// Copyright (c) 2026 bhgv
// EN: Set up a private, incrementally decoded geometry stream for one pane.
// RU: Подготовить поэтапное декодирование отдельного потока геометрии панели.


// EN: Bind the declaration to the staged pane, ticket and existing wire schema.
// RU: Привязать объявление к панели снимка, билету и действующему формату.
function beginDesktopVector() {
    if (desktopOpen) {
        desktopVectorTicket = desktopWire.substr(29, 8);
        desktopVectorSlot = desktopWire.substr(37, 8);
        desktopVectorTotal = ("0x" + desktopWire.substr(45, 8)) * 1;
        desktopMirror = ("0x" + desktopVectorSlot) * 1;
        desktopId = desktopMirror;
        desktopVectorResolved = _root.resolveDesktopMirror();
        desktopVectorVersion = desktopWire.substr(61);
        desktopVectorKey = "|" + desktopVectorSlot + "|";
        if (Math.abs(desktopVectorTicket.indexOf(desktopTicket)) + desktopVectorRemaining + !desktopMirrorSource
            + Math.abs(desktopVectorVersion.indexOf("00000002"))
            + desktopVectorSlots.indexOf(desktopVectorKey) + 1
            + !desktopVectorTotal + Math.max(0, 56 - desktopVectorTotal)
            + Math.max(0, desktopVectorTotal - 1230000)) {
            desktopVectorFailed = true;
        } else {
            desktopVectorSlots = desktopVectorSlots + desktopVectorKey;
            desktopVectorPane = desktopMirrorSource;
            desktopVectorPaneWidth = desktopSourceWidth;
            desktopVectorPaneHeight = desktopSourceHeight;
            desktopVectorViewport = desktopVectorPane.createEmptyMovieClip("drawing", 8);
            desktopVectorClip = desktopVectorViewport.createEmptyMovieClip("paths", 1);
            desktopVectorCamera = "00000064000001f40000012c";
            desktopVectorZoom = 100;
            desktopVectorCenterX = 500;
            desktopVectorCenterY = 300;
            desktopVectorRemaining = desktopVectorTotal;
            desktopVectorRead = 0;
            desktopVectorBuffer = "";
            desktopVectorBuffered = 0;
            desktopVectorStep = 0;
            desktopVectorNeed = 48;
        }
    }
}
