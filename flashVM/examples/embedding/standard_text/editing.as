/* FlashVM: Embedding/ActionScript tutorial: editing.
 * File: examples/embedding/standard_text/editing.as.
 */

// The host installs fvm_standard_text_install, registers a font, creates the
// field with fvm_textfield_create and publishes it as caption. No display node
// or MovieClip.createTextField is implied by this example.
// Приложение создаёт и публикует caption через C API; display list ещё не готов.
/* Сохраняем вычисленное значение: caption.text. */
caption.text = "AV fi";
/* Вызываем caption.replaceText: заменяем выбранный диапазон текста.
 * 0 — индекс начала диапазона.
 * 2 — индекс за концом диапазона.
 * "B" — новый текст. */
caption.replaceText(0, 2, "B");
/* Вызываем caption.getTextFormat: читаем формат выбранного диапазона текста.
 * 0 — индекс начала диапазона.
 * 1 — индекс за концом диапазона. */
current = caption.getTextFormat(0, 1);
/* Вызываем caption.replaceText: заменяем выбранный диапазон текста.
 * 4 — индекс начала диапазона.
 * 4 — индекс за концом диапазона.
 * "!" — новый текст. */
caption.replaceText(4, 4, "!");
// caption.text is now "B fi!"; indices are UTF-16 units, end is exclusive.
// Индексы считаются в единицах UTF-16; конечный индекс не входит в диапазон.

/* Disable wrapping so automatic sizing can fit both dimensions. */
caption.wordWrap = false;
/* Keep the native left origin fixed; fit the existing text without creating a display node. */
caption.autoSize = "left";
