// The native adapter supplies same-context MovieClip/TextField capabilities only.
// Dis owns window/document processes and sends copied geometry/text through channels.
// These assignments execute in FlashVM from the linked Coco/R AS -> FIR -> SWF product.
ideTable._y = ideTableOffset;
ideRight._x = ideRightX;
ideWidget._x = ideWidgetX;
ideWidget._y = ideWidgetY;
ideWidget._xscale = ideWidgetScale;
ideWidget._yscale = ideWidgetScale;
ideLabel.multiline = true;
ideLabel.wordWrap = true;
ideLabel._x = 6;
ideLabel._y = 6;
ideLabel._width = ideLabelWidth;
ideLabel._height = ideLabelHeight;
// Configure geometry before tabbed text triggers eager layout validation.
ideLabel.text = ideLabelText;
