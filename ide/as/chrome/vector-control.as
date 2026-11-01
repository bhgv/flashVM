// Native adapters supply copied state and a same-context drawing capability.
// Palette colour comes from the Limbo document. Selection uses a blue border;
// the controls retain the rounded MovieClip geometry of the IDE chrome.
ideDraw.w = chromeWidth;
ideDraw.h = chromeHeight;
ideDraw.color = vectorControlColor;
vectorBorderDelta = 2524379 - 8886696;
ideDraw.border = 8886696 + chromeSelected * vectorBorderDelta;
ideDraw.box = true;
