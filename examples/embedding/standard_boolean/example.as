/* FlashVM: Embedding/ActionScript tutorial: example.
 * File: examples/embedding/standard_boolean/example.as.
 */

// Host: call fvm_standard_install before executing these AVM1 actions.
// Приложение должно установить стандартную библиотеку перед выполнением AVM1.
/* Выполняем вызов Boolean с указанными аргументами. */
boxed = new Boolean(false);
/* Выполняем вызов boxed.valueOf с указанными аргументами. */
primitive = boxed.valueOf();
/* Выполняем вызов boxed.toString с указанными аргументами. */
label = boxed.toString();
/* Выполняем вызов Boolean с указанными аргументами. */
truth = Boolean(boxed);
/* Выполняем вызов Number с указанными аргументами. */
numeric = Number(boxed);
// primitive=false, label="false", truth=true, numeric=0.
// Methods currently require the wrapper; primitive automatic boxing is pending.
// Методы сейчас требуют объектную оболочку; автоматическая упаковка ещё не готова.
