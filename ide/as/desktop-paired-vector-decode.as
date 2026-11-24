// Copyright (c) 2026 bhgv
// EN: Set up a private, incrementally decoded geometry stream for one pane.
// RU: Подготовить поэтапное декодирование отдельного потока геометрии панели.


// EN: Read one header or delegate a shape/node record, with an exact next size.
// RU: Прочитать заголовок или передать запись контура, выставив точный следующий размер.
function decodeDesktopVectorRecord() {
    if (!(desktopVectorStep - 0)) {
        desktopVectorFlags = ("0x" + desktopVectorRecord.substr(0, 8)) * 1;
        if (!desktopVectorFlags + Math.max(0, desktopVectorFlags - 196616)) {
            desktopVectorFailed = true;
        }
        desktopVectorFlags = Math.floor(desktopVectorFlags / 65536);
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
            desktopVectorStep = 2;
            desktopVectorNeed = 8;
        } else {
            if (!(desktopVectorStep - 2)) {
                desktopVectorShapes = ("0x" + desktopVectorRecord) * 1;
                if (Math.max(0, desktopVectorShapes - 32)) {
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
