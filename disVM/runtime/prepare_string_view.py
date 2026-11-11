# Copyright (c) 2026 bhgv
# EN: Adapt native UTF-8 views to serialized creation and immutable shared reads.
# RU: Адаптация UTF-8 views к защищённому созданию и неизменяемому общему чтению.
import re


def adapt(code, relative_replace):
    """Return adapted owned text; reject pinned boundaries. Callbacks never throw
    while the native cache mutex is held; callers retain the String lifetime.
    RU: Вернуть адаптированный текст либо отказать при дрейфе границ. Пока mutex
    кэша захвачен, нет throw; вызывающий удерживает время жизни String.
    """
    start = code.index('char*\nstring2c(String *s)')
    end = code.index('\n}', start)+2
    body = code[start:end]
    body = relative_replace(body, 'string2c(String *s)', 'dvm_string_view_locked(String *s, const char **problem)', 1)
    body = 'static '+body
    body = relative_replace(body, "\t\t((char*)address)[length] = '\\0';", "\t\tif(((char*)address)[length] != '\\0') {\n\t\t\t((char*)address)[length] = '\\0';\n\t\t}", 1)
    body = relative_replace(body, '\t\tfree(cache);', '\t\tmemset(buffer, 0, l);\n\t\tfree(cache);', 1)
    before = '''\t\tif(c < Runeself)
\t\t\t*p++ = c;
\t\telse
\t\t\tp += runetochar(p, &c);
\t}
\t*p = 0;'''
    after = '''        char encoded[UTFmax];
        int width;
        if(c < Runeself) {
            encoded[0] = c;
            width = 1;
        } else {
            width = runetochar(encoded, &c);
        }
        /* Readers may retain a returned view: never rewrite matching bytes. */
        if(memcmp(p, encoded, width) != 0) {
            memmove(p, encoded, width);
        }
        p += width;
\t}
    if(*p != 0) {
        *p = 0;
    }'''
    body = relative_replace(body, before, after, 1)
    body, count = re.subn(r'^(\t+)if([^\n]*?)\n\1\terror\((ex\w+)\);',
                         r'\1if\2 {\n\1\t*problem = \3;\n\1\treturn nil;\n\1}', body, flags=re.M)
    if count != 8 or 'error(' in body:
        raise ValueError('UTF-8 view error boundaries changed')
    wrapper = """

char*
dvm_runtime_string_view(String *s, const char **problem)
{
    char *view;
    dvm_string_view_lock();
    view = dvm_string_view_locked(s, problem);
    dvm_string_view_unlock();
    return view;
}

char*
string2c(String *s)
{
    const char *problem = NULL;
    char *view = dvm_runtime_string_view(s, &problem);
    if(problem) {
        error((char *)problem);
    }
    return view;
}"""
    return '#include "string_view_sync.h"\n' + code[:start] + body + wrapper + code[end:]
