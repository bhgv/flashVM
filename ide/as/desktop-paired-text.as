// Copyright (c) 2026 bhgv
// EN: Place copied Unicode labels in standard TextFields owned by the paired scene.
// RU: Размещать копии подписей Unicode в штатных TextField парной сцены.

// EN: Split a pane's first line into its caption. Keep the rest as one counted body.
// RU: Отделить первую строку панели для заголовка, сохраняя оставшийся текст целиком.
function drawDesktopText() {
    desktopLabelText = desktopWire.substr(125);
    desktopCaret = ("0x" + desktopWire.substr(101, 8)) * 1;
    desktopAnchor = ("0x" + desktopWire.substr(117, 8)) * 1;
    desktopHasCaret = !(desktopSign.indexOf(desktopWire.substr(101, 1)) + 1);
    desktopHasLabel = desktopLabelText;
    if (desktopHasCaret) {
        desktopHasLabel = true;
    }
    if (desktopHasLabel) {
        desktopLabelY = 2;
        desktopLabelX = 4;
        desktopLabelW = desktopW - 8;
        desktopLabelH = desktopH - 4;
        if (desktopBand) {
            desktopBreak = desktopLabelText.indexOf("\n");
            desktopCaptionText = desktopLabelText;
            desktopLabelText = "";
            if (desktopBreak + 1) {
                desktopLabelText = desktopCaptionText.substr(desktopBreak + 1);
                desktopCaptionText = desktopCaptionText.substr(0, desktopBreak);
            }
            desktopCaption = desktopPane.createTextField("caption", 1, desktopLabelX,
                desktopLabelY, desktopLabelW, desktopHeaderHeight - 4);
            desktopCaptionFormat = desktopCaption.getNewTextFormat();
            desktopCaptionFormat.size = 11;
            desktopCaptionFormat.color = 16185078;
            desktopTextResult = desktopCaption.setNewTextFormat(desktopCaptionFormat);
            desktopCaption.text = desktopCaptionText;
            desktopPrefix = desktopCaption.getScalarCount() + 1;
            desktopCaret = desktopCaret - desktopPrefix;
            desktopAnchor = desktopAnchor - desktopPrefix;
            desktopLabelY = desktopLabelY + desktopHeaderHeight;
            desktopLabelH = desktopLabelH - desktopHeaderHeight;
        }
        desktopHasLabel = desktopLabelText;
        if (desktopHasCaret) {
            desktopHasLabel = true;
        }
        if (desktopHasLabel) {
            desktopLabel = desktopPane.createTextField("body", 2, desktopLabelX,
                desktopLabelY, desktopLabelW, desktopLabelH);
            desktopLabelFormat = desktopLabel.getNewTextFormat();
            desktopLabelFormat.size = 11;
            desktopLabelFormat.color = 2107702;
            if (desktopActiveTab) {
                desktopLabelFormat.color = 16185078;
            }
            desktopTextResult = desktopLabel.setNewTextFormat(desktopLabelFormat);
            desktopLabel.multiline = true;
            desktopLabel.wordWrap = true;
            desktopLabel.text = desktopLabelText;
            if (desktopHasCaret) {
                if (desktopSign.indexOf(desktopWire.substr(117, 1)) + 1) {
                    desktopAnchor = desktopCaret;
                }
                desktopTextResult = desktopLabel.setSelectionPreview(desktopAnchor, desktopCaret, 13426431, 2315690);
            }
        }
    }
}
