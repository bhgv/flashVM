/* Checked guarded data-operation C tail entry, selected only by startup experiment. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "owner_fast.h"
#include "bytes_fast.h"
#include "shared_bytes.h"
#include "disvm/native_context.h"

extern void lea(void);
extern void movpc(void);
extern void indc(void); /* Guarded string character read. */
extern void insc(void); /* Guarded string store operand capture. */
extern void indx(void);
extern void indb(void);
extern void indw(void);
extern void indl(void);
extern void indf(void);
extern void cvtrf(void); /* Guarded FP conversion. */
extern void cvtfr(void); /* Guarded FP conversion. */
extern void cvtwf(void); /* Guarded FP conversion. */
extern void cvtlf(void); /* Guarded FP conversion. */
extern void cvtfw(void); /* Guarded FP conversion. */
extern void cvtfl(void); /* Guarded FP conversion. */
extern void addf(void); /* Guarded REAL addition. */
extern void subf(void); /* Guarded REAL subtraction. */
extern void mulf(void); /* Guarded REAL multiplication. */
extern void mulx0(void);
extern void divx0(void);
extern void cvtxx0(void);
extern void mulx1(void);
extern void divx1(void);
extern void cvtxx1(void);
extern void mulx(void);
extern void divx(void);
extern void cvtfx(void);
extern void cvtxf(void);
extern void cvtxx(void);
extern void iexpw(void);
extern void iexpl(void);
extern void iexpf(void);
extern void negf(void); /* Guarded REAL negation. */
extern void divf(void); /* Guarded REAL division. */
extern void beqf(void); /* Actual guarded REAL branch. */
extern void bnef(void); /* Actual guarded REAL branch. */
extern void bltf(void); /* Actual guarded REAL branch. */
extern void blef(void); /* Actual guarded REAL branch. */
extern void bgtf(void); /* Actual guarded REAL branch. */
extern void bgef(void); /* Actual guarded REAL branch. */
extern void beqc(void); /* Actual guarded string branch. */
extern void bnec(void); /* Actual guarded string branch. */
extern void bltc(void); /* Actual guarded string branch. */
extern void blec(void); /* Actual guarded string branch. */
extern void bgtc(void); /* Actual guarded string branch. */
extern void bgec(void); /* Actual guarded string branch. */
extern void lenl(void); /* Actual guarded list length opcode. */
extern void lenc(void); /* Existing string-length entry. */
extern void lena(void); /* Actual guarded array length opcode. */
extern void movb(void); /* Actual hosted byte copy opcode. */
extern void movw(void); /* Actual hosted native WORD copy opcode. */
extern void movl(void); /* Actual hosted LONG copy opcode. */
extern void movf(void); /* Actual hosted bit-preserving REAL copy opcode. */
extern void movm(void); /* Actual hosted variable-size raw copy opcode. */
extern void beqb(void); /* Actual guarded integer conditional branch. */
extern void bneb(void); /* Actual guarded integer conditional branch. */
extern void bltb(void); /* Actual guarded integer conditional branch. */
extern void bleb(void); /* Actual guarded integer conditional branch. */
extern void bgtb(void); /* Actual guarded integer conditional branch. */
extern void bgeb(void); /* Actual guarded integer conditional branch. */
extern void beqw(void); /* Actual guarded integer conditional branch. */
extern void bnew(void); /* Actual guarded integer conditional branch. */
extern void bltw(void); /* Actual guarded integer conditional branch. */
extern void blew(void); /* Actual guarded integer conditional branch. */
extern void bgtw(void); /* Actual guarded integer conditional branch. */
extern void bgew(void); /* Actual guarded integer conditional branch. */
extern void beql(void); /* Actual guarded integer conditional branch. */
extern void bnel(void); /* Actual guarded integer conditional branch. */
extern void bltl(void); /* Actual guarded integer conditional branch. */
extern void blel(void); /* Actual guarded integer conditional branch. */
extern void bgtl(void); /* Actual guarded integer conditional branch. */
extern void bgel(void); /* Actual guarded integer conditional branch. */
extern void shlb(void); /* Actual guarded mixed-width shift. */
extern void shlw(void); /* Actual guarded mixed-width shift. */
extern void shll(void); /* Actual guarded mixed-width shift. */
extern void shrb(void); /* Actual guarded mixed-width shift. */
extern void shrw(void); /* Actual guarded mixed-width shift. */
extern void shrl(void); /* Actual guarded mixed-width shift. */
extern void lsrw(void); /* Actual guarded mixed-width shift. */
extern void lsrl(void); /* Actual guarded mixed-width shift. */
extern void mulb(void); /* Actual guarded product/quotient/remainder. */
extern void mulw(void); /* Actual guarded product/quotient/remainder. */
extern void mull(void); /* Actual guarded product/quotient/remainder. */
extern void divb(void); /* Actual guarded product/quotient/remainder. */
extern void divw(void); /* Actual guarded product/quotient/remainder. */
extern void divl(void); /* Actual guarded product/quotient/remainder. */
extern void modb(void); /* Actual guarded product/quotient/remainder. */
extern void modw(void); /* Actual guarded product/quotient/remainder. */
extern void modl(void); /* Actual guarded product/quotient/remainder. */
extern void addb(void); /* Actual guarded integer arithmetic opcode. */
extern void addw(void); /* Actual guarded integer arithmetic opcode. */
extern void addl(void); /* Actual guarded integer arithmetic opcode. */
extern void subb(void); /* Actual guarded integer arithmetic opcode. */
extern void subw(void); /* Actual guarded integer arithmetic opcode. */
extern void subl(void); /* Actual guarded integer arithmetic opcode. */
extern void andb(void); /* Actual guarded integer arithmetic opcode. */
extern void andw(void); /* Actual guarded integer arithmetic opcode. */
extern void andl(void); /* Actual guarded integer arithmetic opcode. */
extern void orb(void); /* Actual guarded integer arithmetic opcode. */
extern void orw(void); /* Actual guarded integer arithmetic opcode. */
extern void orl(void); /* Actual guarded integer arithmetic opcode. */
extern void xorb(void); /* Actual guarded integer arithmetic opcode. */
extern void xorw(void); /* Actual guarded integer arithmetic opcode. */
extern void xorl(void); /* Actual guarded integer arithmetic opcode. */
extern void cvtbw(void); /* Actual guarded integer conversion opcode. */
extern void cvtwb(void); /* Actual guarded integer conversion opcode. */
extern void cvtws(void); /* Actual guarded integer conversion opcode. */
extern void cvtsw(void); /* Actual guarded integer conversion opcode. */
extern void cvtwl(void); /* Actual guarded integer conversion opcode. */
extern void cvtlw(void); /* Actual guarded integer conversion opcode. */
extern void headb(void); /* Actual guarded scalar/raw list-head opcode. */
extern void headw(void); /* Actual guarded scalar/raw list-head opcode. */
extern void headl(void); /* Actual guarded scalar/raw list-head opcode. */
extern void headf(void); /* Actual guarded scalar/raw list-head opcode. */
extern void headm(void); /* Actual guarded scalar/raw list-head opcode. */


/* [PERF REALDISPATCH-109] Matched old/fixed dispatch samples are retained in
 * PERFORMANCE-COSTS.md; this is not an isolated tail/FP cost.
 * [GC:] Native identity is checked before dispatch/operands. No cleanup, code
 * metadata read or retry follows a completed opcode or propagated VM error. */
int dvm_runtime_bytes_fast(REG *registers, int opcode)
{
    if (!dvm_runtime_guarded_copies || !dvm_runtime_owner_entry_ready(registers) ||
        dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    switch (opcode)
    {
    case ILEA:
        lea();
        break;
    case IMOVPC:
        movpc();
        break;
    case IINDC:
        indc();
        break;
    case IINSC:
        insc();
        break;
    case IINDX:
        indx();
        break;
    case IINDB:
        indb();
        break;
    case IINDW:
        indw();
        break;
    case IINDL:
        indl();
        break;
    case IINDF:
        indf();
        break;
    case ICVTRF:
        cvtrf();
        break;
    case ICVTFR:
        cvtfr();
        break;
    case ICVTWF:
        cvtwf();
        break;
    case ICVTLF:
        cvtlf();
        break;
    case ICVTFW:
        cvtfw();
        break;
    case ICVTFL:
        cvtfl();
        break;
    case IADDF:
        addf();
        break;
    case ISUBF:
        subf();
        break;
    case IMULF:
        mulf();
        break;
    case IDIVF:
        divf();
        break;
    case IMULX0:
        mulx0();
        break;
    case IDIVX0:
        divx0();
        break;
    case ICVTXX0:
        cvtxx0();
        break;
    case IMULX1:
        mulx1();
        break;
    case IDIVX1:
        divx1();
        break;
    case ICVTXX1:
        cvtxx1();
        break;
    case IMULX:
        mulx();
        break;
    case IDIVX:
        divx();
        break;
    case ICVTFX:
        cvtfx();
        break;
    case ICVTXF:
        cvtxf();
        break;
    case ICVTXX:
        cvtxx();
        break;
    case IEXPW:
        iexpw();
        break;
    case IEXPL:
        iexpl();
        break;
    case IEXPF:
        iexpf();
        break;
    case INEGF:
        negf();
        break;
    case IBEQF:
        beqf();
        break;
    case IBNEF:
        bnef();
        break;
    case IBLTF:
        bltf();
        break;
    case IBLEF:
        blef();
        break;
    case IBGTF:
        bgtf();
        break;
    case IBGEF:
        bgef();
        break;
    case IBEQC:
        beqc();
        break;
    case IBNEC:
        bnec();
        break;
    case IBLTC:
        bltc();
        break;
    case IBLEC:
        blec();
        break;
    case IBGTC:
        bgtc();
        break;
    case IBGEC:
        bgec();
        break;
    case ILENL:
        lenl();
        break;
    case ILENC:
        lenc();
        break;
    case ILENA:
        lena();
        break;
    case IMOVB:
        movb();
        break;
    case IMOVW:
        movw();
        break;
    case IMOVL:
        movl();
        break;
    case IMOVF:
        movf();
        break;
    case IMOVM:
        movm();
        break;
    case IBEQB:
        beqb();
        break;
    case IBNEB:
        bneb();
        break;
    case IBLTB:
        bltb();
        break;
    case IBLEB:
        bleb();
        break;
    case IBGTB:
        bgtb();
        break;
    case IBGEB:
        bgeb();
        break;
    case IBEQW:
        beqw();
        break;
    case IBNEW:
        bnew();
        break;
    case IBLTW:
        bltw();
        break;
    case IBLEW:
        blew();
        break;
    case IBGTW:
        bgtw();
        break;
    case IBGEW:
        bgew();
        break;
    case IBEQL:
        beql();
        break;
    case IBNEL:
        bnel();
        break;
    case IBLTL:
        bltl();
        break;
    case IBLEL:
        blel();
        break;
    case IBGTL:
        bgtl();
        break;
    case IBGEL:
        bgel();
        break;
    case ISHLB:
        shlb();
        break;
    case ISHLW:
        shlw();
        break;
    case ISHLL:
        shll();
        break;
    case ISHRB:
        shrb();
        break;
    case ISHRW:
        shrw();
        break;
    case ISHRL:
        shrl();
        break;
    case ILSRW:
        lsrw();
        break;
    case ILSRL:
        lsrl();
        break;
    case IMULB:
        mulb();
        break;
    case IMULW:
        mulw();
        break;
    case IMULL:
        mull();
        break;
    case IDIVB:
        divb();
        break;
    case IDIVW:
        divw();
        break;
    case IDIVL:
        divl();
        break;
    case IMODB:
        modb();
        break;
    case IMODW:
        modw();
        break;
    case IMODL:
        modl();
        break;
    case IADDB:
        addb();
        break;
    case IADDW:
        addw();
        break;
    case IADDL:
        addl();
        break;
    case ISUBB:
        subb();
        break;
    case ISUBW:
        subw();
        break;
    case ISUBL:
        subl();
        break;
    case IANDB:
        andb();
        break;
    case IANDW:
        andw();
        break;
    case IANDL:
        andl();
        break;
    case IORB:
        orb();
        break;
    case IORW:
        orw();
        break;
    case IORL:
        orl();
        break;
    case IXORB:
        xorb();
        break;
    case IXORW:
        xorw();
        break;
    case IXORL:
        xorl();
        break;
    case ICVTBW:
        cvtbw();
        break;
    case ICVTWB:
        cvtwb();
        break;
    case ICVTWS:
        cvtws();
        break;
    case ICVTSW:
        cvtsw();
        break;
    case ICVTWL:
        cvtwl();
        break;
    case ICVTLW:
        cvtlw();
        break;
    case IHEADB:
        headb();
        break;
    case IHEADW:
        headw();
        break;
    case IHEADL:
        headl();
        break;
    case IHEADF:
        headf();
        break;
    case IHEADM:
        headm();
        break;
    default:
        return 0;
    }
    return 1;
}
