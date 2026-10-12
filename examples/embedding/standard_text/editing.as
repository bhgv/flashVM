/* FlashVM: Embedding/ActionScript tutorial: editing.
 * File: examples/embedding/standard_text/editing.as.
 */

// The host installs fvm_standard_text_install, registers a font, creates the
// field with fvm_textfield_create and publishes it as caption. No display node
// or MovieClip.createTextField is implied by this example.
// Приложение создаёт и публикует caption через C API; display list ещё не готов.
/* Сохраняем вычисленное значение: caption.text. */
caption.text = "AV fi";
/* Выполняем вызов caption.replaceText с указанными аргументами. */
caption.replaceText(0, 2, "B");
/* Выполняем вызов caption.getTextFormat с указанными аргументами. */
current = caption.getTextFormat(0, 1);
/* Выполняем вызов caption.replaceText с указанными аргументами. */
caption.replaceText(4, 4, "!");
// caption.text is now "B fi!"; indices are UTF-16 units, end is exclusive.
// Индексы считаются в единицах UTF-16; конечный индекс не входит в диапазон.
