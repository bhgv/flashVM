# Взаимодополняющее исполнение Dis и SWF

[English](ARCHITECTURE.md) · [Граница ядра](README.ru.md)

DisVM и FlashVM — соседние независимые движки. Limbo/Dis и AS/SWF должны уметь
реализовывать и использовать общие модульные интерфейсы. `.m` — авторитетный
контракт и для реализации, и для вызывающего кода. Внешние модули не ограничены C:
AS-реализация не-Dis модуля и AS-клиент Dis — два обязательных направления.

Собственный компилятор уже выдаёт descriptor/source scaffolds обеих сторон.
Настоящий bridge ещё должен разрешить signatures/types/layouts, зарегистрировать
внешние экспорты, переносить значения/capabilities и удерживать pending GC roots.
Нельзя обходить typed UI channels, передавать raw heap pointers или блокировать
render/event owner на rendezvous. Call IDs не являются native frame pointers.
Кучи/планировщики раздельны, debugger показывает boundary frames и source identity
ожидающих, завершённых и ошибочных межмашинных вызовов.

Используются настоящие native module/channel механизмы, без подмены Dis Runtab
и native frames обычными AS-объектами. C/AS-генерация по `.m` должна опираться на
общий разрешённый semantic descriptor; текущий syntactic manifest — промежуточный
шаг, не замена настоящим runtime signatures.

Позднейшее объединение возможно, но отдельно проектируется: module identity,
семантика значений/инструкций, GC, исключения, scheduling, debug relocation и
совместимость link/load. Нельзя заранее объявлять общий формат через случайный
SWF tag или контейнер двух независимых файлов. FIR может понижаться к обоим
движкам, сохраняя семантику исходного языка.
