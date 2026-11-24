// Copyright (c) 2026 bhgv
// EN: Set up a private, incrementally decoded geometry stream for one pane.
// RU: Подготовить поэтапное декодирование отдельного потока геометрии панели.


// EN: Consume one consecutive fragment. Retain only an incomplete record;
// the complete staged MovieClip is held by the scene until EndView.
// RU: Принять очередной фрагмент, сохранив только остаток неполной записи.
// Готовый MovieClip остаётся частью скрытой сцены до EndView.
function receiveDesktopVector() {
    if (!(!desktopOpen) * !desktopVectorFailed) {
        desktopVectorChunk = desktopWire.substr(53);
        desktopVectorBytes = 0;
        desktopVectorProbe = 1024;
        while (desktopVectorProbe) {
            if (desktopVectorChunk.substr(desktopVectorBytes + desktopVectorProbe - 1, 1)) {
                desktopVectorBytes = desktopVectorBytes + desktopVectorProbe;
            }
            desktopVectorProbe = Math.floor(desktopVectorProbe / 2);
        }
        desktopVectorOffset = ("0x" + desktopWire.substr(45, 8)) * 1;
        desktopVectorFragmentTicket = desktopWire.substr(29, 8);
        desktopVectorFragmentSlot = desktopWire.substr(37, 8);
        if (Math.abs(desktopVectorFragmentTicket.indexOf(desktopTicket))
            + Math.abs(desktopVectorFragmentSlot.indexOf(desktopVectorSlot))
            + Math.abs(desktopVectorOffset - desktopVectorRead) + !desktopVectorBytes
            + Math.max(0, desktopVectorBytes - desktopVectorRemaining)) {
            desktopVectorFailed = true;
        } else {
            desktopVectorRead = desktopVectorRead + desktopVectorBytes;
            desktopVectorRemaining = desktopVectorRemaining - desktopVectorBytes;
            desktopVectorBuffer = desktopVectorBuffer + desktopVectorChunk;
            desktopVectorBuffered = desktopVectorBuffered + desktopVectorBytes;
            while (!desktopVectorFailed * desktopVectorNeed * !Math.max(0, desktopVectorNeed - desktopVectorBuffered)) {
                desktopVectorRecord = desktopVectorBuffer.substr(0, desktopVectorNeed);
                desktopVectorBuffer = desktopVectorBuffer.substr(desktopVectorNeed);
                desktopVectorBuffered = desktopVectorBuffered - desktopVectorNeed;
                desktopVectorDecoded = _root.decodeDesktopVectorRecord();
            }
            if (!desktopVectorRemaining) {
                if (desktopVectorNeed + desktopVectorBuffered) {
                    desktopVectorFailed = true;
                } else {
                    desktopVectorCount = desktopVectorCount + 1;
                }
            }
        }
    }
}
