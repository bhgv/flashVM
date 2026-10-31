// Frosted right rail; 60 percent opacity, fading toward the desk.
// Executed AS -> FIR -> SWO -> SWF actions. ideDraw is an owner-local generic
// drawing capability; Dis owns geometry/selection and no heap pointer crosses it.
ideDraw.clear = true;
// Rail strip 1: inner-edge alpha is deliberately lower.
ideDraw.x = chromeWidth * 0 / 8;
ideDraw.y = 0;
ideDraw.w = chromeWidth / 8;
ideDraw.h = chromeHeight;
ideDraw.color = 3422523;
ideDraw.alpha = 5;
ideDraw.rect = true;
// Rail strip 2: inner-edge alpha is deliberately lower.
ideDraw.x = chromeWidth * 1 / 8;
ideDraw.y = 0;
ideDraw.w = chromeWidth / 8;
ideDraw.h = chromeHeight;
ideDraw.color = 3488316;
ideDraw.alpha = 27;
ideDraw.rect = true;
// Rail strip 3: inner-edge alpha is deliberately lower.
ideDraw.x = chromeWidth * 2 / 8;
ideDraw.y = 0;
ideDraw.w = chromeWidth / 8;
ideDraw.h = chromeHeight;
ideDraw.color = 3554109;
ideDraw.alpha = 48;
ideDraw.rect = true;
// Rail strip 4: inner-edge alpha is deliberately lower.
ideDraw.x = chromeWidth * 3 / 8;
ideDraw.y = 0;
ideDraw.w = chromeWidth / 8;
ideDraw.h = chromeHeight;
ideDraw.color = 3422523;
ideDraw.alpha = 58;
ideDraw.rect = true;
// Rail strip 5: inner-edge alpha is deliberately lower.
ideDraw.x = chromeWidth * 4 / 8;
ideDraw.y = 0;
ideDraw.w = chromeWidth / 8;
ideDraw.h = chromeHeight;
ideDraw.color = 3488316;
ideDraw.alpha = 60;
ideDraw.rect = true;
// Rail strip 6: inner-edge alpha is deliberately lower.
ideDraw.x = chromeWidth * 5 / 8;
ideDraw.y = 0;
ideDraw.w = chromeWidth / 8;
ideDraw.h = chromeHeight;
ideDraw.color = 3554109;
ideDraw.alpha = 60;
ideDraw.rect = true;
// Rail strip 7: inner-edge alpha is deliberately lower.
ideDraw.x = chromeWidth * 6 / 8;
ideDraw.y = 0;
ideDraw.w = chromeWidth / 8;
ideDraw.h = chromeHeight;
ideDraw.color = 3422523;
ideDraw.alpha = 60;
ideDraw.rect = true;
// Rail strip 8: inner-edge alpha is deliberately lower.
ideDraw.x = chromeWidth * 7 / 8;
ideDraw.y = 0;
ideDraw.w = chromeWidth / 8;
ideDraw.h = chromeHeight;
ideDraw.color = 3488316;
ideDraw.alpha = 60;
ideDraw.rect = true;
