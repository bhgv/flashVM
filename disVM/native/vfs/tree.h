/* Copyright (c) 2026 bhgv
 * EN: Confined node walking and live provider-backed directory peek/commit.
 * RU: Ограниченный корнем обход node и живой каталог с peek/commit.
 */
#ifndef STYX_TREE_H
#define STYX_TREE_H
#include "file.h"
/* EN/RU: Walk one component; dotdot clamps at the prepared provider root.
 * Owned node/NULL; no raw host-path lookup; effective parent execute is required.
 * Пройти один компонент; dotdot ограничен prepared root. Свой node/NULL. */
StyxBacking *styxwalknode(StyxBacking *node, const char *component);
/* EN: Exclusively create one named child, adopting its open cursor and returning
 * its captured node via out. The trusted provider must check permissions in wx.
 * RU: Исключительно создать потомка, выдать его курсор и удержанный узел в out.
 * Доверенный провайдер обязан проверить права в режиме wx. */
StyxOpen *styxcreatefile(StyxBacking *parent, const char *name, StyxBacking **out);

/* EN: Compare the retained provider, path and captured object identity.
 * RU: Сравнить удерживаемый провайдер, путь и захваченную идентичность. */
int styxbackingsamenode(const StyxBacking *left, const StyxBacking *right);

/* EN/RU: Begin one directory read at exact continuation offset, or restart at zero.
 * Rewind reopens only through the retained provider and must preserve object identity.
 * Начать чтение по точному offset или заново с нуля; identity при rewind сохраняется. */
int styxdirbegin(StyxOpen *opened, unsigned long long offset);
/* EN/RU: Peek one current child:1 record,0 actual EOF,-1 error; output only on1.
 * Recheck current visibility/metadata without consuming a record or leaking hidden names.
 * Получить одну запись:1 запись,0 EOF,-1 ошибка; без продвижения и скрытых имён. */
int styxdirpeek(StyxOpen *opened, StyxMeta *output);
/* EN/RU: Commit exactly one peeked record after successful full encoding/copy.
 * bytes is its actual wire length; return1/0, no provider I/O.
 * Продвинуть одну просмотренную запись после полной упаковки; bytes — её wire-длина. */
int styxdiradvance(StyxOpen *opened, size_t bytes);
#endif
