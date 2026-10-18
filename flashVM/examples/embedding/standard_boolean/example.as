/* FlashVM: Embedding/ActionScript tutorial: example.
 * File: examples/embedding/standard_boolean/example.as.
 */

// Host: call fvm_standard_install before executing these AVM1 actions.
// Приложение должно установить стандартную библиотеку перед выполнением AVM1.
/* Вызываем Boolean: преобразуем значение в Boolean.
 * false — преобразуемое значение. */
boxed = new Boolean(false);
/* Вызываем boxed.valueOf: извлекаем примитивное значение из обёртки. */
primitive = boxed.valueOf();
/* Вызываем boxed.toString: получаем строковое представление значения. */
label = boxed.toString();
/* Вызываем Boolean: преобразуем значение в Boolean.
 * boxed — преобразуемое значение. */
truth = Boolean(boxed);
/* Вызываем Number: преобразуем значение в Number.
 * boxed — преобразуемое значение. */
numeric = Number(boxed);
// primitive=false, label="false", truth=true, numeric=0.
// Methods currently require the wrapper; primitive automatic boxing is pending.
// Методы сейчас требуют объектную оболочку; автоматическая упаковка ещё не готова.
