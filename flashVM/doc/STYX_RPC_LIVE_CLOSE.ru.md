# binding.close

[English](STYX_RPC_LIVE_CLOSE.md) · [Живые переменные](STYX_RPC_LIVE_VARIABLES.ru.md) · [Типы моста](STYX_RPC_VALUE_TYPES.ru.md) · [Реализация](../src/styxrpc/session_live_values.c#L208)

## Синтаксис

```actionscript
binding.close():Boolean
```


## Результат и эффект

Аргументов нет. Первый вызов освобождает собственный fid и возвращает `true`. Повторный вызов возвращает `false`. Все алиасы прокси видят закрытое состояние. Независимо захваченное поле удерживает собственный fid. Закрытие допустимо после снятия публикации. Результат сообщает о локальном владении. Когда последний алиас недостижим, сборщик мусора может отложить освобождение fid.

Явное закрытие выполняется в контексте владельца и может ожидать обычного освобождения fid. Выполняйте его вне другой операции на этой сессии. Лишние аргументы приводят к ошибке до закрытия.

## Пример

```actionscript
// Явно освобождаем временную привязку чтения.
var name = vfs.liveVariable("/live/.var/details/name");
var currentName = name.get();
var firstClose = name.close(); // true
var secondClose = name.close(); // false
```

[Открытие привязки](STYX_RPC_LIVE_VARIABLE.ru.md)
