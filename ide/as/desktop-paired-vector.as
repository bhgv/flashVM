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
            desktopVectorClip = desktopMirrorSource.createEmptyMovieClip("drawing", 8);
            desktopVectorScale = Math.min((desktopSourceWidth - 16) / 1000, (desktopSourceHeight - 32) / 600);
            desktopVectorScale = Math.max(1 / 1000, desktopVectorScale);
            desktopVectorClip._x = (desktopSourceWidth - 1000 * desktopVectorScale) / 2;
            desktopVectorClip._y = 28;
            desktopVectorClip._xscale = desktopVectorScale * 100;
            desktopVectorClip._yscale = desktopVectorScale * 100;
            desktopPaint = desktopVectorClip.beginFill(16777215);
            desktopPaint = desktopVectorClip.moveTo(0, 0);
            desktopPaint = desktopVectorClip.lineTo(1000, 0);
            desktopPaint = desktopVectorClip.lineTo(1000, 600);
            desktopPaint = desktopVectorClip.lineTo(0, 600);
            desktopPaint = desktopVectorClip.lineTo(0, 0);
            desktopPaint = desktopVectorClip.endFill();
            desktopVectorRemaining = desktopVectorTotal;
            desktopVectorRead = 0;
            desktopVectorBuffer = "";
            desktopVectorBuffered = 0;
            desktopVectorStep = 0;
            desktopVectorNeed = 48;
        }
    }
}
