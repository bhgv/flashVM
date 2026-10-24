implement Cocor;
include "sys.m";
include "draw.m";
sys: Sys;
Cocor: module { init: fn(nil: ref Draw->Context, args: list of string); };

T, NT, ALT, OPT, REP, SEM, RES, ANY, SYNC, WT, CH: con iota+1;
Node: adt {
 k, id, nullable: int;
 text: string;
 a, b, next: ref Node;
 first, chars: array of int;
 context: int;
};
Production: adt {
 name, attr: string;
 body: ref Node;
 first, follow: array of int;
 nullable, defined, derives: int;
};
Token: adt { name, gen, literal: string; body: ref Node; pragma: string; };
Charset: adt { name: string; bits: array of int; };
prods: array of ref Production;
tokens: array of ref Token;
sets: list of ref Charset;
aliases: list of (string,string);
strict: int;
coms: list of (string,string,int);
ignore, allsync: array of int;
source, filename, compiler, globals, prefix, backend, runtimepath: string;
pos, start, finish, kind, nocase, errors: int;
look, value: string;
output: ref Sys->FD;
trans: list of (int,int,int,int,int);
accepts: list of (int,int);
nstates, no_sym: int;

fatal(s: string)
{
 line := 1; col := 1;
 for (i := 0; i < start && i < len source; i++) { if (source[i] == '\n') { line++; col = 1; } else col++; }
 sys->fprint(sys->fildes(2), "%s:%d:%d: %s\n", filename, line, col, s);
 raise "fail:cocor";
}
readfile(path: string): string
{
 fd := sys->open(path, Sys->OREAD);
 if (fd == nil) fatal("cannot open " + path);
 (ok, st) := sys->fstat(fd);
 if (ok < 0 || st.length > big Sys->Maxint) fatal("cannot stat " + path);
 data := array[int st.length] of byte;
 if (sys->readn(fd, data, len data) != len data) fatal("cannot read " + path);
 return string data;
}
isid(c: int): int { return c == '_' || c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= 128; }
starts(p: int, s: string): int { return p + len s <= len source && source[p:p+len s] == s; }
skipquoted(p: int): int
{
 q := source[p++];
 while (p < len source) {
  c := source[p++];
  if (c == '\\') { if (p < len source) p++; }
  else if (c == q) return p;
 }
 fatal("unterminated string"); return p;
}
skipcomment(p: int): int
{
 if (starts(p, "//")) { while (p < len source && source[p] != '\n') p++; return p; }
 p += 2; level := 1;
 while (p < len source && level) {
  if (starts(p, "/*")) { level++; p += 2; }
  else if (starts(p, "*/")) { level--; p += 2; }
  else p++;
 }
 if (level) fatal("unterminated comment");
 return p;
}
next()
{
 for (;;) {
  while (pos < len source && source[pos] <= ' ') pos++;
  if (starts(pos, "/*") || starts(pos, "//")) pos = skipcomment(pos);
  else if (backend == "limbo" && pos < len source && source[pos] == '#') { while (pos < len source && source[pos] != '\n') pos++; }
  else break;
 }
 start = pos; value = nil; kind = 0;
 if (pos >= len source) { look = "<eof>"; finish = pos; return; }
 c := source[pos++];
 if (isid(c)) {
  while (pos < len source && (isid(source[pos]) || source[pos] >= '0' && source[pos] <= '9')) pos++;
  kind = 1;
 } else if (c >= '0' && c <= '9') {
  while (pos < len source && source[pos] >= '0' && source[pos] <= '9') pos++;
  kind = 2;
 } else if (c == '"' || c == '\'') {
  pos = skipquoted(start); kind = 3;
  raw := source[start+1:pos-1];
  for (i := 0; i < len raw; i++) {
   ch := raw[i];
   if (ch == '\\' && i+1 < len raw && (raw[i+1] == '\\' || raw[i+1] == '\'' || raw[i+1] == '"')) ch = raw[++i];
   value[len value] = ch;
  }
 } else if ((c == '<' && pos < len source && source[pos] == '.') || (c == '.' && pos < len source && (source[pos] == '.' || source[pos] == '>'))) pos++;
 else if (c == '(' && pos < len source && source[pos] == '.') {
  pos++; body := pos;
  while (pos < len source && !starts(pos, ".)")) {
   if (source[pos] == '"' || source[pos] == '\'') pos = skipquoted(pos);
   else if (starts(pos, "/*") || starts(pos, "//")) pos = skipcomment(pos);
  else if (backend == "limbo" && pos < len source && source[pos] == '#') { while (pos < len source && source[pos] != '\n') pos++; }
   else if (backend == "limbo" && source[pos] == '#') { while (pos < len source && source[pos] != '\n') pos++; }
   else pos++;
  }
  if (pos >= len source) fatal("unterminated semantic action");
  value = source[body:pos]; pos += 2; kind = 4;
 }
 look = source[start:pos]; finish = pos;
}
take(s: string): int { if (look != s) return 0; next(); return 1; }
need(s: string) { if (!take(s)) fatal("expected " + s + ", found " + look); }
ident(): string { if (kind != 1) fatal("expected identifier"); s := look; next(); return s; }
capture(op, cl: string): string
{
 if (look != op) fatal("expected " + op);
 p := finish; begin := p; depth := 1;
 while (p < len source) {
  if (source[p] == '"' || source[p] == '\'') p = skipquoted(p);
  else if (starts(p, "/*") || starts(p, "//")) p = skipcomment(p);
  else if (backend == "limbo" && source[p] == '#') { while (p < len source && source[p] != '\n') p++; }
  else if (cl == ">" && source[p] == '>' && p > begin && source[p-1] == '-') p++;
  else if (starts(p, cl)) {
   if (--depth == 0) { s := source[begin:p]; pos = p+len cl; next(); return s; }
   p += len cl;
  } else if (starts(p, op)) { depth++; p += len op; }
  else p++;
 }
 fatal("unclosed " + op); return nil;
}
attributes(): string
{
 if (look == "<.") return capture("<.", ".>");
 return capture("<", ">");
}
newnode(k: int): ref Node { n := ref Node; n.k = k; return n; }
addprod(name: string): int
{
 for (i := 0; i < len prods; i++) if (prods[i].name == name) return i;
 p := ref Production; p.name = name;
 a := array[len prods+1] of ref Production; a[0:] = prods; a[len prods] = p; prods = a;
 return len prods-1;
}
findtoken(name: string): int
{
 for (i := 1; i < len tokens; i++) if (tokens[i].name == name) return i;
 return -1;
}
fold(c: int): int { if (nocase && c >= 'a' && c <= 'z') return c-32; return c; }
foldstr(s: string): string
{
 b := array of byte s; for (i := 0; i < len b; i++) b[i] = byte fold(int b[i]); return string b;
}
symbolname(literal: string, id: int): string
{
 chars := " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
 names := array[] of {"_","Bang","Dquote","Hash","Dollar","Percent","And","Squote","Lparen","Rparen","Star","Plus","Comma","Minus","Point","Slash","Colon","Semicolon","Less","Equal","Greater","Query","At","Lbrack","Backslash","Rbrack","Uparrow","Underscore","Accent","Lbrace","Bar","Rbrace","Tilde"};
 b := array of byte literal; r: string;
 for (i := 0; i < len b; i++) {
  c := int b[i]; mapped := 0;
  for (j := 0; j < len chars; j++) if (chars[j] == c) { r += names[j]; mapped = 1; break; }
  if (mapped) continue;
  if (c >= '0' && c <= '9') r += "D" + string array[] of {byte c};
  else if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') r += string array[] of {byte c};
  else r += "Asc" + string c;
 }
 if (len r > 28) return r[0:17]+"_"+string id+"Sym";
 return r+"Sym";
}
addtoken(name, literal: string): int
{
 id := findtoken(name); if (id >= 0) return id;
 if (literal != nil) for (i := 1; i < len tokens; i++) if (tokens[i].literal == literal) return i;
 t := ref Token; t.name = name; t.literal = literal;
 if (len name > 0 && (name[0] == '"' || name[0] == '\'')) t.gen = symbolname(literal,len tokens);
 else t.gen = name+"Sym";
 for (l := aliases; l != nil; l = tl l) { (key,alias) := hd l; if (key == name || literal != nil && key == "@"+literal) t.gen = alias; }
 a := array[len tokens+1] of ref Token; a[0:] = tokens; a[len tokens] = t; tokens = a;
 return len tokens-1;
}
literalnode(s: string): ref Node
{
 b := array of byte s; head, tail: ref Node;
 if (len b == 0) fatal("empty token");
 for (i := 0; i < len b; i++) {
  n := newnode(CH); n.chars = array[256] of int; n.chars[fold(int b[i])] = 1;
  if (head == nil) head = n; else tail.next = n;
  tail = n;
 }
 return head;
}
setatom(): array of int
{
 s := array[256] of int;
 if (take("ANY")) { for (i := 1; i < 256; i++) s[i] = 1; return s; }
 if (take("CHR")) {
  need("("); if (kind != 2) fatal("expected byte number"); n := int look; next(); need(")");
  if (n < 0 || n > 255) fatal("CHR outside byte range"); s[fold(n)] = 1;
 } else if (kind == 3) {
  b := array of byte value; next();
  for (i := 0; i < len b; i++) s[fold(int b[i])] = 1;
 } else {
  name := ident();
  for (l := sets; l != nil; l = tl l) { cs := hd l; if (cs.name == name) { s[0:] = cs.bits; return s; } }
  fatal("unknown character set " + name);
 }
 if (take("..")) {
  t := setatom(); lo := -1; hi := -1;
  for (i := 0; i < 256; i++) { if (s[i]) { if (lo >= 0) fatal("range needs singleton"); lo = i; } if (t[i]) { if (hi >= 0) fatal("range needs singleton"); hi = i; } }
  if (lo < 0 || hi < lo) fatal("invalid range");
  for (i = lo; i <= hi; i++) s[i] = 1;
 }
 return s;
}
charset(): array of int
{
 s := setatom();
 while (look == "+" || look == "-") {
  op := look; next(); t := setatom();
  for (i := 0; i < 256; i++) if (t[i]) s[i] = op == "+";
 }
 return s;
}
regexatom(): ref Node
{
 n: ref Node;
 if (look == "(" || look == "[" || look == "{") {
  op := look; next(); inner := regex();
  if (op == "(") { need(")"); return inner; }
  if (op == "[") { need("]"); n = newnode(OPT); }
  else { need("}"); n = newnode(REP); }
  n.a = inner; return n;
 }
 if (kind == 3) { s := value; next(); return literalnode(s); }
 n = newnode(CH); n.chars = setatom(); return n;
}
regexseq(): ref Node
{
 head, tail: ref Node;
 while (kind == 3 || kind == 1 && look != "CONTEXT" || look == "(" || look == "[" || look == "{") {
  n := regexatom();
  if (head == nil) head = n; else tail.next = n;
  tail = n; while (tail.next != nil) tail = tail.next;
 }
 return head;
}
regex(): ref Node
{
 n := regexseq();
 if (!take("|")) return n;
 a := newnode(ALT); a.a = n; a.b = regex(); return a;
}
markcontext(n: ref Node)
{
 for (; n != nil; n = n.next) { n.context = 1; if (n.a != nil) markcontext(n.a); if (n.b != nil) markcontext(n.b); }
}
prodseq(): ref Node
{
 head, tail: ref Node;
 while (look != "." && look != "|" && look != ")" && look != "]" && look != "}" && look != "<eof>") {
  n: ref Node;
  if (kind == 4) { n = newnode(SEM); n.text = value; next(); }
  else if (take("IF")) { n = newnode(RES); n.text = capture("(", ")"); if (len n.text == 0) fatal("empty resolver"); }
  else if (take("ANY")) n = newnode(ANY);
  else if (take("SYNC")) n = newnode(SYNC);
  else if (look == "(" || look == "[" || look == "{") {
   op := look; next(); inner := expression();
   if (op == "(") { need(")"); n = inner; }
   else { n = newnode(OPT); if (op == "{") { n.k = REP; need("}"); } else need("]"); n.a = inner; }
  } else {
   weak := take("WEAK");
   n = newnode(T);
   if (kind == 3) { name := look; lit := value; next(); n.id = addtoken(name,foldstr(lit)); }
   else {
    name := ident(); id := findtoken(name);
    if (id >= 0) n.id = id;
    else { n.k = NT; n.id = addprod(name); }
   }
   if (weak) { if (n.k != T) fatal("WEAK needs terminal"); n.k = WT; }
   if (look == "<" || look == "<.") { if (n.k != NT) fatal("terminal has attributes"); n.text = attributes(); }
  }
  if (n == nil) continue;
  if (head == nil) head = n; else tail.next = n;
  tail = n; while (tail.next != nil) tail = tail.next;
 }
 return head;
}
expression(): ref Node
{
 n := prodseq();
 if (!take("|")) return n;
 a := newnode(ALT); a.a = n; a.b = expression(); return a;
}
section(): int
{
 return look == "CHARACTERS" || look == "TOKENS" || look == "PRAGMAS" || look == "COMMENTS" || look == "IGNORE" || look == "NAMES" || look == "PRODUCTIONS";
}
grammar()
{
 while (look != "COMPILER" && look != "<eof>") next();
 need("COMPILER"); compiler = ident(); addprod(compiler);
 begin := start;
 while (!section() && look != "<eof>") next();
 globals = source[begin:start];
 while (look != "PRODUCTIONS") {
  if (take("CHARACTERS")) {
   while (kind == 1 && !section()) {
    cs := ref Charset; cs.name = ident(); need("="); cs.bits = charset(); need(".");
    for (l := sets; l != nil; l = tl l) if ((hd l).name == cs.name) fatal("duplicate set " + cs.name);
    sets = cs :: sets;
   }
  } else if (look == "TOKENS" || look == "PRAGMAS") {
   pragma := look == "PRAGMAS"; next();
   while (kind == 1 && !section()) {
    name := ident(); if (findtoken(name) >= 0) fatal("duplicate token " + name);
    id := addtoken(name,nil); need("="); tokens[id].body = regex();
    if (take("CONTEXT")) {
     need("("); ctx := regex(); need(")"); markcontext(ctx);
     n := tokens[id].body; if (n == nil) fatal("empty token"); while (n.next != nil) n = n.next; n.next = ctx;
    }
    need(".");
    if (pragma) { tokens[id].pragma = " "; if (kind == 4) { tokens[id].pragma = value; next(); } }
   }
  } else if (take("COMMENTS")) {
   need("FROM"); op := commentword(); need("TO"); cl := commentword(); nested := take("NESTED");
   if (len op == 0 || len cl == 0) fatal("empty comment delimiter");
   coms = (op,cl,nested) :: coms;
  } else if (take("IGNORE")) {
   if (take("CASE")) {
    if (len tokens > 1 || sets != nil) fatal("IGNORE CASE must precede sets and tokens"); nocase = 1;
   } else { s := charset(); for (i := 0; i < 256; i++) if (s[i]) ignore[i] = 1; }
  } else if (take("NAMES")) {
   while (kind == 1 && !section()) {
    name := ident(); need("="); if (kind != 3 && kind != 1) fatal("NAMES needs a symbol");
    key := look; if (kind == 3) key = "@"+foldstr(value);
    aliases = (key,name) :: aliases;
    id := findtoken(look); if (id >= 0) tokens[id].gen = name;
    next(); need(".");
   }
  } else fatal("expected declaration, found " + look);
 }
 need("PRODUCTIONS");
 while (look != "END" && look != "<eof>") {
  id := addprod(ident()); p := prods[id]; if (p.defined) fatal("duplicate production " + p.name); p.defined = 1;
  if (look == "<" || look == "<.") p.attr = attributes();
  need("=");
  lead: ref Node;
  if (kind == 4) { lead = newnode(SEM); lead.text = value; next(); }
  p.body = expression();
  if (lead != nil) { lead.next = p.body; p.body = lead; }
  need(".");
 }
 need("END"); if (ident() != compiler) fatal("compiler name differs at END"); need(".");
 if (look != "<eof>") fatal("trailing grammar text");
 if (prods[0].attr != nil) fatal("start production cannot have attributes");
}
commentword(): string
{
 if (kind == 3) { s := value; next(); return s; }
 s := setatom(); c := -1;
 for (i := 0; i < 256; i++) if (s[i]) { if (c >= 0) fatal("comment delimiter must be a string or singleton set"); c = i; }
 if (c < 0) fatal("empty comment delimiter");
 return string array[] of {byte c};
}

newset(): array of int { return array[len tokens+1] of int; }
union(a, b: array of int): int
{
 changed := 0;
 for (i := 0; i < len a; i++) if (b[i] && !a[i]) { a[i] = 1; changed = 1; }
 return changed;
}
nullable(n: ref Node): int
{
 for (; n != nil; n = n.next) {
  case n.k {
  T or WT or CH or ANY => return 0;
  NT => if (!prods[n.id].nullable) return 0;
  ALT => if (!nullable(n.a) && !nullable(n.b)) return 0;
  OPT or REP or SEM or RES or SYNC => ;
  }
 }
 return 1;
}
first(n: ref Node): array of int
{
 s := newset();
 for (; n != nil; n = n.next) {
  case n.k {
  T or WT => s[n.id] = 1; return s;
  NT => union(s,prods[n.id].first); if (!prods[n.id].nullable) return s;
  ALT => union(s,first(n.a)); union(s,first(n.b)); if (!nullable(n.a) && !nullable(n.b)) return s;
  OPT or REP => union(s,first(n.a));
  ANY => union(s,n.chars); return s;
  }
 }
 return s;
}
follow(n: ref Node, after: array of int): int
{
 changed := 0;
 for (; n != nil; n = n.next) {
  s := first(n.next); if (nullable(n.next)) union(s,after);
  case n.k {
  NT => changed |= union(prods[n.id].follow,s);
  ALT => changed |= follow(n.a,s); changed |= follow(n.b,s);
  OPT => changed |= follow(n.a,s);
  REP => union(s,first(n.a)); changed |= follow(n.a,s);
  }
 }
 return changed;
}
left(n: ref Node, target: int, seen: array of int): int
{
 for (; n != nil; n = n.next) {
  if (n.k == NT) {
   if (n.id == target) return 1;
   if (!seen[n.id]) { seen[n.id] = 1; if (left(prods[n.id].body,target,seen)) return 1; }
   if (!prods[n.id].nullable) return 0;
  } else if (n.k == ALT) { if (left(n.a,target,seen) || left(n.b,target,seen)) return 1; if (!nullable(n.a) && !nullable(n.b)) return 0; }
  else if (n.k == OPT || n.k == REP) { if (left(n.a,target,seen)) return 1; }
  else if (n.k == T || n.k == WT || n.k == ANY) return 0;
 }
 return 0;
}
conflict(s: string)
{
 if (strict) fatal(s);
 sys->fprint(sys->fildes(2), "%s: warning: %s\n",filename,s);
}
check(n: ref Node, after: array of int)
{
 for (; n != nil; n = n.next) {
  s := first(n.next); if (nullable(n.next)) union(s,after);
  if (n.k == ALT) {
   a := first(n.a); b := first(n.b);
   if (nullable(n.a)) union(a,s); if (nullable(n.b)) union(b,s);
   guarded := n.a != nil && n.a.k == RES;
   for (i := 0; i < len a; i++) if (a[i] && b[i] && !guarded) conflict("LL(1) conflict on token " + string i);
   check(n.a,s); check(n.b,s);
  } else if (n.k == OPT || n.k == REP) {
   if (nullable(n.a)) fatal("nullable option/repetition");
   a := first(n.a); guarded := n.a != nil && n.a.k == RES;
   for (i := 0; i < len a; i++) if (a[i] && s[i] && !guarded) conflict("LL(1) conflict in option/repetition");
   if (n.k == REP) union(s,a); check(n.a,s);
  }
 }
}
validresolvers(n: ref Node, allowed: int)
{
 firstnode := 1;
 for (; n != nil; n = n.next) {
  if (n.k == RES && (!allowed || !firstnode)) fatal("IF must lead an alternative, option or repetition");
  if (n.k == ALT) { validresolvers(n.a,1); validresolvers(n.b,1); }
  else if (n.k == OPT || n.k == REP) validresolvers(n.a,1);
  firstnode = 0;
 }
}
derive(n: ref Node): int
{
 for (; n != nil; n = n.next) {
  if (n.k == NT && !prods[n.id].derives) return 0;
  if (n.k == ALT && !derive(n.a) && !derive(n.b)) return 0;
 }
 return 1;
}
initany(n: ref Node)
{
 for (; n != nil; n = n.next) {
  if (n.k == ANY) {
   n.chars = newset();
   for (i := 1; i < len tokens; i++) if (tokens[i].pragma == nil) n.chars[i] = 1;
  }
  if (n.a != nil) initany(n.a); if (n.b != nil) initany(n.b);
 }
}
leadingany(n: ref Node): ref Node
{
 for (; n != nil; n = n.next) {
  if (n.k == ANY) return n;
  if (n.k == OPT || n.k == REP || n.k == ALT) {
   a := leadingany(n.a); if (a != nil) return a;
   if (n.k == ALT) { a = leadingany(n.b); if (a != nil) return a; }
  }
  if (n.k == T || n.k == WT || n.k == NT && !prods[n.id].nullable) break;
 }
 return nil;
}
exclude(a: ref Node,s: array of int): int
{
 if (a == nil) return 0;
 changed := 0;
 for (i := 0; i < len s; i++) if (s[i] && a.chars[i]) { a.chars[i] = 0; changed = 1; }
 return changed;
}
refineany(n: ref Node,after: array of int): int
{
 changed := 0;
 for (; n != nil; n = n.next) {
  s := first(n.next); if (nullable(n.next)) union(s,after);
  if (n.k == OPT || n.k == REP) {
   changed |= refineany(n.a,s);
   changed |= exclude(leadingany(n.a),s);
  } else if (n.k == ALT) {
   changed |= refineany(n.a,s); changed |= refineany(n.b,s);
   a := first(n.a); if (nullable(n.a)) union(a,s);
   b := first(n.b); if (nullable(n.b)) union(b,s);
   changed |= exclude(leadingany(n.a),b); changed |= exclude(leadingany(n.b),a);
  }
 }
 return changed;
}
collectsync(n: ref Node, after: array of int)
{
 for (; n != nil; n = n.next) {
  s := first(n.next); if (nullable(n.next)) union(s,after);
  if (n.k == SYNC) union(allsync,s);
  if (n.k == NT && ((n.text == nil) != (prods[n.id].attr == nil))) fatal("attribute mismatch for " + prods[n.id].name);
  if (n.k == ANY) { any := 0; for (i := 0; i < len n.chars; i++) any |= n.chars[i]; if (!any) conflict("empty ANY set"); }
  if (n.k == REP) union(s,first(n.a));
  if (n.a != nil) collectsync(n.a,s); if (n.b != nil) collectsync(n.b,s);
 }
}
setsfixedpoint()
{
 for (i := 0; i < len prods; i++) { prods[i].first = newset(); prods[i].follow = newset(); }
 changed := 1;
 while (changed) {
  changed = 0;
  for (i = 0; i < len prods; i++) { p := prods[i]; if (!p.nullable && nullable(p.body)) { p.nullable = 1; changed = 1; } changed |= union(p.first,first(p.body)); }
 }
 prods[0].follow[0] = 1; changed = 1;
 while (changed) { changed = 0; for (i = 0; i < len prods; i++) changed |= follow(prods[i].body,prods[i].follow); }
}
analyze()
{
 for (i := 0; i < len prods; i++) {
  p := prods[i]; if (!p.defined) fatal("undefined production " + p.name);
  initany(p.body);
 }
 changed := 1;
 while (changed) {
  changed = 0;
  for (i = 0; i < len prods; i++) if (!prods[i].derives && derive(prods[i].body)) { prods[i].derives = 1; changed = 1; }
 }
 for (i = 0; i < len prods; i++) if (!prods[i].derives) fatal("production cannot derive terminals: " + prods[i].name);
 for (;;) {
  setsfixedpoint(); changed = 0;
  for (i = 0; i < len prods; i++) changed |= refineany(prods[i].body,prods[i].follow);
  if (!changed) break;
 }
 allsync = newset(); allsync[0] = 1;
 for (i = 0; i < len prods; i++) {
  if (left(prods[i].body,i,array[len prods] of int)) fatal("left recursion in " + prods[i].name);
  check(prods[i].body,prods[i].follow);
  validresolvers(prods[i].body,0);
  collectsync(prods[i].body,prods[i].follow);
 }
}
state(): int { return nstates++; }
edge(f,t,lo,hi,ctx: int) { trans = (f,t,lo,hi,ctx) :: trans; }
nfa(n: ref Node, f,t: int)
{
 if (n == nil) { edge(f,t,-1,-1,0); return; }
 end := t; if (n.next != nil) end = state();
 case n.k {
 CH =>
  for (i := 0; i < 256; i++) if (n.chars[i]) { lo := i; while (i+1 < 256 && n.chars[i+1]) i++; edge(f,end,lo,i,n.context); }
 ALT => nfa(n.a,f,end); nfa(n.b,f,end);
 OPT => edge(f,end,-1,-1,0); nfa(n.a,f,end);
 REP =>
  a := state(); b := state(); edge(f,a,-1,-1,0); edge(f,end,-1,-1,0);
  nfa(n.a,a,b); edge(b,a,-1,-1,0); edge(b,end,-1,-1,0);
 }
 if (n.next != nil) nfa(n.next,end,t);
}
epsilon(s: array of int)
{
 changed := 1;
 while (changed) {
  changed = 0;
  for (l := trans; l != nil; l = tl l) {
   (f,t,lo,nil,nil) := hd l;
   if (lo == -1 && s[f] && !s[t]) { s[t] = 1; changed = 1; }
  }
 }
}
setkey(s: array of int): string
{
 b := array[len s] of byte;
 for (i := 0; i < len s; i++) b[i] = byte s[i];
 return string b;
}
determinize()
{
 # Trailing context keeps tagged NFA paths so token-end positions survive.
 for (l := trans; l != nil; l = tl l) { (nil,nil,nil,nil,c) := hd l; if (c) return; }
 nf := array[nstates] of int;
 for (a := accepts; a != nil; a = tl a) { (s,k) := hd a; nf[s] = k; }
 root := array[nstates] of int; root[0] = 1; epsilon(root);
 states := array[1] of {root}; keys := array[1] of {setkey(root)};
 dt: list of (int,int,int,int,int); da: list of (int,int);
 for (index := 0; index < len states; index++) {
  active := states[index]; best := 0; classseen := 0;
  for (j := 0; j < nstates; j++) if (active[j] && nf[j]) {
   k := nf[j];
   if (tokens[k].literal == nil) {
    if (classseen && classseen != k) fatal("indistinguishable tokens " + tokens[classseen].name + " and " + tokens[k].name);
    classseen = k;
   }
   if (best == 0) best = k;
   else if (best != k) {
    if (tokens[k].literal == nil && tokens[best].literal == nil)
     fatal("indistinguishable tokens " + tokens[best].name + " and " + tokens[k].name);
    if (tokens[k].literal != nil) best = k;
   }
  }
  if (best) da = (index,best) :: da;
  last := -1; begin := 0;
  for (c := 0; c <= 256; c++) {
   target := -1;
   if (c < 256) {
    dest := array[nstates] of int; any := 0;
    for (l := trans; l != nil; l = tl l) {
     (f,t,lo,hi,nil) := hd l;
     if (active[f] && lo >= 0 && c >= lo && c <= hi) { dest[t] = 1; any = 1; }
    }
    if (any) {
     epsilon(dest); key := setkey(dest);
     for (j = 0; j < len keys; j++) if (keys[j] == key) { target = j; break; }
     if (target < 0) {
      target = len states;
      ss := array[target+1] of array of int; ss[0:] = states; ss[target] = dest; states = ss;
      kk := array[target+1] of string; kk[0:] = keys; kk[target] = key; keys = kk;
     }
    }
   }
   if (target != last) {
    if (last >= 0) dt = (index,last,begin,c-1,0) :: dt;
    last = target; begin = c;
   }
  }
 }
 trans = dt; accepts = da; nstates = len states;
}

scanbuild()
{
 nstates = 1;
 for (i := 1; i < len tokens; i++) {
  t := tokens[i];
  if (t.body == nil && t.literal != nil) t.body = literalnode(t.literal);
  if (t.body == nil) fatal("token has no definition " + t.name);
  if (nullable(t.body)) fatal("nullable token " + t.name);
  end := state(); nfa(t.body,0,end); accepts = (end,i) :: accepts;
 }
 no_sym = len tokens;
 determinize();
}

put(s: string) { b := array of byte s; if (sys->write(output,b,len b) != len b) fatal("write output"); }
quote(s: string): string
{
 r := "\"";
 for (i := 0; i < len s; i++) {
  c := s[i];
  case c {
  '\\' => r += "\\\\";
  '"' => r += "\\\"";
  '\n' => r += "\\n";
  '\r' => r += "\\r";
  '\t' => r += "\\t";
  * => if (c < 32) r += sys->sprint("\\u%04x",c); else r[len r] = c;
  }
 }
 return r+"\"";
}
condition(n: ref Node, after: array of int): string
{
 if (n != nil && n.k == RES) return n.text;
 s := first(n); if (nullable(n)) union(s,after); return setcondition(s);
}
setcondition(s: array of int): string
{
 r: string;
 for (i := 0; i < len s; i++) if (s[i]) { if (r != nil) r += " || "; r += "Sym == "+string i; }
 if (r == nil) r = "0";
 return r;
}
setarray(s: array of int): string
{
 r := "array[] of {"; sep: string;
 for (i := 0; i < len s; i++) if (s[i]) { r += sep+string i; sep = ","; }
 if (sep == nil) r += "* => -1";
 if (backend == "c") {
  r = "(const int[]){";
  for (i := 0; i < len s; i++) if (s[i]) r += string i+",";
  r += "-1";
 }
 return r+"}";
}
emitgraph(n: ref Node, after: array of int)
{
 for (; n != nil; n = n.next) {
  s := first(n.next); if (nullable(n.next)) union(s,after);
  case n.k {
  T => put("Expect("+string n.id+");\n");
  WT =>
   put("ExpectWeak("+string n.id+", "+setarray(s)+");\n");
  NT => put(prods[n.id].name+"("+n.text+");\n");
  SEM => put(n.text+"\n");
  RES => ;
  ANY =>
   a := n.chars;
   put("if ("+setcondition(a)+") Get(); else SynError(No_Sym);\n");
  SYNC => put("while (Sym != 0 && !("+setcondition(s)+")) { SynError(No_Sym); Get(); }\n");
  OPT => put("if ("+condition(n.a,s)+") {\n"); emitgraph(n.a,s); put("}\n");
  REP =>
   if (n.a != nil && n.a.k == WT) {
    bodyfirst := first(n.a.next);
    if (nullable(n.a.next)) union(bodyfirst,s);
    put("while (WeakSeparator("+string n.a.id+", "+setarray(bodyfirst)+", "+setarray(s)+")) {\n");
    union(s,first(n.a)); emitgraph(n.a.next,s); put("}\n");
   } else {
    put("while ("+condition(n.a,s)+") {\n"); union(s,first(n.a)); emitgraph(n.a,s); put("}\n");
   }
  ALT =>
   put("if ("+condition(n.a,s)+") {\n"); emitgraph(n.a,s); put("} else {\n");
   if (n.b != nil && n.b.k == ALT) emitgraph(n.b,s);
   else { put("if ("+condition(n.b,s)+") {\n"); emitgraph(n.b,s); put("} else SynError(No_Sym);\n"); }
   put("}\n");
  }
 }
}
emittables()
{
 if (backend == "limbo") {
  put("tables() {\nedges = array[] of {\n"); sep: string;
  for (l := trans; l != nil; l = tl l) { (f,t,lo,hi,c) := hd l; put(sep+sys->sprint("(%d,%d,%d,%d,%d)\n",f,t,lo,hi,c)); sep = ","; }
  if (sep == nil) put("(0,0,-2,-2,0)\n");
  put("};\nfinals = array["+string nstates+"] of { * => 0 };\n");
  for (a := accepts; a != nil; a = tl a) { (s,k) := hd a; put(sys->sprint("finals[%d] = %d;\n",s,k)); }
  put("ignored = array[256] of { * => 0 };\nsyncset = "+setarray(allsync)+";\n");
  for (i := 0; i < 256; i++) if (ignore[i]) put(sys->sprint("ignored[%d] = 1;\n",i));
  nl := 0; for (i = 1; i < len tokens; i++) if (tokens[i].literal != nil) nl++;
  put("literals = array["+string nl+"] of (string,int);\n"); nl = 0;
  for (i = 1; i < len tokens; i++) if (tokens[i].literal != nil) put(sys->sprint("literals[%d] = (%s,%d);\n",nl++,quote(tokens[i].literal),i));
  put("comments = array["+string len coms+"] of (string,string,int);\n"); i = 0;
  for (c := coms; c != nil; c = tl c) { (op,cl,n) := hd c; put(sys->sprint("comments[%d] = (%s,%s,%d);\n",i++,quote(op),quote(cl),n)); }
  put("}\n");
 } else {
  put("#define NSTATES "+string nstates+"\nstatic int edges[][5] = {\n");
  for (l := trans; l != nil; l = tl l) { (f,t,lo,hi,c) := hd l; put(sys->sprint("{%d,%d,%d,%d,%d},\n",f,t,lo,hi,c)); }
  put("{0,0,-2,-2,0}};\nstatic int finals[NSTATES], ignored[256];\nstatic const int *syncset = "+setarray(allsync)+";\n");
  put("static struct { const char *word; int token; } literals[] = {\n");
  for (i := 1; i < len tokens; i++) if (tokens[i].literal != nil) put("{"+quote(tokens[i].literal)+","+string i+"},\n");
  put("{NULL,0}};\nstatic struct { const char *op,*cl; int nested; } comments[] = {\n");
  for (c := coms; c != nil; c = tl c) { (op,cl,n) := hd c; put("{"+quote(op)+","+quote(cl)+","+string n+"},\n"); }
  put("{NULL,NULL,0}};\nstatic void tables(void) {\n");
  for (a := accepts; a != nil; a = tl a) { (s,k) := hd a; put(sys->sprint("finals[%d] = %d;\n",s,k)); }
  for (i = 0; i < 256; i++) if (ignore[i]) put(sys->sprint("ignored[%d] = 1;\n",i));
  put("}\n");
 }
}
emit()
{
 ext := ".b"; if (backend == "c") ext = ".c";
 fragment := "runtime.b.frag"; if (backend == "c") fragment = "runtime.c.frag";
 runtime := readfile(runtimepath+"/"+fragment);
 output = sys->create(prefix+ext,Sys->OWRITE,8r644); if (output == nil) fatal("cannot create output");
 if (backend == "limbo") {
  put("implement "+compiler+"Parser;\ninclude \"sys.m\";\ninclude \"draw.m\";\nsys: Sys;\n"+compiler+"Parser: module { init: fn(nil: ref Draw->Context,args: list of string); parse: fn(data: array of byte): int; };\n");
  put("IgnoreCase: con "+string nocase+";\nNo_Sym: con "+string no_sym+";\nEOF_Sym: con 0;\n");
  for (i := 1; i < len tokens; i++) put(tokens[i].gen+": con "+string i+";\n");
 } else {
  put("#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <stdint.h>\n#define IgnoreCase "+string nocase+"\n#define No_Sym "+string no_sym+"\n#define EOF_Sym 0\n");
  for (i := 1; i < len tokens; i++) put("#define "+tokens[i].gen+" "+string i+"\n");
  put("static int Sym, Errors, S_Pos, S_Len, S_Line, S_Col, S_NextPos, S_NextLen, S_NextLine, S_NextCol;\nstatic void Get(void), SynError(int), SemError(int), Expect(int);\nstatic void S_GetString(int,int,char*,int), S_GetName(int,int,char*,int);\nstatic void LexString(char*,int), LookAheadString(char*,int), LexName(char*,int), LookAheadName(char*,int);\nstatic int Successful(void);\nstatic void Pragma(void), ParseRoot(void);\n");
  for (i = 0; i < len prods; i++) { attr := prods[i].attr; if (attr == nil) attr = "void"; put("static void "+prods[i].name+"("+attr+");\n"); }
 }
 put(globals+"\n");
 if (backend == "limbo") { put(runtime); emittables(); }
 else { emittables(); put(runtime); }
 r := "0";
 for (i := 1; i < len tokens; i++) if (tokens[i].pragma != nil) r += " || n == "+string i;
 if (backend == "limbo") put("IsPragma(n: int): int { return "+r+"; }\nPragma() {\n");
 else put("static int IsPragma(int n) { return "+r+"; }\nstatic void Pragma(void) {\n");
 for (i = 1; i < len tokens; i++) if (tokens[i].pragma != nil) put("if (Sym == "+string i+") {\n"+tokens[i].pragma+"\n}\n");
 put("}\n");
 for (i = 0; i < len prods; i++) {
  p := prods[i]; attr := p.attr;
  if (backend == "c") { if (attr == nil) attr = "void"; put("static void "); }
  put(p.name+"("+attr+") {\n"); emitgraph(p.body,p.follow); put("}\n");
 }
 if (backend == "limbo") put("ParseRoot() { "+compiler+"(); }\n");
 else put("static void ParseRoot(void) { "+compiler+"(); }\n");
 output = nil;
}
init(nil: ref Draw->Context,args: list of string)
{
 sys = load Sys Sys->PATH;
 backend = "limbo"; runtimepath = "/lib/cocor";
 args = tl args;
 while (args != nil) {
  a := hd args; args = tl args;
  if (a == "-strict") strict = 1;
  else if (a == "-c") backend = "c";
  else if (a == "-limbo") backend = "limbo";
  else if (a == "-r" || a == "-o") {
   if (args == nil) fatal("missing option argument");
   if (a == "-r") runtimepath = hd args; else prefix = hd args;
   args = tl args;
  } else if (len a > 0 && a[0] == '-') fatal("unknown option " + a);
  else { if (filename != nil) fatal("one grammar expected"); filename = a; }
 }
 if (filename == nil) fatal("usage: cocor [-c|-limbo] [-r runtime-dir] [-o prefix] grammar.atg");
 source = readfile(filename); if (len source > 0 && source[0] == 16rfeff) source = source[1:];
 tokens = array[1] of ref Token; tokens[0] = ref Token; tokens[0].name = "EOF";
 ignore = array[256] of int; ignore[9] = ignore[32] = 1;
 next(); grammar(); analyze(); scanbuild();
 if (prefix == nil) { prefix = compiler; for (i := 0; i < len prefix; i++) if (prefix[i] >= 'A' && prefix[i] <= 'Z') prefix[i] += 32; }
 emit();
 sys->print("generated %s (%s backend)\n",prefix,backend);
}
