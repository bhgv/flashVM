/* FlashVM: Embedding/ActionScript tutorial: example as2.
 * File: examples/extending/media/example_as2.as.
 */

// AS2 (SWF 7/8): standard Flash sound APIs, not implemented by FlashVM yet.
/* Объявляем sound и сохраняем начальное значение. */
var sound : Sound = new Sound();
/* Выполняем вызов sound.attachSound с указанными аргументами. */
sound.attachSound("click");
/* Выполняем вызов sound.start с указанными аргументами. */
sound.start();
/* Выполняем вызов sound.stop с указанными аргументами. */
sound.stop();
