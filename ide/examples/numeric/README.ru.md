# Пример численной сборки

[English](README.md). Исполняемый исходник численного поднабора AS2 выводит `42`.
Это настоящий вход компилятора/линкера, а не полный языковой профиль AS2.
[Руководство backend](../../README.ru.md) описывает стадии, владение, timeout,
JSON и контракт публикации. Явно укажите доверенные host executables:

```sh
python3 ide/build.py ide/examples/numeric/program.as movie.swf --language as2 \
  --compiler /host/fir1_source --ir /host/fir1 --linker /host/swl \
  --module example.numeric --runner /host/flashvm_runner --expect-trace '42
'
```

В строке ожидания находится настоящий перевод строки. Успех возвращает JSON
с ok=true и published=true, exit 0 и заменяет movie.swf. Ошибки source,
verification, optimization, object/link/run сохраняют прежний movie.
Редактор владеет возвращённым JSON и output path; backend дожидается прямых
children и удаляет промежуточные файлы. Объекты VM/компилятора не сохраняются.
