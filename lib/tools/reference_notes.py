# Copyright (c) 2026 bhgv
# EN: Reviewed parameter contracts for wire codecs and the simplest public integration paths.
# RU: Проверенные контракты параметров wire-codecs и основных публичных путей интеграции.
PARAMETERS = {
    'convM2S': {
        'ap': ('Mutable input message; decoded string/data pointers borrow this storage.', 'Изменяемое входное сообщение; строки/data результата заимствуют эту память.'),
        'nap': ('Accessible input bytes, including the four-byte message length.', 'Доступные входные байты, включая четырёхбайтовую длину сообщения.'),
        'f': ('Output Fcall storage. On zero return, discard its potentially partial fields.', 'Приёмник Fcall. При возврате нуля частично заполненные поля отбрасываются.')},
    'convS2M': {
        'f': ('Input message fields and borrowed strings/data to encode; caller retains ownership.', 'Входные поля и заимствованные строки/data; владение остаётся у вызывающего.'),
        'ap': ('Caller-owned output storage, disjoint from referenced input fields.', 'Выходной буфер вызывающего, не перекрывающий входные поля.'),
        'nap': ('Available output bytes; sizeS2M reports the required complete extent.', 'Доступная выходная ёмкость; sizeS2M сообщает размер целого сообщения.')},
    'sizeS2M': {'f': ('Input message to measure, with valid referenced strings and payload.', 'Входное сообщение для измерения с допустимыми строками и payload.')},
    'sizeD2M': {'d': ('Input directory metadata; strings must be valid NUL-terminated C strings.', 'Входные metadata каталога; строки должны быть допустимыми C-строками с NUL.')},
    'convD2M': {
        'd': ('Input directory metadata to encode as one counted stat record.', 'Входные metadata для одной counted stat-записи.'),
        'buf': ('Caller-owned writable destination; do not confuse a two-byte size prefix with a complete record.', 'Выходной буфер вызывающего; двухбайтовый size prefix не является целой записью.'),
        'nbuf': ('Destination capacity in bytes, including the leading two-byte length.', 'Ёмкость приёмника в байтах, включая двухбайтовую длину.')},
    'convM2D': {
        'buf': ('Input counted stat bytes; no native Dir layout is accepted.', 'Входные counted stat-байты; native layout Dir не принимается.'),
        'nbuf': ('Accessible input extent in bytes.', 'Доступная входная длина в байтах.'),
        'd': ('Caller-owned decoded scalar fields and string pointers.', 'Приёмник декодированных scalar fields и указателей строк.'),
        'strs': ('Caller-owned decoded-string storage, or NULL for empty string fields. nbuf bytes suffice for a valid record.', 'Буфер декодированных строк либо NULL для пустых string fields. Для допустимой записи достаточно nbuf байт.')},
    'statcheck': {
        'buf': ('Counted stat record to validate structurally.', 'Counted stat-запись для структурной проверки.'),
        'nbuf': ('Accessible extent; must contain the complete counted record.', 'Доступная длина; должна содержать целую counted запись.')},
    'fvm_sstream_init': {
        'stream': ('Writable decoder state for one receive direction; initialize before push.', 'Изменяемое состояние одного направления приёма; init до push.'),
        'buffer': ('Caller-owned receive storage, disjoint from stream and later input fragments; retain through decoder use.', 'Память приёма вызывающего, отдельно от stream и входных фрагментов; сохраняется на время работы decoder.'),
        'capacity': ('Accessible bytes in buffer; may impose a tighter bound than limit.', 'Доступные байты buffer; могут ограничивать запись сильнее limit.'),
        'limit': ('Maximum body bytes, including encoded tag: 1..FVM_SRECORD_BODY_MAX.', 'Максимум байтов body, включая encoded tag: 1..FVM_SRECORD_BODY_MAX.')},
    'fvm_sstream_push': {
        'stream': ('Initialized decoder; a ready view requires reset before another push.', 'Инициализированный decoder; после готового view нужен reset до нового push.'),
        'input': ('Borrowed new fragment, disjoint from receive buffer/state; NULL only with size zero.', 'Новый заимствованный фрагмент, отдельно от receive buffer/state; NULL только при size=0.'),
        'size': ('Accessible bytes in this new fragment, not total bytes retained by the decoder.', 'Доступные байты нового фрагмента, не общий накопленный размер.'),
        'view': ('Published only on OK; borrows receive storage until reset/disposal.', 'Публикуется только при OK; заимствует receive storage до reset/disposal.'),
        'used': ('Consumed fragment bytes, including on MORE/data error; retain only the unconsumed suffix. Invalid arguments leave it unchanged.', 'Поглощённые байты фрагмента, также при MORE/data error; сохранять только непоглощённый хвост. Неверные аргументы оставляют used прежним.')},
    'fvm_sstream_reset': {'stream': ('Initialized decoder to reuse; invalidates the previous borrowed view and clears latched errors.', 'Decoder для повторного использования; делает прежний view недействительным и сбрасывает latched errors.')},
    'fvm_srecord_write': {
        'tag': ('Unsigned direction-local tag, 0..0xffffffff; transport tag 0 is service-owned.', 'Беззнаковый тег направления 0..0xffffffff; transport tag 0 принадлежит обслуживанию.'),
        'payload': ('Borrowed opaque bytes, possibly nested records; disjoint from output/state.', 'Заимствованные непрозрачные байты, возможно вложенные записи; отдельно от output/state.'),
        'size': ('Payload bytes; tag+payload must fit the body profile bound.', 'Байты payload; tag+payload должны помещаться в предел body-профиля.'),
        'output': ('Caller-owned writable destination for the complete checked record.', 'Выходной буфер вызывающего для целой проверяемой записи.'),
        'capacity': ('Accessible destination bytes; insufficient space reports LIMIT.', 'Доступные выходные байты; нехватка места даёт LIMIT.'),
        'used': ('Complete encoded byte count on OK; unchanged on failure.', 'Полная кодированная длина при OK; при отказе не меняется.')},
    'fvm_srecord_read': {
        'input': ('Borrowed input bytes; preserve them throughout use of the returned record view.', 'Заимствованный вход; сохраняется всё время использования record view.'),
        'size': ('Accessible input bytes; trailing records may remain unconsumed.', 'Доступные входные байты; последующие записи могут остаться непоглощёнными.'),
        'limit': ('Allowed body bytes, including encoded tag: 1..FVM_SRECORD_BODY_MAX.', 'Допустимые body-байты, включая encoded tag: 1..FVM_SRECORD_BODY_MAX.'),
        'view': ('Verified borrowed record, published only on OK.', 'Проверенная заимствованная запись, публикуемая только при OK.'),
        'used': ('Bytes of exactly one record on OK; unchanged on any other result.', 'Байты ровно одной записи при OK; иначе не меняется.')},
    'styxorderwait': {
        'order': ('Live native queue engine; keep alive until this wait and other concurrent calls finish.', 'Живой native queue engine; удерживать до завершения этого wait и параллельных вызовов.'),
        'tag': ('Only this direction-local tag queue is observed.', 'Наблюдается только очередь этого тега направления.'),
        'timeout': ('Monotonic wait budget in milliseconds, 0..120000; zero tests current readiness.', 'Бюджет по monotonic clock в миллисекундах 0..120000; ноль проверяет текущую готовность.')},
    'styxordernext': {
        'ordinal': ('Output sequence of the consumed record; unchanged when no record is consumed.', 'Выходной номер поглощённой записи; без поглощения не меняется.'),
        'bytes': ('Caller-owned destination, or NULL when capacity is zero.', 'Выходной буфер вызывающего либо NULL при capacity=0.'),
        'capacity': ('Destination bytes; SMALL preserves the queued record for retry.', 'Выходная ёмкость; SMALL сохраняет запись в очереди для повторного чтения.'),
        'size': ('Required/actual payload bytes when a record is present, including SMALL.', 'Требуемые/фактические байты payload при наличии записи, также при SMALL.')},
}

NOTES = {
    'fvm_sstream_push': ('OK yields at most one record. MORE retains consumed input inside stream. Errors latch until reset; calling push after a ready view returns INVALID with used=0. No I/O or blocking is performed; concurrent access to the same decoder needs external synchronization.',
                         'OK выдаёт максимум одну запись. MORE сохраняет поглощённый вход внутри stream. Ошибки удерживаются до reset; push после готового view даёт INVALID и used=0. Нет I/O и блокировки; параллельный доступ к одному decoder требует внешней синхронизации.'),
    'styxordernext': ('RECORD consumes exactly one ready item; SMALL publishes its required size without removing it. EMPTY means no ready item on an open engine; END means closed/invalid without an available item. Ready items can still be drained after close. This call does not wait for arrival.',
                      'RECORD поглощает ровно одну готовую запись; SMALL сообщает требуемый размер без удаления. EMPTY — нет готовой записи открытого engine; END — закрыт/неверен без доступной записи. После close готовые записи ещё можно дочитать. Вызов не ждёт появления данных.'),
    'styxorderwait': ('Returns nonzero only if the selected queue has a ready record. Returns zero on timeout, closed-empty state, invalid arguments or native clock/wait failure. It does not distinguish these zero cases; inspect lifecycle separately. Other tags have independent readiness.',
                      'Ненулевой результат только при готовой записи выбранной очереди. Ноль — timeout, closed-empty, неверные аргументы либо native clock/wait error. Причины нуля не различаются; lifecycle проверяется отдельно. У других тегов независимая готовность.'),
}

# Source definitions use out; the public prototype calls the same destination output.
PARAMETERS["fvm_srecord_write"]["out"] = PARAMETERS["fvm_srecord_write"]["output"]
