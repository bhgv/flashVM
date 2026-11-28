// Copyright (c) 2026 bhgv
// EN: Publish composed queues and drive two typed result endpoints in the stock player.
// RU: Публикация составных очередей и работа двух концов результата в штатном плеере.


// EN: Retain an unbuffered endpoint without manufacturing a message.
// RU: Удержать синхронный конец без создания сообщения.
function meeting(seed) {
    meetingPort = vfs.resultChannel("i", 0);
    increment = seed;
    sentMeeting = false;
    pendingMeeting = undefined;
    return meetingPort;
}
