// Portrait shelf card around an undistorted live preview.
// The document preview retains its original aspect ratio. Portrait card size
// stays readable; the separate Limbo rail owner scrolls overflowing cards.
ideDraw.w = chromePreviewWidth / chromePreviewScale;
ideDraw.h = chromePreviewHeight / chromePreviewScale;
ideDraw.color = 15725297;
chromeBorderDelta = 2524379 - 12174538;
ideDraw.border = 12174538 + chromeActive * chromeBorderDelta;
ideDraw.box = true;
