// A scaled live source view. Keep its narrow rim within the admitted map box.
ideDraw.w = chromePreviewWidth / chromePreviewScale;
ideDraw.h = chromePreviewHeight / chromePreviewScale;
ideDraw.color = 15725297;
atlasRimDelta = 2524379 - 12174538;
ideDraw.border = 12174538 + chromeActive * atlasRimDelta;
ideDraw.box = true;
