// Copyright (c) 2026 bhgv
// EN: Publish the matched scene, its hit map and its acknowledged dimensions.
// RU: Опубликовать согласованный снимок, карту попаданий и подтверждённые размеры.


// EN: Commit one EndView with matching ticket and descriptor count.
// RU: Принять EndView с совпадающими билетом и числом записей.
function endDesktopScene() {
    if (desktopOpen) {
        desktopRecordTicket = desktopWire.substr(29, 8);
        if (!desktopRecordTicket.indexOf(desktopTicket)) {
            if (desktopExpected) {
                desktopVectorExpected = ("0x" + desktopWire.substr(37, 8)) * 1;
                if (!(desktopSeen - desktopExpected) * !desktopVectorFailed * !desktopVectorRemaining
                    * !(desktopVectorCount - desktopVectorExpected)) {
                    if (desktopFront) {
                        desktopFront._visible = false;
                    }
                    desktopFront = desktopPending;
                    desktopFrontDepth = desktopDepth;
                    desktopFront._visible = true;
                    desktopFrontOrder = desktopStagedOrder;
                    desktopFrontTicket = desktopTicket;
                    desktopFrontWidth = desktopWidth;
                    desktopFrontHeight = desktopHeight;
                    desktopFrontSources = desktopSources;
                    desktopFrontSourcePositions = desktopSourcePositions;
                    desktopFrontGroups = desktopStagedGroups;
                    desktopFrontGroupPositions = desktopGroupPositions;
                    desktopFrontTableX = desktopTableX;
                    desktopFrontTableY = desktopTableY;
                    desktopFrontTableScale = desktopScale;
                    desktopFrontVectorCameras = desktopStagedVectorCameras;
                    desktopFrontVectorSlots = desktopVectorSlots;
                    desktopReady = true;
                    if (desktopResizePending) {
                        if (Math.abs(desktopResizeWidth - desktopWidth) + Math.abs(desktopResizeHeight - desktopHeight)) {
                            desktopResizeSent = _root.sendDesktopResize();
                        } else {
                            desktopResizeCommitted = hostInput.commitViewport(desktopWidth, desktopHeight);
                            desktopResizePending = false;
                        }
                    }
                    if (desktopInputEnabled) {
                        desktopPresentation = desktopPresentation + 1;
                        desktopPresented = hostInput.publishFrame(desktopPresentation);
                    }
                }
            }
            desktopOpen = false;
        }
    }
}
