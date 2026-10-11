# C++98 channel wrapper / Обёртка канала на C++98

A minimal RAII wrapper releases its owned channel when leaving scope.
Copying is deliberately disabled to avoid accidental double-release;
explicitly call `fvm_channel_value_copy()` when another owner is required.

Минимальная RAII-обёртка освобождает канал при выходе из области видимости.
Копирование запрещено, чтобы избежать двойного освобождения. Для создания
нового владельца вызывайте `fvm_channel_value_copy()`.
