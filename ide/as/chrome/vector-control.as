// Native adapters supply copied state and a same-context drawing capability.
// Palette colour comes from the Limbo document. Selection uses a blue border;
// the controls retain the rounded MovieClip geometry of the IDE chrome.
ideDraw.w = chromeWidth;
ideDraw.h = chromeHeight;
ideDraw.color = vectorControlColor;
vectorBorderDelta = 2524379 - 8886696;
ideDraw.border = 8886696 + chromeSelected * vectorBorderDelta;
ideDraw.box = true;

// Glazed upper face matches the rail controls while retaining the chosen colour.
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
