// Executed by FlashVM AVM1 for every completed Dis view, including the file browser.
// idePanel is an owner-local TextField capability; no Dis/native heap pointer is sent.
// All other bindings are copied values admitted by the native channel adapter.
// Text content, selected row and directory actions remain owned by the Limbo controller.
// Keep layout policy in AS; this mutable local-to-context binding is not an RPC pointer.
ideViewPadding = 8;
idePanel.multiline = true;
idePanel.wordWrap = false;
idePanel.text = idePanelText;
// The host supplies finite admitted dimensions and font metrics, in native pixels.
// AS owns the widget inset and TextField extent; the raster port only paints the result.
idePanel._width = ideWidth - ideViewPadding * 2;
idePanel._height = idePanelHeight - ideViewPadding;
idePanel._x = ideViewPadding;
idePanel._y = ideHeight - idePanelHeight + 6;
