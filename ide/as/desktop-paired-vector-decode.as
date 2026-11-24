// Copyright (c) 2026 bhgv
// EN: Set up a private, incrementally decoded geometry stream for one pane.
// RU: Подготовить поэтапное декодирование отдельного потока геометрии панели.


// EN: Read one header or delegate a shape/node record, with an exact next size.
// RU: Прочитать заголовок или передать запись контура, выставив точный следующий размер.
function decodeDesktopVectorRecord() {
    if (!(desktopVectorStep - 0)) {
        desktopVectorHeader = ("0x" + desktopVectorRecord.substr(0, 8)) * 1;
        desktopVectorFlags = Math.floor(desktopVectorHeader / 65536);
        desktopVectorTool = desktopVectorHeader - 65536 * desktopVectorFlags;
        desktopVectorInkStyle = desktopVectorRevision + desktopVectorRecord.substr(8, 16) + desktopVectorTool;
        if (!desktopVectorHeader) {
            desktopVectorFailed = true;
        }
        if (!desktopVectorTool + Math.max(0, desktopVectorTool - 8) + Math.max(0, desktopVectorFlags - 3)) {
            desktopVectorFailed = true;
        }
        desktopVectorStride = 40 + Math.floor(desktopVectorFlags / 2) * 16;
        if (desktopVectorFlags - 2 * Math.floor(desktopVectorFlags / 2)) {
            desktopVectorStep = 1;
            desktopVectorNeed = 24;
        } else {
            desktopVectorStep = 2;
            desktopVectorNeed = 8;
        }
    } else {
        if (!(desktopVectorStep - 1)) {
            desktopVectorCamera = desktopVectorRecord;
            desktopVectorZoom = ("0x" + desktopVectorCamera.substr(0, 8)) * 1;
            desktopVectorCenterX = ("0x" + desktopVectorCamera.substr(8, 8)) * 1;
            desktopVectorCenterY = ("0x" + desktopVectorCamera.substr(16, 8)) * 1;
            if (!((desktopVectorCenterX + 1) * (desktopVectorCenterY + 1) * desktopVectorZoom)) {
                desktopVectorFailed = true;
            }
            if (Math.max(0, 25 - desktopVectorZoom) + Math.max(0, desktopVectorZoom - 1600)
                + Math.max(0, desktopVectorCenterX - 1000) + Math.max(0, desktopVectorCenterY - 600)) {
                desktopVectorFailed = true;
            }
            desktopVectorStep = 2;
            desktopVectorNeed = 8;
        } else {
            if (!(desktopVectorStep - 2)) {
                desktopVectorShapes = ("0x" + desktopVectorRecord) * 1;
                if (Math.max(0, desktopVectorShapes - 32 - !(desktopVectorTool - 8))) {
                    desktopVectorFailed = true;
                }
                desktopVectorStep = 3;
                desktopVectorNeed = 0;
                if (desktopVectorShapes) {
                    desktopVectorNeed = 32;
                }
            } else {
                if (!(desktopVectorStep - 3)) {
                    desktopVectorDecodedPath = _root.beginDesktopVectorPath();
                } else {
                    desktopVectorDecodedNode = _root.paintDesktopVectorNode();
                }
            }
        }
    }
}
