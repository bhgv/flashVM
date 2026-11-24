// Copyright (c) 2026 bhgv
// EN: Index source clips by document identity inside the current staged scene.
// RU: Индексировать исходные клипы по документу внутри подготовленного снимка.


// EN: Store parent, paint depth and local extent for shelf and overview mirrors.
// RU: Запомнить родителя, глубину и локальный размер для зеркал полки и карты.
function registerDesktopSource() {
    desktopSourceIds = ",1,2,3,4,5,6,7,8,";
    if (desktopSourceIds.indexOf("," + desktopId + ",") + 1) {
        desktopSourceOffset = (desktopId - 1) * 25;
        desktopWord = desktopSeen;
        desktopSourceEncoded = _root.encodeDesktopWord();
        desktopSourceDepthWord = desktopHexWord;
        desktopWord = desktopW;
        desktopSourceEncoded = _root.encodeDesktopWord();
        desktopSourceWidthWord = desktopHexWord;
        desktopWord = desktopH;
        desktopSourceEncoded = _root.encodeDesktopWord();
        desktopSources = desktopSources.substr(0, desktopSourceOffset) + desktopParent.toString(16)
            + desktopSourceDepthWord + desktopSourceWidthWord + desktopHexWord
            + desktopSources.substr(desktopSourceOffset + 25);
        desktopSourcePositionOffset = (desktopId - 1) * 16;
        desktopSourcePositions = desktopSourcePositions.substr(0, desktopSourcePositionOffset)
            + desktopWire.substr(61, 16) + desktopSourcePositions.substr(desktopSourcePositionOffset + 16);
    }
}
