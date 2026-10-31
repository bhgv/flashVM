// Six-band blue header for desktop cards and live miniatures.
// Executed AS -> FIR -> SWO -> SWF actions. ideDraw is an owner-local generic
// drawing capability; Dis owns geometry/selection and no heap pointer crosses it.
// Use the admitted shelf bounds for a miniature, keeping its preview undistorted.
chromeHeaderDelta = chromePreviewWidth / chromePreviewScale - chromeWidth;
chromeHeaderWidth = chromeWidth + chromeMiniature * chromeHeaderDelta;
// Header highlight band 1; coordinates remain local to the card.
ideDraw.x = 3;
ideDraw.y = 2 + 0 * 22 / 6;
ideDraw.w = chromeHeaderWidth - 6;
ideDraw.h = 22 / 6;
ideDraw.color = 8824241;
ideDraw.alpha = 100;
ideDraw.rect = true;
// Header highlight band 2; coordinates remain local to the card.
ideDraw.x = 3;
ideDraw.y = 2 + 1 * 22 / 6;
ideDraw.w = chromeHeaderWidth - 6;
ideDraw.h = 22 / 6;
ideDraw.color = 9153206;
ideDraw.alpha = 100;
ideDraw.rect = true;
// Header highlight band 3; coordinates remain local to the card.
ideDraw.x = 3;
ideDraw.y = 2 + 2 * 22 / 6;
ideDraw.w = chromeHeaderWidth - 6;
ideDraw.h = 22 / 6;
ideDraw.color = 9482171;
ideDraw.alpha = 100;
ideDraw.rect = true;
// Header highlight band 4; coordinates remain local to the card.
ideDraw.x = 3;
ideDraw.y = 2 + 3 * 22 / 6;
ideDraw.w = chromeHeaderWidth - 6;
ideDraw.h = 22 / 6;
ideDraw.color = 9811136;
ideDraw.alpha = 100;
ideDraw.rect = true;
// Header highlight band 5; coordinates remain local to the card.
ideDraw.x = 3;
ideDraw.y = 2 + 4 * 22 / 6;
ideDraw.w = chromeHeaderWidth - 6;
ideDraw.h = 22 / 6;
ideDraw.color = 10140101;
ideDraw.alpha = 100;
ideDraw.rect = true;
// Header highlight band 6; coordinates remain local to the card.
ideDraw.x = 3;
ideDraw.y = 2 + 5 * 22 / 6;
ideDraw.w = chromeHeaderWidth - 6;
ideDraw.h = 22 / 6;
ideDraw.color = 10469066;
ideDraw.alpha = 100;
ideDraw.rect = true;
