# Явные начертания встроенных шрифтов

v0.869 добавляет [fvm_standard_text_register_font_variant](../include/flashvm/standard_text.h#L35)(context, family, variant,
bytes, size, face_index). Функция проверяет и копирует настоящее лицо шрифта в слот
семейства: [FVM_TEXT_FONT_REGULAR](../include/flashvm/standard_text.h#L10) (ноль), [FVM_TEXT_FONT_BOLD](../include/flashvm/standard_text.h#L11), [FVM_TEXT_FONT_ITALIC](../include/flashvm/standard_text.h#L12)
или их побитовое объединение. Нужное лицо передаёт хост; флаги выбирают слот,
не синтезируют контуры и не угадывают начертание по метаданным. Прежняя
[fvm_standard_text_register_font](../include/flashvm/standard_text.h#L25) регистрирует regular без изменения контракта.

Нужны живой контекст в потоке-владельце, непустое terminated-имя семейства до 255
байтов, допустимые флаги и читаемые данные/index лица. Байты заимствуются на время
регистрации, после успеха контекст владеет независимой копией. Дубликат family/style
и неизвестные биты дают INVALID_ARGUMENT. Ошибки проверки шрифта передаются
вызывающему. OUT_OF_MEMORY не публикует слот. Прежние границы 16 лиц и 64 MiB
исходных данных учитывают все начертания вместе; превышение даёт UNSUPPORTED.
NO_FONTS возвращает UNSUPPORTED для допустимого запроса. Во время GC/teardown
вызовы недопустимы. Системный поиск, callbacks и синхронизация чужих потоков
не выполняются. Только первая regular-регистрация выбирает семейство по умолчанию.
Семейство только со styled-лицами не становится regular по умолчанию. Контекст
освобождает все копии источников; замена ранее зарегистрированного слота запрещена.

Приватные counted-ключи начертаний начинаются с NUL и не пересекаются с публичными
C-именами семейств. Снимки native-полей содержат публичное имя, а не приватный ключ.
Загрузка embedded-font SWF, device/system fonts, fallback, styled spans и полная
совместимость Flash ещё не завершены. Все стадии roadmap открыты.

[Справочник Adobe AS2 TextFormat, зеркало](https://open-flash.github.io/mirrors/as2-language-reference/TextFormat.html)
требует отдельных embedded-лиц для bold/italic. Здесь их предоставляет явная
регистрация хостом; реальные платформы и устройства проверяются отдельно.

## Выбор в native TextFormat — v0.870

bold/italic используют прежнее примитивное Boolean-преобразование и независимо
выбирают биты слота семейства. Null/undefined при слиянии формата сохраняют прежний
выбор. getTextFormat/getNewTextFormat возвращают фактические Boolean-начертания;
новый неприменённый TextFormat по-прежнему начинается с null. Изменения текущего
и будущего формата проверяют выбранное лицо до публикации. Отсутствующее начертание
даёт NOT_FOUND без изменения текста, формата и геометрии. Измерение и экспорт
глифов/подчёркивания используют один заранее вычисленный ключ лица. Изменяемые
font handles между вызовами не сохраняются; выделения ключа во время layout
и системного fallback не добавлено.

Native-граф хранит bold/italic в отдельных style-битах и встраивает выбранные
текущий/будущий источники. Новые числовые payload-поля и изменения layout публичных
структур не нужны. Старые записи восстанавливают regular; старые readers отвергают
новые биты. Уже имеющийся family/style получателя должен совпадать по байтам/index;
остальные начертания этого семейства независимы. Совместимость нового графа со
старым reader не обещается. Поток контекста, GC и откат сохраняют прежний контракт.

v0.871 добавляет три исходных CC0-лица с разными AV advance при 40px: regular 48, bold 56, italic 52, bold-italic 60. Метрики поля/TextFormat и границы контуров подтверждают выбор источника; включение кернинга вычитает 3.2 для каждого лица. Реестр проверяет общую границу 16 лиц по всем слотам семейств.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_standard_text_register_font` | function | [include/flashvm/standard_text.h:25](../include/flashvm/standard_text.h#L25) |
| `fvm_standard_text_register_font_variant` | function | [include/flashvm/standard_text.h:35](../include/flashvm/standard_text.h#L35) |
| `FVM_TEXT_FONT_BOLD` | macro / define | [include/flashvm/standard_text.h:11](../include/flashvm/standard_text.h#L11) |
| `FVM_TEXT_FONT_ITALIC` | macro / define | [include/flashvm/standard_text.h:12](../include/flashvm/standard_text.h#L12) |
| `FVM_TEXT_FONT_REGULAR` | macro / define | [include/flashvm/standard_text.h:10](../include/flashvm/standard_text.h#L10) |
<!-- source-links:end -->
