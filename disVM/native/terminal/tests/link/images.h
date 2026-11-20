/* Copyright (c) 2026 bhgv
 * EN: Styx fixture images and the process directory tree.
 * RU: Образы стенда Styx и дерево каталогов процессов.
 */
#ifndef STYX_TERMINAL_FIXTURE_IMAGES_H
#define STYX_TERMINAL_FIXTURE_IMAGES_H


/* EN/RU: Own actual compiled readonly asset storage through original server join.
 * Владеть настоящим compiled readonly asset-storage до join original server.
 */
extern void fixtureimagesinit(Styxserver *);
extern Path fixtureprograminit(Styxserver *); /* Set up the two scoped contexts. */


/* EN/RU: Serve one original remote offset/count, borrowed buffers, short prefixes only.
 * Обслужить исходные remote offset/count, buffers borrowed, только short prefixes.
 */
extern char *fixtureimagesread(Qid, char *, ulong *, vlong);


/* EN/RU: Release image storage after every original server callback has retired.
 * Освободить image-storage после завершения всех original server callbacks.
 */
extern void fixtureimagesfree(void);
#endif
