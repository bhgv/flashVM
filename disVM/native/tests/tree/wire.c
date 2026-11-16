/* Copyright (c) 2026 bhgv
 * EN: Original Styx TCP callbacks driving prepared VFS node/directory capabilities.
 * RU: Исходные Styx TCP callbacks для prepared VFS node/directory capabilities.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "tree.h"
#define Controlqid 0x7fffffffffffffffULL
static Styxserver server;
static StyxBacking *nodes[16]; /* Only visited fixture nodes; never an enumerated tree. */
static int stopping;
extern void treechange(int action);

/* EN/RU: Drop visited capabilities; opened fids independently pin their providers.
 * Поглотить посещённые capabilities; открытые fid независимо удерживают providers. */
static void drop(void)
{
	int i;
	for(i = 0; i < 16; i++){ styxbackingfree(nodes[i]); nodes[i] = nil; }
}

/* EN/RU: Map only this fixture's declared opaque IDs; zero is the protocol root.
 * Сопоставить только opaque IDs теста; ноль — корень протокола. */
static unsigned indexof(Qid q) { return q.path < 16 ? q.path : 16; }

/* EN/RU: Walk one component in the provider root; names never become host paths.
 * Пройти один компонент внутри provider root; имена не становятся host paths. */
static char *walk(Qid *qid, char *name)
{
	StyxBacking *child;
	StyxMeta meta;
	unsigned index = indexof(*qid);
	if(qid->path == Qroot && !strcmp(name, "ctl")){ *qid = styxqid(Controlqid, 0); return nil; }
	if(index >= 16 || !nodes[index]) return Enonexist;
	child = styxwalknode(nodes[index], name);
	if(!child || !styxbackinginfo(child, &meta)){ styxbackingfree(child); return Enonexist; }
	index = meta.identity == 1 ? 0 : (unsigned)meta.identity;
	if(index >= 16){ styxbackingfree(child); return Ebadarg; }
	styxbackingfree(nodes[index]); nodes[index] = child;
	*qid = styxqid(index, meta.directory);
	return nil;
}

/* EN/RU: Every file/directory fid receives its own actual VFS open.
 * Каждый file/directory fid получает отдельное настоящее VFS-открытие. */
static char *openfile(Qid *qid, int mode)
{
	unsigned index = indexof(*qid);
	StyxOpen *opened;
	if(qid->path == Controlqid) return mode == OWRITE ? nil : Eperm;
	if(index >= 16 || !nodes[index]) return Enonexist;
	opened = styxfileopen(nodes[index], mode);
	if(!opened) return Eperm;
	if(styxsetaux(&server, opened) < 0){ styxfileclose(opened); return Eperm; }
	return nil;
}

/* EN/RU: Convert copied metadata to ordinary 9P stat fields, preserving names/modes.
 * Преобразовать metadata в обычные 9P stat-поля с именами и mode. */
static void attributes(const StyxMeta *meta, Dir *dir)
{
	memset(dir, 0, sizeof(*dir));
	dir->qid = styxqid(meta->identity == 1 ? 0 : meta->identity, meta->directory);
	dir->mode = meta->mode | (meta->directory ? DMDIR : 0);
	dir->length = meta->length;
	dir->name = (char*)meta->name;
	dir->uid = dir->gid = dir->muid = "fixture";
}

/* EN/RU: Publish full records only; peek is committed only after complete encoding.
 * Публиковать только целые записи; peek продвигается после полной упаковки. */
static char *readdir(StyxOpen *opened, char *buf, ulong *n, vlong offset)
{
	StyxMeta meta;
	Dir dir;
	uint size;
	ulong used = 0, capacity = *n;
	int status;
	if(offset < 0 || !styxdirbegin(opened, offset)) return Eperm;
	while(used < capacity){
		status = styxdirpeek(opened, &meta);
		if(status < 0){ if(!used) return Eperm; break; }
		if(!status) break;
		attributes(&meta, &dir);
		size = sizeD2M(&dir);
		if(size > capacity-used){ if(!used) return "directory read too small"; break; }
		if(convD2M(&dir, (uchar*)buf+used, capacity-used) != size || !styxdiradvance(opened, size)) return Ebadarg;
		used += size;
	}
	*n = used;
	return nil;
}

/* EN/RU: Retained object reads use existing precise-chunk semantics, not path reopening.
 * Читать удержанный объект существующими precise-chunk операциями, без path reopen. */
static char *readfile(Qid qid, char *buf, ulong *n, vlong offset)
{
	StyxOpen *opened = styxgetaux(&server);
	StyxTransfer result;
	if(qid.type&QTDIR) return readdir(opened, buf, n, offset);
	if(offset < 0) return Ebadarg;
	styxfileread(opened, offset, buf, *n, &result);
	if(!result.confirmed || result.uncertain || (!result.success && !result.count)) return Eperm;
	*n = result.count;
	return nil;
}

/* EN/RU: stat name storage must outlive its callback until the reply is encoded.
 * Хранение stat-имени живёт до упаковки ответа после callback. */
static char *statfile(Qid qid, Dir *dir)
{
	static StyxMeta meta; /* Isolated fixture owner, no cross-thread use. */
	StyxOpen *opened = styxgetaux(&server);
	unsigned index = indexof(qid);
	if(!(opened ? styxfileinfo(opened, &meta) : index < 16 && styxbackinginfo(nodes[index], &meta))) return Enonexist;
	attributes(&meta, dir);
	return nil;
}

/* EN/RU: Test-only owner commands mutate the live provider, never guest state.
 * Тестовое управление владельца меняет живой provider, не guest state. */
static char *writefile(Qid qid, char *buf, ulong *n, vlong offset)
{
	static const char *commands[] = {"hide", "show", "add", "deny", "allow", "replace"};
	int i;
	if(qid.path != Controlqid || offset != 0) return Eperm;
	if(*n == 4 && !memcmp(buf, "drop", 4)){ drop(); return nil; }
	if(*n == 4 && !memcmp(buf, "stop", 4)){ stopping = 1; return nil; }
	for(i = 0; i < 6; i++) if(*n == strlen(commands[i]) && !memcmp(buf, commands[i], *n)){
		treechange(i+1); return nil;
	}
	return Ebadarg;
}

/* EN/RU: The test owner explicitly provides one preapproved tree grant to TCP.
 * Владелец теста явно предоставляет один заранее разрешённый tree grant для TCP. */
int treewire(StyxBacking *root)
{
	Styxops ops;
	struct sockaddr_in address;
	socklen_t size = sizeof(address);
	nodes[0] = root;
	memset(&ops, 0, sizeof(ops));
	ops.walk = walk; ops.open = openfile; ops.read = readfile; ops.write = writefile;
	ops.stat = statfile; ops.freefid = styxfileclose;
	if(styxinit(&server, &ops, "0", 0555, 0) != nil) return 1;
	if(getsockname(server.connfd, (struct sockaddr*)&address, &size) < 0) return 2;
	print("%d\n", ntohs(address.sin_port));
	while(!stopping) if(styxwait(&server) != nil || styxprocess(&server) != nil) return 3;
	if(styxend(&server) != nil) return 4;
	drop();
	return 0;
}
