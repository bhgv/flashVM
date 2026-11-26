// Copyright (c) 2026 bhgv
// EN: Withdraw published callback files between peer requests and reuse their paths.
// RU: Отзывать опубликованные файлы между запросами участника и переиспользовать пути.


// EN: Publish the original tree once. Make the requested changes after its callback returns.
// RU: Один раз опубликовать исходное дерево. Применить изменения после выхода из колбэка.
function onEnterFrame() {
    if (!withdrawReady) {
        var changed = os.cd("/tools");
        var file = vfs.newnode("/menu/data", "rwx", withdrawOriginal, 1);
        var sibling = vfs.newnode("/menu/keep", "r", withdrawOriginal);
        var branch = vfs.newnode("/gone/deep/value", "r", withdrawOriginal);
        withdrawReady = true;
    }
    if (withdrawArmed) {
        if (!withdrawDone) {
            var directory = os.rmnode("/menu");
            var reserved = vfs.rmnode("/.upload");
            var traversal = vfs.rmnode("/menu/../menu/data");
            var count = 0;
            while (160 - count) {
                var created = vfs.newnode("/scratch/entry", "r", withdrawOriginal);
                var retired = os.rmnode("scratch/entry");
                if (!retired) {
                    invalidWithdrawal = true;
                }
                count = count + 1;
            }
            var removed = os.rmnode("menu/data");
            var repeated = vfs.rmnode("/menu/data");
            var pruned = vfs.rmnode("/gone/deep/value");
            var observed = directory + "|" + reserved + "|" + traversal + "|" + removed + "|" + repeated + "|" + pruned + "|";
            if (!invalidWithdrawal) {
                if (!observed.indexOf("false|false|false|true|false|true|")) {
                    var replacement = vfs.newnode("/menu/data", "rx", withdrawReplacement, 3);
                }
            }
            withdrawDone = true;
        }
    }
}
