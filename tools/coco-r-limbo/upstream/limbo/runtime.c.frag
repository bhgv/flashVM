/* Standalone C runtime emitted by the native Limbo generator. */
static unsigned char *input;
static int inputlen, scanpos, scanline, scancol;
static char *lexbuf;
static size_t lexcap;
static int fold(int c) { return IgnoreCase && c >= 'a' && c <= 'z' ? c-32 : c; }
static int same(int p, const char *s) {
 size_t i,n=strlen(s); if (n>(size_t)(inputlen-p)) return 0;
 for(i=0;i<n;i++) if(fold(input[p+(int)i])!=fold((unsigned char)s[i]))return 0;
 return 1;
}
static void move(int p) {
 while(scanpos<p) { int c=input[scanpos++]; if(c==10){scanline++;scancol=1;}
 else if(c==9)scancol+=8-(scancol-1)%8;else scancol++; }
}
static void SynError(int error_number) {
 Errors++;fprintf(stderr,"%d:%d: syntax error %d (token %d)\n",S_NextLine,S_NextCol,error_number,Sym);
}
static void SemError(int n) { Errors++;fprintf(stderr,"%d:%d: semantic error %d\n",S_Line,S_Col,n); }
static int Successful(void) { return Errors==0; }
static char *token_text(int pos,int count) {
 size_t n;
 if(pos<0||count<0||pos>inputlen||count>inputlen-pos) return "";
 n=(size_t)count+1;
 if(n>lexcap){char *p=realloc(lexbuf,n);if(!p){fputs("out of memory\n",stderr);exit(1);}lexbuf=p;lexcap=n;}
 memcpy(lexbuf,input+pos,(size_t)count);lexbuf[count]=0;return lexbuf;
}
static void S_GetString(int pos,int count,char *dest,int max) {
 int n=count;if(max<0)return;if(n>max)n=max;if(n<0)n=0;
 memcpy(dest,token_text(pos,n),(size_t)n);dest[n]=0;
}
static void S_GetName(int pos,int count,char *dest,int max) {
 int i;S_GetString(pos,count,dest,max);for(i=0;dest[i];i++)dest[i]=(char)fold((unsigned char)dest[i]);
}
static void LexString(char *s,int max) { S_GetString(S_Pos,S_Len,s,max); }
static void LookAheadString(char *s,int max) { S_GetString(S_NextPos,S_NextLen,s,max); }
static void LexName(char *s,int max) { S_GetName(S_Pos,S_Len,s,max); }
static void LookAheadName(char *s,int max) { S_GetName(S_NextPos,S_NextLen,s,max); }
static void closure(int *a) {
 int changed=1;size_t i;
 while(changed){changed=0;for(i=0;i<sizeof edges/sizeof edges[0];i++) {
 int *e=edges[i];if(e[2]==-1&&a[e[0]]>=0&&a[e[1]]<0){a[e[1]]=a[e[0]];changed=1;}
 }}
}
static int S_Get(void) {
 int i,j,found,p,best,end,far,any,c;
 int a[NSTATES],b[NSTATES];
 for(;;){
  while(scanpos<inputlen&&ignored[fold(input[scanpos])])move(scanpos+1);
  found=0;
  for(i=0;comments[i].op;i++){
   int level;
   if(!same(scanpos,comments[i].op))continue;
   move(scanpos+(int)strlen(comments[i].op));level=1;
   while(level&&scanpos<inputlen){
    if(same(scanpos,comments[i].cl)){level--;move(scanpos+(int)strlen(comments[i].cl));}
    else if(comments[i].nested&&same(scanpos,comments[i].op)){level++;move(scanpos+(int)strlen(comments[i].op));}
    else move(scanpos+1);
   }
   if(level){S_NextLine=scanline;S_NextCol=scancol;SynError(No_Sym);}
   found=1;break;
  }
  if(!found)break;
 }
 S_NextPos=scanpos;S_NextLine=scanline;S_NextCol=scancol;S_NextLen=0;
 if(scanpos>=inputlen)return 0;
 for(i=0;i<NSTATES;i++)a[i]=-1;
 a[0]=0;closure(a);best=No_Sym;end=scanpos+1;far=-1;p=scanpos;
 for(;;){
  for(i=0;i<NSTATES;i++)if(a[i]>=0&&finals[i]>0){int k=finals[i];
   if(p>far||(p==far&&k<best)){far=p;best=k;end=a[i]>0?a[i]-1:p;}
  }
  if(p>=inputlen)break;
  for(i=0;i<NSTATES;i++)b[i]=-1;
  any=0;c=fold(input[p]);
  for(j=0;j<(int)(sizeof edges/sizeof edges[0]);j++){
   int *e=edges[j],cut;
   if(a[e[0]]<0||e[2]<0||c<e[2]||c>e[3])continue;
   cut=a[e[0]];if(e[4]&&cut==0)cut=p+1;if(!e[4])cut=0;
   b[e[1]]=cut;any=1;
  }
  if(!any)break;
  p++;memcpy(a,b,sizeof a);closure(a);
 }
 if(end<=scanpos){end=scanpos+1;best=No_Sym;}
 move(end);S_NextLen=end-S_NextPos;
 for(i=0;literals[i].word;i++){
 char *s=token_text(S_NextPos,S_NextLen);int k;for(k=0;k<S_NextLen;k++)s[k]=(char)fold((unsigned char)s[k]);
 if(!strcmp(s,literals[i].word))return literals[i].token;
 }
 return best;
}
static int IsPragma(int n);
static void Get(void) {
 S_Pos=S_NextPos;S_Len=S_NextLen;S_Line=S_NextLine;S_Col=S_NextCol;
 do { Sym=S_Get();if(IsPragma(Sym)){Pragma();if(!IsPragma(Sym))break;}else break; }while(1);
}
static void Expect(int n) { if(Sym==n)Get();else{SynError(n);if(Sym)Get();} }
static int InSet(int n,const int *s) { for(;*s>=0;s++)if(*s==n)return 1;return 0; }
static void ExpectWeak(int n,const int *follow) {
 if(Sym==n)Get();else{SynError(n);while(Sym&&!InSet(Sym,follow)&&!InSet(Sym,syncset))Get();}
}
static int WeakSeparator(int n,const int *syfol,const int *repfol) {
 if(Sym==n){Get();return 1;}if(InSet(Sym,repfol))return 0;
 SynError(n);while(Sym&&!InSet(Sym,syfol)&&!InSet(Sym,repfol)&&!InSet(Sym,syncset))Get();
 return InSet(Sym,syfol);
}
int coco_parse(const unsigned char *data,int length) {
 if(length<0||(!data&&length))return -1;
 input=(unsigned char*)data;inputlen=length;scanpos=0;scanline=scancol=1;
 Errors=Sym=S_Pos=S_Len=S_NextPos=S_NextLen=0;
 if(length>=3&&input[0]==239&&input[1]==187&&input[2]==191)scanpos=3;
 tables();Get();ParseRoot();Expect(0);return Errors;
}
#ifndef COCO_NO_MAIN
int main(int argc,char **argv) {
 FILE *f;unsigned char *data;long n;int result;
 if(argc!=2){fputs("usage: parser input\n",stderr);return 2;}
 f=fopen(argv[1],"rb");if(!f){perror(argv[1]);return 2;}
 if(fseek(f,0,SEEK_END)||(n=ftell(f))<0||n>INT32_MAX||fseek(f,0,SEEK_SET)){fclose(f);return 2;}
 data=malloc((size_t)n+1);if(!data){fclose(f);return 2;}
 if(fread(data,1,(size_t)n,f)!=(size_t)n){free(data);fclose(f);return 2;}
 fclose(f);result=coco_parse(data,(int)n);free(data);free(lexbuf);lexbuf=NULL;lexcap=0;return result?1:0;
}
#endif
