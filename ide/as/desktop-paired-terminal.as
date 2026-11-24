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
                desktopBegan = _root.beginDesktopScene();
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
                desktopEnded = _root.endDesktopScene();
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
