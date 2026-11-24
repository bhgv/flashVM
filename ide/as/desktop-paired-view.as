// Copyright (c) 2026 bhgv
// EN: Present committed desktop rectangle records on the paired FlashVM stage.
// RU: Отображать подтверждённые прямоугольники desktop на сцене парного FlashVM.

// EN: Read one bounded terminal record per tick. Paint only on the FlashVM owner.
// RU: Принимать одну запись за кадр и рисовать в потоке владельца FlashVM.
function onEnterFrame() {
    if (!desktopFrontDepth) {
        desktopFrontDepth = 10;
        desktopSign = "89abcdef";
    }
    if (desktopOutgoing) {
        desktopAccepted = styxTerminal.trySendFrame(desktopOutgoing);
        if (desktopAccepted) {
            desktopOutgoing = "";
        }
    } else {
        desktopWire = styxTerminal.tryReceiveFrame();
        if (desktopWire) {
            if (!desktopWire.indexOf("fvd1:000000070000000000010021")) {
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
                desktopSeen = 0;
                desktopOpen = desktopTicket;
            }
            if (!desktopWire.indexOf("fvd1:000000070000000000010022")) {
                if (desktopOpen) {
                    desktopRecordTicket = desktopWire.substr(29, 8);
                    if (!desktopRecordTicket.indexOf(desktopTicket)) {
                        desktopSeen = desktopSeen + 1;
                        desktopPainted = _root.drawDesktopItem();
                    }
                }
            }
            if (!desktopWire.indexOf("fvd1:000000070000000000010023")) {
                if (desktopOpen) {
                    desktopRecordTicket = desktopWire.substr(29, 8);
                    if (!desktopRecordTicket.indexOf(desktopTicket)) {
                        if (desktopExpected) {
                            if (!(desktopSeen - desktopExpected)) {
                                if (desktopFront) {
                                    desktopFront._visible = false;
                                }
                                desktopFront = desktopPending;
                                desktopFrontDepth = desktopDepth;
                                desktopFront._visible = true;
                            }
                        }
                        desktopOpen = false;
                    }
                }
            }
            if (onDesktopRecord) {
                desktopObserved = _root.onDesktopRecord();
            }
        }
    }
}
