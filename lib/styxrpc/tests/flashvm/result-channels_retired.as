// Copyright (c) 2026 bhgv
// EN: Publish composed queues and drive two typed result endpoints in the stock player.
// RU: Публикация составных очередей и работа двух концов результата в штатном плеере.


// EN: Confirm the remote result fids have released both owner endpoints.
// RU: Убедиться, что удалённые fid результатов освободили оба локальных конца.
function retired(ignored) {
    var result = 0;
    if (sentMeeting) {
        if (!bufferedPort.active()) {
            if (!meetingPort.active()) {
                var closedBuffer = bufferedPort.close();
                var closedMeeting = meetingPort.close();
                result = 1;
            }
        }
    }
    return result;
}
