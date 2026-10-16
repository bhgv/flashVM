# Регистр linkage-имён — v0.380

В загруженных SWF4–6 имена Object.registerClass теперь сравниваются по
закреплённой таблице Flash для отдельных UTF-16 units. Общее правило используют
регистрация, удаление через null/undefined, C89-поиск класса, attachMovie и
дублирование. Сохраняются встроенный NUL, одиночные суррогаты и каноническая
эквивалентность scalar/paired UTF-8. I с точкой совпадает с i; Deseret остаётся
различимым, поскольку регистр меняется по units. SWF7–8 и незагруженный host
по-прежнему сравнивают точное каноническое написание.

Замена имени с другим регистром обновляет и конструктор, и сохранённое написание.
Удаление/вставка готовятся в копии реестра и публикуются swap без выделений;
OOM сохраняет прежнюю запись целиком. Непосредственный поиск не выделяет память
и не вызывает getters. Валидация конструктора по-прежнему предшествует
преобразованию имени. Сохраняются прежняя политика null-выхода C-запроса при
ошибке и политика выхода нативного registerClass. ABI, callbacks и wire не меняются.

[GC:] Существующий нативный барьер сохраняет ссылки в подготовленном реестре и
при повторном входе из преобразования имени. Перенос Object сохраняет написание
и граф конструктора; правило сравнения выбирает текущий контекст получателя.
Восстановленный library fallback без Object использует тот же поиск.
Подготовленные process-local тела функций по-прежнему не переносятся.

Проверки: object_register_class_case — host/SWF4–8, ASCII/Latin/Cyrillic/Greek,
I с точкой, NUL, суррогаты, Deseret, замена написания, удаление, GC и независимый
перенос; object_register_class_case_allocation — все границы OOM при замене,
прежнее/новое состояние, GC/retry и поиск при нулевом allowance;
movieclip_registered_case — SWF5–8, attachMovie, нативное дублирование и
восстановленный fallback; jit_registered_case — регистрация/замена/удаление
через интерпретатор, явный и автоматический JIT в host/SWF4–8.

Сопоставление исходников: [реестр закреплённого Ruffle](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/runtime.rs)
и [проверка аргументов registerClass](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/object.rs).
Это не новый прогон оригинального Flash Player.

Открыты отдельные global environments, точный выбор версии смешанных movie и
base clip, прежние совпадающие по регистру записи из другой политики контекста,
AMF typed aliases и полная привязка symbols. Сам поиск экспортируемого ресурса
не меняется. Все девять стадий остаются открытыми.

Validation v0.380: all 789 AGG / 789 Cairo / 770 minimal Linux Release checks
have passing evidence after three ELF execution-permission repairs and targeted
reruns (one AGG, two Cairo; minimal passed initially). Eight restricted heap/
global ASan+UBSan checks pass: four new registry/scene/JIT cases plus existing
C89 registry, name/transfer, sprite resources and duplicate-version checks.
Stack ASan and LeakSanitizer remain disabled. No new original external SWF
oracle or foreign-OS run is claimed. All nine roadmap stages remain open.

## v0.390 раздельные реестры конструкторов

Новые реестры хранят точные регистрации host/SWF7–8 в захваченной записи, а
SWF4–6 — в трассируемом дочернем объекте приватного индексного слота 0. Любые
строковые имена, включая NUL и «0», с ним не пересекаются. Повторная загрузка
контекста с другой группой версий не открывает и не заменяет чужие записи.
Обе карты переносятся прежним кодеком графов и удерживаются замыканием
registerClass, включая резервный реестр библиотеки. ABI и ID callback прежние.
При OOM замена атомарна. Старые плоские записи без дочернего объекта сохраняют
прежнее общее хранилище для совместимости.

object_register_class_environments и _allocation проверяют реальные загрузки
SWF4–8, активацию host, замену/удаление, NUL/числовые имена, GC, независимый
перенос и OOM/retry. Полное разделение глобальных встроенных объектов и точная
привязка к base clip при смешанных версиях остаются открытыми. Сравнение с
указанными исходниками Ruffle не означает проверку оригинальным Flash Player.
