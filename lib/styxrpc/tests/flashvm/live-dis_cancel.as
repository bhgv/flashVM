// Copyright (c) 2026 bhgv
// EN: Enter a live load whose DisVM image read is stalled by the test owner.
// RU: Запустить живую загрузку с приостановленным владельцем чтением образа DisVM.


// EN: Wait for the live proxy or fail when the movie cancels the task.
// RU: Дождаться живого прокси либо завершиться ошибкой при отмене задачи фильмом.
asinc function loadSlowDis() {
    var source = vfs.open("/slow.dis");
    var instance = source.load();
    var closed = source.close();
    var answer = instance.other(7);
    var retired = os.unload(instance);
    return answer;
}
