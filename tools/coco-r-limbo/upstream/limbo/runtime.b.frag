# Shared byte scanner. State 0 is the initial state; lo=-1 denotes epsilon.
edges: array of (int, int, int, int, int);
finals: array of int;
literals: array of (string, int);
comments: array of (string, string, int);
ignored, syncset: array of int;
input: array of byte;
scanpos, scanline, scancol: int;
Sym, Errors, S_Pos, S_Len, S_Line, S_Col: int;
S_NextPos, S_NextLen, S_NextLine, S_NextCol: int;

fold(c: int): int
{
 if (IgnoreCase && c >= 'a' && c <= 'z') return c - 32;
 return c;
}

same(p: int, s: string): int
{
 b := array of byte s;
 if (p + len b > len input) return 0;
 for (i := 0; i < len b; i++)
  if (fold(int input[p+i]) != fold(int b[i])) return 0;
 return 1;
}

move(p: int)
{
 while (scanpos < p) {
  c := int input[scanpos++];
  if (c == 10) { scanline++; scancol = 1; }
  else if (c == 9) scancol += 8 - ((scancol-1) % 8);
  else scancol++;
 }
}

closure(a: array of int)
{
 changed := 1;
 while (changed) {
  changed = 0;
  for (i := 0; i < len edges; i++) {
   (f, t, lo, nil, nil) := edges[i];
   if (lo == -1 && a[f] >= 0 && a[t] < 0) {
    a[t] = a[f]; changed = 1;
   }
  }
 }
}

S_Get(): int
{
 for (;;) {
  while (scanpos < len input && ignored[fold(int input[scanpos])]) move(scanpos+1);
  found := 0;
  for (i := 0; i < len comments; i++) {
   (op, cl, nested) := comments[i];
   if (!same(scanpos, op)) continue;
   move(scanpos + len array of byte op);
   level := 1;
   while (level && scanpos < len input) {
    if (same(scanpos, cl)) { level--; move(scanpos + len array of byte cl); }
    else if (nested && same(scanpos, op)) { level++; move(scanpos + len array of byte op); }
    else move(scanpos+1);
   }
   if (level) { S_NextLine = scanline; S_NextCol = scancol; SynError(No_Sym); }
   found = 1; break;
  }
  if (!found) break;
 }
 S_NextPos = scanpos; S_NextLine = scanline; S_NextCol = scancol;
 S_NextLen = 0;
 if (scanpos >= len input) return 0;
 a := array[len finals] of { * => -1 };
 a[0] = 0; closure(a);
 best := No_Sym; end := scanpos+1; far := -1;
 p := scanpos;
 for (;;) {
  for (i := 0; i < len a; i++) {
   if (a[i] < 0 || finals[i] <= 0) continue;
   k := finals[i];
   if (p > far || (p == far && k < best)) {
    far = p; best = k; end = p;
    if (a[i] > 0) end = a[i]-1;
   }
  }
  if (p >= len input) break;
  b := array[len a] of { * => -1 };
  any := 0; c := fold(int input[p]);
  for (i = 0; i < len edges; i++) {
   (f, t, lo, hi, context) := edges[i];
   if (a[f] < 0 || lo < 0 || c < lo || c > hi) continue;
   cut := a[f];
   if (context && cut == 0) cut = p+1;
   if (!context) cut = 0;
   b[t] = cut; any = 1;
  }
  if (!any) break;
  p++; a = b; closure(a);
 }
 if (end <= scanpos) { end = scanpos+1; best = No_Sym; }
 move(end); S_NextLen = end - S_NextPos;
 text := LookAheadName();
 for (i := 0; i < len literals; i++) {
  (word, token) := literals[i];
  if (text == word) return token;
 }
 return best;
}

S_GetString(pos, count: int): string
{
 if (pos < 0 || count < 0 || pos > len input || count > len input-pos) return nil;
 return string input[pos:pos+count];
}

LexString(): string { return S_GetString(S_Pos, S_Len); }
LookAheadString(): string { return S_GetString(S_NextPos, S_NextLen); }
S_GetName(pos, count: int): string
{
 b := array of byte S_GetString(pos, count);
 for (i := 0; i < len b; i++) b[i] = byte fold(int b[i]);
 return string b;
}
LexName(): string { return S_GetName(S_Pos, S_Len); }
LookAheadName(): string { return S_GetName(S_NextPos, S_NextLen); }
Successful(): int { return Errors == 0; }
SynError(error_number: int)
{
 Errors++;
 sys->fprint(sys->fildes(2), "%d:%d: syntax error %d (token %d)\n", S_NextLine, S_NextCol, error_number, Sym);
}
SemError(error_number: int)
{
 Errors++;
 sys->fprint(sys->fildes(2), "%d:%d: semantic error %d\n", S_Line, S_Col, error_number);
}
Get()
{
 S_Pos = S_NextPos; S_Len = S_NextLen; S_Line = S_NextLine; S_Col = S_NextCol;
 for (;;) {
  Sym = S_Get();
  if (!IsPragma(Sym)) break;
  Pragma();
  if (!IsPragma(Sym)) break;
 }
}
Expect(n: int)
{
 if (Sym == n) Get();
 else { SynError(n); if (Sym != 0) Get(); }
}
InSet(n: int, s: array of int): int
{
 for (i := 0; i < len s; i++) if (s[i] == n) return 1;
 return 0;
}
ExpectWeak(n: int, follow: array of int)
{
 if (Sym == n) Get();
 else {
  SynError(n);
  while (Sym != 0 && !InSet(Sym, follow) && !InSet(Sym, syncset)) Get();
 }
}
WeakSeparator(n: int, syfol, repfol: array of int): int
{
 if (Sym == n) { Get(); return 1; }
 if (InSet(Sym, repfol)) return 0;
 SynError(n);
 while (Sym != 0 && !InSet(Sym, syfol) && !InSet(Sym, repfol) && !InSet(Sym, syncset)) Get();
 return InSet(Sym, syfol);
}

parse(data: array of byte): int
{
 if (sys == nil) sys = load Sys Sys->PATH;
 input = data; scanpos = 0; scanline = scancol = 1; Errors = 0; Sym = 0;
 S_Pos = S_Len = S_NextPos = S_NextLen = 0;
 if (len input >= 3 && int input[0] == 239 && int input[1] == 187 && int input[2] == 191) scanpos = 3;
 tables(); Get(); ParseRoot(); Expect(0);
 return Errors;
}

init(nil: ref Draw->Context, args: list of string)
{
 sys = load Sys Sys->PATH;
 args = tl args;
 if (args == nil) { sys->fprint(sys->fildes(2), "usage: parser input\n"); raise "fail:usage"; }
 fd := sys->open(hd args, Sys->OREAD);
 if (fd == nil) raise "fail:open input";
 (ok, st) := sys->fstat(fd);
 if (ok < 0 || st.length > big Sys->Maxint) raise "fail:stat input";
 data := array[int st.length] of byte;
 if (sys->readn(fd, data, len data) != len data) raise "fail:read input";
 if (parse(data) != 0) raise "fail:syntax errors";
}
