# Owned channel handles / Каналы с управляемым владением

This C89 example creates a channel, copies its *handle* (not its queue), sends
one number and receives it through the second handle. Each handle must be
released exactly once. Build using `FVM_BUILD_EXAMPLES=ON`.

Пример на C89 создаёт канал, копирует *дескриптор* (не очередь), передаёт число
и принимает его через второй дескриптор. Каждый дескриптор освобождается один раз.

**ActionScript design sketch (not yet implemented):** `var c = <>; c <- 17;`

**Эскиз ActionScript (пока не реализован):** `var c = <>; c <- 17;`
