// Copyright (c) 2026 bhgv
// EN: Retain the first exact tree in a detached task so the caller can observe the stale error.
// RU: Удержать первое дерево в отдельной задаче, чтобы вызывающий увидел отказ устаревшего прокси.


// EN: Retain the first exact tree in a detached task so the caller can observe the stale error.
// RU: Удержать первое дерево в отдельной задаче, чтобы вызывающий увидел отказ устаревшего прокси.
asinc function rejectStale() {
    var root = vfs.liveVariable("/live/.var", true);
    var numbers = root.field("numbers");
    var ready = vfs.mkdir("/live-stage-held");
    var pending = true;
    while (pending) {
        var gate = vfs.open("/live-stage-gate");
        var flag = gate.read(1);
        var closed = gate.close();
        pending = flag[0] - 49;
    }
    var impossible = numbers.set(0, 88);
    return impossible;
}
