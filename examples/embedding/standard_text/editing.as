// The host installs fvm_standard_text_install, registers a font, creates the
// field with fvm_textfield_create and publishes it as caption. No display node
// or MovieClip.createTextField is implied by this example.
// Приложение создаёт и публикует caption через C API; display list ещё не готов.
caption.text = "AV fi";
caption.replaceText(0, 2, "B");
current = caption.getTextFormat(0, 1);
caption.replaceText(4, 4, "!");
// caption.text is now "B fi!"; indices are UTF-16 units, end is exclusive.
// Индексы считаются в единицах UTF-16; конечный индекс не входит в диапазон.
