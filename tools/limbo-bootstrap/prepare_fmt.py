# Copyright (c) 2026 bhgv
# EN: Protect formatter publication and lookup while returning callbacks unlocked.
# RU: Защитить публикацию/поиск форматтеров, возвращая callback без mutex.
import argparse
from pathlib import Path

REPLACEMENT = r'''/* EN: Install validated c/f under the registry mutex; no callback is invoked.
 * RU: Установить проверенные c/f под mutex реестра, не вызывая callback. */
static int
fmtinstalllocked(int c, Fmts f)
{
    Convfmt *p, *ep;
    ep = &fmtalloc.fmt[fmtalloc.nfmt];
    for(p = fmtalloc.fmt; p < ep; p++){
        if(p->c == c){
            break;
        }
    }
    if(p == &fmtalloc.fmt[Maxfmt]){
        return -1;
    }
    p->fmt = f;
    if(p == ep){
        p->c = c;
        fmtalloc.nfmt++;
    }
    return 0;
}

/* EN: Copy c's callback under the same mutex as publication, returning unlocked.
 * Lazy known formats are registered atomically; full tables retain builtin fallback.
 * RU: Скопировать callback для c под mutex публикации и вернуть без mutex.
 * Ленивые форматы регистрируются атомарно; полный реестр сохраняет builtin fallback. */
static Fmts
fmtfmt(int c)
{
    Convfmt *p, *ep;
    Fmts result;
    _fmtlock();
    ep = &fmtalloc.fmt[fmtalloc.nfmt];
    for(p = fmtalloc.fmt; p < ep; p++){
        if(p->c == c){
            result = p->fmt;
            _fmtunlock();
            return result;
        }
    }
    for(p = knownfmt; p->c; p++){
        if(p->c == c){
            result = p->fmt;
            fmtinstalllocked(c, result);
            _fmtunlock();
            return result;
        }
    }
    _fmtunlock();
    return _badfmt;
}

/* EN: Validate c/f and publish one registry entry; callback code runs elsewhere.
 * Return the original success/failure status, preserving NULL's bad-format fallback.
 * RU: Проверить c/f и опубликовать запись; callback выполняется снаружи.
 * Вернуть исходный статус, сохранив замену NULL на bad-format callback. */
int
fmtinstall(int c, Fmts f)
{
    int result;
    if(c <= 0 || c >= 65536){
        return -1;
    }
    if(!f){
        f = _badfmt;
    }
    _fmtlock();
    result = fmtinstalllocked(c, f);
    _fmtunlock();
    return result;
}
'''  # Build-owned registry routines; formatter ABI and surrounding source remain unchanged.


# EN: Replace reviewed private registry routines in source or reject boundary drift.
# RU: Заменить проверенные операции реестра в source либо отвергнуть расхождение.
def prepare(source):
    old_comment = '/* for spin lock in fmtfmt; avoids race due to write order */'
    if source.count(old_comment) != 1:
        raise ValueError('native formatter publication comment boundary drift')
    source = source.replace(old_comment, '/* Registry mutex protects callback publication and lookup. */')
    start = source.index('static Fmts\nfmtfmt(int c)\n{')
    end = source.index('/* Reject widths and precisions', start)
    old = source[start:end]
    if (old.count('_fmtlock();') != 1 or old.count('_fmtunlock();') != 2
            or old.count('fmtinstall(int c, Fmts f)') != 1):
        raise ValueError('native format registry boundary drift')
    return source[:start] + REPLACEMENT + '\n' + source[end:]


# EN: CLI supplies original source and build-owned output; never modify imports.
# RU: CLI задаёт исходный source и личный output сборки; импорт не менять.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(prepare(args.source.read_text()))


if __name__ == '__main__':
    main()
