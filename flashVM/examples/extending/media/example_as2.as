/* FlashVM: Embedding/ActionScript tutorial: example as2.
* File: examples/extending/media/example_as2.as.
*/

// AS2 (SWF 7/8): standard Flash sound APIs, not implemented by FlashVM yet.
/* Объявляем sound и сохраняем начальное значение. */
var sound: Sound = new Sound(); // Объявляем sound и сохраняем начальное значение.
/* Вызываем sound.attachSound: выбираем встроенный звук по идентификатору экспорта.
* "click" — имя экспортированного звукового ресурса. */
sound.attachSound("click"); // Вызываем sound.attachSound: выбираем встроенный звук по идентификатору экспорта. "click" — имя экспортированного звукового ресурса.
/* Вызываем sound.start: запускаем выбранный звук. */
sound.start(); // Вызываем sound.start: запускаем выбранный звук.
/* Вызываем sound.stop: останавливаем воспроизведение звука. */
sound.stop(); // Вызываем sound.stop: останавливаем воспроизведение звука.
