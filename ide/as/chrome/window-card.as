// Rounded paper card, active outline and inactive grey rim.
// Executed AS -> FIR -> SWO -> SWF actions. ideDraw is an owner-local generic
// drawing capability; Dis owns geometry/selection and no heap pointer crosses it.
chromeBorderDelta = 2524379 - 12174538;
ideDraw.w = chromeWidth;
ideDraw.h = chromeHeight;
ideDraw.color = 15725297;
ideDraw.border = 12174538 + chromeActive * chromeBorderDelta;
ideDraw.box = true;
