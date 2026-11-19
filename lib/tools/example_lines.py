# Copyright (c) 2026 bhgv
# EN: Preserve code while adding bilingual per-line teaching notes to examples only; never touch library algorithms.
# RU: Сохранить код, добавив EN/RU построчные пояснения только к примерам; алгоритмы библиотек не менять.
from pathlib import Path
import ast
import re
import tokenize
import io
from .layout import TOKEN, tokens

CALLS = {
    'memset': ('Clear the destination before filling its fields', 'Очистить приёмник до заполнения полей'),
    'memcpy': ('Copy the explicitly bounded byte span', 'Скопировать явно ограниченный диапазон байтов'),
    'memcmp': ('Compare the exact byte span, including embedded zero bytes', 'Сравнить точный диапазон, включая нулевые байты'),
    'strlen': ('Measure the NUL-terminated demonstration text', 'Измерить учебный текст до NUL'),
    'snprintf': ('Format within the supplied output capacity', 'Форматировать в указанной ёмкости'),
    'fvm_srecord_write': ('Encode a checked S-record in caller storage', 'Кодировать проверяемую S-запись в буфер вызывающего'),
    'fvm_srecord_read': ('Validate one complete S-record before using its borrowed payload', 'Проверить S-запись до использования заимствованного payload'),
    'fvm_sstream_push': ('Feed a fragment and preserve the exact consumed count', 'Подать фрагмент, сохранив точное consumed'),
    'fvm_sstream_reset': ('Release the current borrowed record before accepting another', 'Освободить текущее borrowed представление до следующей записи'),
    'pthread_join': ('Wait for the owned worker to finish before reclaiming its state', 'Дождаться своего worker до освобождения его состояния'),
    'pthread_mutex_lock': ('Acquire this state lock before modifying shared data', 'Взять этот lock до изменения общих данных'),
    'pthread_mutex_unlock': ('Publish the protected updates and release this state lock', 'Опубликовать защищённые изменения и снять lock'),
    'pthread_cond_wait': ('Wait with this queue lock released, reacquiring it on wakeup', 'Ждать со снятым lock очереди, повторно взяв его при пробуждении'),
    'pthread_cond_signal': ('Notify a waiting consumer of this queue', 'Уведомить ожидающего потребителя этой очереди'),
    'pthread_cond_broadcast': ('Notify all consumers waiting on this condition', 'Уведомить всех ожидающих этого условия'),
    'close': ('Release this owned native descriptor', 'Освободить этот native descriptor'),
    'free': ('Release the allocation after its final use', 'Освободить allocation после последнего использования'),
    'catalog_query': ('Ask the backend only for the selected object/page', 'Запросить у backend выбранный объект или страницу'),
    'catalog_pin': ('Retain the opened object independently of its published name', 'Удержать открытый объект независимо от имени'),
    'catalog_unpin': ('Release the opened-object hold', 'Снять удержание открытого объекта'),
    'convD2M': ('Pack a standard counted directory record', 'Упаковать стандартную counted запись каталога'),
    'sizeD2M': ('Measure the complete encoded directory entry', 'Измерить целую кодированную запись каталога'),
    'strtoull': ('Parse a numeric object identity and inspect the end pointer', 'Разобрать числовой ID и проверить end pointer'),
    'strtoul': ('Parse the numeric command-line value and inspect the end pointer', 'Разобрать числовой аргумент и проверить end pointer'),
}


# EN: Describe one code line from its actual operation; code is borrowed and no state is modified.
# RU: Описать строку по её операции; code заимствован, состояние не меняется.
def explain(code):
    value = code.strip()
    if value.startswith(('def ', 'class ')):
        return ('Define the documented handler below', 'Определить документированный ниже обработчик')
    for name, description in CALLS.items():
        if re.search(r'\b' + name + r'\s*\(', value):
            return description
    if value.startswith('#include'):
        return ('Import declarations for this translation unit', 'Подключить объявления этой единицы компиляции')
    if value.startswith(('#define', '#if', '#else', '#endif')):
        return ('Select the declared compile-time interface/profile', 'Выбрать объявленный compile-time интерфейс или профиль')
    if value.startswith(('return ', 'return;')):
        return ('Return this branch result to the caller; no further statements in this branch run', 'Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются')
    if value.startswith(('for ', 'for(', 'while ', 'while(')):
        return ('Iterate while the explicit loop condition permits', 'Повторять, пока выполняется явное условие цикла')
    if value.startswith(('if ', 'if(', 'elif ', 'else if')):
        return ('Choose a branch using this condition', 'Выбрать ветку по этому условию')
    if value == 'else' or value.startswith('else:'):
        return ('Handle the alternative to the preceding condition', 'Обработать альтернативу предыдущего условия')
    if value in ('{', '}','};',')','):',']',']:','],', '),'):
        return ('Begin this scope' if value=='{' else 'Finish this scope or grouped value', 'Начало области' if value=='{' else 'Конец области либо составного значения')
    if value.startswith(('break', 'continue')):
        return ('Leave this loop' if value.startswith('break') else 'Advance to the next iteration', 'Выйти из цикла' if value.startswith('break') else 'Перейти к следующей итерации')
    match = re.match(r'([\w.>\-\[\]*]+)\s*(?:=|\+=|\+\+|--)', value)
    if match:
        return (f'Update {match[1]} for the current operation', f'Обновить {match[1]} для текущей операции')
    match = re.match(r'(?:static |const |unsigned |struct )*(?:int|char|size_t|uint64_t|long|void|[A-Z]\w*)\s+(.+?)[;=]', value)
    if match:
        return ('Declare operation-local storage; its initializer specifies the initial value', 'Объявить память операции; initializer задаёт начальное значение')
    match = re.search(r'([\w.]+)\s*\(', value)
    if match:
        return (f'Invoke {match[1]} with these explicit arguments', f'Вызвать {match[1]} с этими явными аргументами')
    if value.startswith(('import ', 'from ')):
        return ('Import the standard-library/test helper used below', 'Подключить используемый ниже standard-library или test helper')
    if value.startswith(('def ', 'class ')):
        return ('Define the documented handler below', 'Определить документированный ниже обработчик')
    return ('Continue the surrounding expression with these operands', 'Продолжить окружающее выражение этими операндами')


# EN: Add notes to physical C lines that contain tokens but no comment, preserving all native tokens.
# path is owned example source; existing comments/literals are preserved. Raises if code tokens drift.
# RU: Добавить пояснения к C-строкам с кодом без комментария; сохранить native tokens.
# path — исходник примера; комментарии/литералы сохранены. При изменении токенов — исключение.
def c_lines(path):
    original = path.read_text()
    comments = set()
    for match in TOKEN.finditer(original):
        if match.group().startswith(('/*', '//')):
            comments.update(range(original.count('\n',0,match.start()), original.count('\n',0,match.end())+1))
    lines = original.splitlines()
    for index, line in enumerate(lines):
        if line.strip() and index not in comments and not line.rstrip().endswith('\\'):
            en, ru = explain(line)
            lines[index] = line + ' /* EN: ' + en + '. RU: ' + ru + '. */'
    result = '\n'.join(lines) + '\n'
    if tokens(result) != tokens(original):
        raise ValueError('example token drift: ' + str(path))
    path.write_text(result)


# EN: Annotate Python physical lines outside strings/comments, preserving the complete Python AST.
# path is an owned example. Explicit continuations cannot accept trailing comments and are left intact.
# RU: Аннотировать строки Python вне строк/комментариев, сохранив AST целиком.
# path — пример. Явные backslash-продолжения нельзя комментировать в конце; они сохраняются.
def python_lines(path):
    original = path.read_text()
    excluded = set()
    for token in tokenize.generate_tokens(io.StringIO(original).readline):
        if token.type in (tokenize.STRING, tokenize.COMMENT):
            if token.type == tokenize.COMMENT or token.start[0] != token.end[0]:
                excluded.update(range(token.start[0]-1, token.end[0]))
    lines = original.splitlines()
    for index, line in enumerate(lines):
        if line.strip() and index not in excluded and not line.rstrip().endswith('\\'):
            en, ru = explain(line)
            lines[index] = line + '  # EN: ' + en + '. RU: ' + ru + '.'
    result = '\n'.join(lines) + '\n'
    if ast.dump(ast.parse(original), include_attributes=False) != ast.dump(ast.parse(result), include_attributes=False):
        raise ValueError('example AST drift: ' + str(path))
    path.write_text(result)


# EN: Annotate only exported examples; no library source or canonical engine snapshot is changed.
# root is lib/, owned by the development checkout. Exceptions stop the pass; builds never invoke this tool.
# RU: Аннотировать только внешние примеры; код библиотек и canonical engine не меняется.
# root — lib/ development checkout. Ошибка останавливает проход; сборка этот tool не вызывает.
def main(root):
    for package in ('styx', 'libunibus'):
        for path in (root / package / 'examples').rglob('*'):
            if path.suffix in ('.c','.h','.inc'):
                c_lines(path)
            elif path.suffix == '.py':
                python_lines(path)


if __name__ == '__main__':
    main(Path(__file__).resolve().parents[1])
