# Touch recognizer / Распознавание касаний

**EN:** `main.c` demonstrates two-finger tap -> right mouse button using the
host-driven recognizer. Input timestamps are monotonic milliseconds; coordinates
are stage pixels. `BEGIN` starts a sequence, `MOVE` updates its maximum finger
count, `END` recognizes and dispatches, `CANCEL` discards. This is a host-side
callback, **not** an ActionScript `onPress` event yet.

**RU:** `main.c` показывает преобразование тапа двумя пальцами в правую кнопку
мыши. Время передаётся в монотонных миллисекундах, координаты — в пикселях
сцены. `BEGIN` начинает жест, `MOVE` обновляет максимальное количество пальцев,
`END` распознаёт жест, `CANCEL` отменяет. Событие пока поступает в C callback,
**не** в ActionScript `onPress`.

Build / Сборка: enable `FVM_BUILD_EXAMPLES=ON` in CMake.
