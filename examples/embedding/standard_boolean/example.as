// Host: call fvm_standard_install before executing these AVM1 actions.
// Приложение должно установить стандартную библиотеку перед выполнением AVM1.
boxed = new Boolean(false);
primitive = boxed.valueOf();
label = boxed.toString();
truth = Boolean(boxed);
numeric = Number(boxed);
// primitive=false, label="false", truth=true, numeric=0.
// Methods currently require the wrapper; primitive automatic boxing is pending.
// Методы сейчас требуют объектную оболочку; автоматическая упаковка ещё не готова.
