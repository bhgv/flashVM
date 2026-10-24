/* Limbo backend for Coco/R C. See limbo/README.md. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crt.h"
#include "cra.h"
#include "crf.h"
#include "crs.h"
#include "limbo_runtime.h"
static FILE *out;
static int current_nt;
static void text(long pos, int len) {
 int i; for(i=0;i<len;i++) fputc(S_CurrentCh(pos+i),out);
}
static void quote(const char *s) {
 const unsigned char *p=(const unsigned char *)s;
 fputc('"',out);
 for(;*p;p++) {
  if(*p=='"'||*p=='\\') fputc('\\',out);
  if(*p=='\n') fputs("\\n",out);
  else if(*p=='\r') fputs("\\r",out);
  else if(*p=='\t') fputs("\\t",out);
  else if(*p<32) fprintf(out,"\\u%04x",*p);
  else fputc(*p,out);
 }
 fputc('"',out);
}
static void arr(Set *s) {
 int i,sep=0; fputs("array[] of {",out);
 for(i=0;i<=Set_MaxIndex(s);i++) if(Set_IsItem(s,i)) {
  fprintf(out,"%s%d",sep?",":"",i); sep=1;
 }
 if(!sep) fputs("* => -1",out); /* empty sets represented separately by conditions */
 fputc('}',out);
}
static void condset(Set *s) {
 int i,sep=0;
 for(i=0;i<=Set_MaxIndex(s);i++) if(Set_IsItem(s,i)) {
  fprintf(out,"%sSym == %d",sep?" || ":"",i); sep=1;
 }
 if(!sep) fputs("0",out);
}
static void condition(int gp, int expected) {
 Set s; Set_Init(&s);
 if(gp>0 && GetGraphP(gp)->type==T_RESOLVER) {
  PGraphNode g=GetGraphP(gp); text(g->SEMPOS,g->SEMLEN);
 } else {
  if(expected) CompExpected(gp,current_nt,&s); else CompFirstSet(gp,&s);
  condset(&s);
 }
 Set_Done(&s);
}
static void graph(int gp) {
 while(gp>0) {
  PGraphNode g=GetGraphP(gp);
  switch(g->type) {
  case T_T: fprintf(out,"Expect(%ld);\n",g->SYMLINK); break;
  case T_WT: {
   PSymSetNode ss=GetSymSetP(g->SETLINK1);
   fprintf(out,"ExpectWeak(%ld, ",g->SYMLINK); arr(&ss->set); fputs(");\n",out); break;
  }
  case T_NT:
   fprintf(out,"%s(",(GetNTermP(g->SYMLINK))->name);
   if(g->ALT) {PGraphNode a=GetGraphP(g->ALT);text(a->SEMPOS,a->SEMLEN);}
   fputs(");\n",out); break;
  case T_OPT:
   fputs("if (",out); condition(g->INNER,0); fputs(") {\n",out);
   graph(g->INNER); fputs("}\n",out); break;
  case T_REP: {
   PGraphNode a=GetGraphP(g->INNER);
   if(a->type==T_WT) {
    fprintf(out,"while (WeakSeparator(%ld, ",a->SYMLINK);
    arr(&(GetSymSetP(a->SETLINK1))->set); fputs(", ",out);
    arr(&(GetSymSetP(a->SETLINK2))->set); fputs(")) {\n",out); graph(a->next);
   } else {
    fputs("while (",out); condition(g->INNER,0);fputs(") {\n",out);graph(g->INNER);
   }
   fputs("}\n",out);break;
  }
  case T_ALT: {
   int a=gp,first=1;
   while(a) {
    PGraphNode branch=GetGraphP(a);
    fputs(first?"if (":"else if (",out); condition(branch->INNER,1);
    fputs(") {\n",out);graph(branch->INNER);fputs("}\n",out);
    first=0;a=branch->ALT;
   }
   fputs("else SynError(No_Sym);\n",out);break;
  }
  case T_SEM: text(g->SEMPOS,g->SEMLEN);fputc('\n',out);break;
  case T_ANY: {
   fputs("if (",out);condset(&(GetSymSetP(g->SETLINK1))->set);
   fputs(") Get(); else SynError(No_Sym);\n",out); break;
  }
  case T_SYNC:
   fputs("while (Sym != 0 && !(",out);condset(&(GetSymSetP(g->SETLINK1))->set);
   fputs(")) { SynError(No_Sym); Get(); }\n",out);break;
  }
  gp=g->next;
 }
}
static void tables(void) {
 Collection *states=CocoStates(), *com=CocoComments(); int i,j,k,n,sep=0;
 fputs("tables()\n{\nedges = array[] of {\n",out);
 for(i=0;i<Collection_Count(states);i++) {
  PStateNode sn=Collection_At(states,i);
  if(sn->gen_state_no==-1) continue;
  for(j=0;j<Collection_Count(&sn->trans_list);j++) {
   PTransNode t=Collection_At(&sn->trans_list,j);
   if(t->type==T_NONE) continue;
   for(k=0;k<256;k++) {
    int match=t->type==T_CHAR?k==t->sym:Set_IsItem(&(GetClassP(t->sym))->data,k);
    if(!match) continue;
    n=k;
    while(k+1<256 && (t->type==T_CHAR?k+1==t->sym:Set_IsItem(&(GetClassP(t->sym))->data,k+1))) k++;
    fprintf(out,"%s(%d,%d,%d,%d,%d)\n",sep?",":"",i,Set_MinIndex(&t->to_states),n,k,t->tc==T_CONTEXT);sep=1;
   }
  }
 }
 if(!sep) fputs("(0,0,-2,-2,0)\n",out);
 fprintf(out,"};\nfinals = array[%d] of { * => 0 };\n",Collection_Count(states));
 for(i=0;i<Collection_Count(states);i++) {
  PStateNode sn=Collection_At(states,i);n=sn->end_of;
  if(n>=FIRST_PRAGMA)n=n-FIRST_PRAGMA+no_sym+1;
  if(n && sn->gen_state_no!=-1)fprintf(out,"finals[%d] = %d;\n",i,n);
 }
 fputs("syncset = ",out); arr(&(GetSymSetP(0))->set); fputs(";\n",out);
 fputs("ignored = array[256] of { * => 0 };\n",out);
 n=FindClass("@ignore_chars");
 if(n>=0)for(i=0;i<256;i++)if(Set_IsItem(&(GetClassP(n))->data,i))fprintf(out,"ignored[%d] = 1;\n",i);
 n=0;for(i=0;i<Collection_Count(&term_tab);i++)if((GetTermP(i))->type==T_LITTOKEN)n++;
 fprintf(out,"literals = array[%d] of (string,int);\n",n);
 n=0;for(i=0;i<Collection_Count(&term_tab);i++)if((GetTermP(i))->type==T_LITTOKEN) {
  char s[MAX_ID_LEN];strcpy(s,(GetTermP(i))->name+1);s[strlen(s)-1]=0;
  fprintf(out,"literals[%d] = (",n++);quote(s);fprintf(out,",%d);\n",i);
 }
 fprintf(out,"comments = array[%d] of (string,string,int);\n",Collection_Count(com));
 for(i=0;i<Collection_Count(com);i++) {
  PCommentNode c=Collection_At(com,i);fprintf(out,"comments[%d] = (",i);quote(c->start_token);
  fputc(',',out);quote(c->end_token);fprintf(out,",%d);\n",c->nested);
 }
 fputs("}\n",out);
}
void GenLimbo(void) {
 char name[256],tok[MAX_ID_LEN]; int i;
 snprintf(name,sizeof name,"%s.b",module_prefix);out=OpenFile(name,"w",1);
 fprintf(out,"implement %sParser;\ninclude \"sys.m\";\ninclude \"draw.m\";\nsys: Sys;\n%sParser: module {\n init: fn(nil: ref Draw->Context, args: list of string);\n parse: fn(data: array of byte): int;\n};\n",compiler_name,compiler_name);
 fprintf(out,"IgnoreCase: con %d;\nNo_Sym: con %d;\n",ignore_case,no_sym);
 for(i=0;i<Collection_Count(&term_tab);i++) {
  GetTermName(i,tok);if(strcmp(tok,"No_Sym"))fprintf(out,"%s: con %d;\n",tok,i);
 }
 if(global_defs.len>0)text(global_defs.pos,global_defs.len);
 fputc('\n',out);fputs(limbo_runtime,out);tables();
 fprintf(out,"IsPragma(n: int): int { return n > No_Sym; }\n");
 fputs("Pragma() {\n",out);
 for(i=0;i<Collection_Count(&pragma_tab);i++) {
  PPragmaNode p=Collection_At(&pragma_tab,i);
  if(p->has_attr){fprintf(out,"if (Sym == %d) {\n",i+no_sym+1);text(p->sem_text.pos,p->sem_text.len);fputs("\n}\n",out);}
 }
 fputs("}\n",out);
 for(i=1;i<Collection_Count(&nterm_tab);i++) {
  PNTermNode n=GetNTermP(i); current_nt=i;
  fprintf(out,"%s(",n->name);
  if(n->has_attr){PGraphNode a=GetGraphP(n->attr);text(a->SEMPOS,a->SEMLEN);}
  fputs(")\n{\n",out);graph(n->graph);fputs("}\n",out);
 }
 fprintf(out,"ParseRoot() { %s(); }\n",compiler_name);
 if(fclose(out)!=0){perror(name);exit(1);}
}
