# Table inspection / Инспекция таблиц

**English:** Compile as C89 and link with `flashvm`. This example writes a named property and a numeric slot, then queries object and property counts. No ActionScript integration of metatables exists yet.

**Русский:** Компилируйте как C89 и линкуйте с `flashvm`. Пример записывает именованное свойство и числовой элемент, затем получает количество свойств и объектов. Интеграция метатаблиц с ActionScript пока отсутствует.

ActionScript 2.0 (conceptual, only named properties currently supported):

```actionscript
var obj = new Object();
obj.answer = 42;
trace(obj.answer);
```
