// Dark glazed scroll control with a folded lower corner.
// Executed AS -> FIR -> SWO -> SWF actions. ideDraw is an owner-local generic
// drawing capability; Dis owns geometry/selection and no heap pointer crosses it.
chromeFillDelta = 16763501 - 2435886;
ideDraw.w = chromeWidth;
ideDraw.h = chromeHeight;
ideDraw.color = 2435886 + chromeSelected * chromeFillDelta;
ideDraw.border = 5068635;
ideDraw.box = true;
ideDraw.alpha = 19;
ideDraw.color = 16777215;
ideDraw.begin = true;
ideDraw.x = 2;
ideDraw.y = 2;
ideDraw.move = true;
ideDraw.x = chromeWidth - 2;
ideDraw.line = true;
ideDraw.y = chromeHeight * 27 / 100;
ideDraw.line = true;
ideDraw.x = 2;
ideDraw.y = chromeHeight * 78 / 100;
ideDraw.line = true;
ideDraw.y = 2;
ideDraw.line = true;
ideDraw.end = true;
