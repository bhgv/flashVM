// Copyright (c) 2026 bhgv
// EN: Commit complete Dis scenes between transport dispatches.
// RU: Публиковать полные снимки сцены Dis между обработками транспорта.


// EN: Handle one credited record or retry one staged send on the FlashVM owner.
// RU: Обработать одну запись с кредитом или повторить отправку у владельца FlashVM.
function onTerminalFrame() {
    if (desktopOutgoing) {
        desktopAccepted = styxTerminal.trySendFrame(desktopOutgoing);
        if (desktopAccepted) {
            desktopOutgoing = "";
            desktopReady = false;
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
                desktopStagedOrder = "";
                desktopOpen = desktopTicket;
            }
            if (!desktopWire.indexOf("fvd1:000000070000000000010022")) {
                if (desktopOpen) {
                    desktopRecordTicket = desktopWire.substr(29, 8);
                    if (!desktopRecordTicket.indexOf(desktopTicket)) {
                        desktopSeen = desktopSeen + 1;
                        desktopStagedOrder = desktopStagedOrder + desktopWire.substr(37, 8);
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
                                desktopFrontOrder = desktopStagedOrder;
                                desktopFrontTicket = desktopTicket;
                                desktopReady = true;
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
    if (desktopReady) {
        if (desktopInputEnabled) {
            if (!desktopOutgoing) {
                desktopPolled = _root.pollDesktopInput();
            }
        }
    }
}
