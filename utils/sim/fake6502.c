/*
Copyright 2021 LLVM-MOS Project

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/* Fake6502 CPU emulator core v1.1 *******************
 * Originally by Mike Chambers (miker00lz@gmail.com) *
 * Modified by LLVM-MOS project.                     *
 *****************************************************
 * v1.1 - Small bugfix in BIT opcode, but it was the *
 *        difference between a few games in my NES   *
 *        emulator working and being broken!         *
 *        I went through the rest carefully again    *
 *        after fixing it just to make sure I didn't *
 *        have any other typos! (Dec. 17, 2011)      *
 *                                                   *
 * v1.0 - First release (Nov. 24, 2011)              *
 *****************************************************
 * Fake6502 is a MOS Technology 6502 CPU emulation   *
 * engine in C. It was written as part of a Nintendo *
 * Entertainment System emulator I've been writing.  *
 *                                                   *
 * A couple important things to know about are two   *
 * defines in the code. One is "UNDOCUMENTED" which, *
 * when defined, allows Fake6502 to compile with     *
 * full support for the more predictable             *
 * undocumented instructions of the 6502. If it is   *
 * undefined, undocumented opcodes just act as NOPs. *
 *                                                   *
 * The other define is "NES_CPU", which causes the   *
 * code to compile without support for binary-coded  *
 * decimal (BCD) support for the ADC and SBC         *
 * opcodes. The Ricoh 2A03 CPU in the NES does not   *
 * support BCD, but is otherwise identical to the    *
 * standard MOS 6502. (Note that this define is      *
 * enabled in this file if you haven't changed it    *
 * yourself. If you're not emulating a NES, you      *
 * should comment it out.)                           *
 *                                                   *
 * If you do discover an error in timing accuracy,   *
 * or operation in general please e-mail me at the   *
 * address above so that I can fix it. Thank you!    *
 *                                                   *
 *****************************************************
 * Usage:                                            *
 *                                                   *
 * Fake6502 requires you to provide two external     *
 * functions:                                        *
 *                                                   *
 * uint8_t read6502(uint16_t address)                *
 * void write6502(uint16_t address, uint8_t value)   *
 *                                                   *
 * You may optionally pass Fake6502 the pointer to a *
 * function which you want to be called after every  *
 * emulated instruction. This function should be a   *
 * void with no parameters expected to be passed to  *
 * it.                                               *
 *                                                   *
 * This can be very useful. For example, in a NES    *
 * emulator, you check the number of clock ticks     *
 * that have passed so you can know when to handle   *
 * APU events.                                       *
 *                                                   *
 * To pass Fake6502 this pointer, use the            *
 * hookexternal(void *funcptr) function provided.    *
 *                                                   *
 * To disable the hook later, pass NULL to it.       *
 *****************************************************
 * Useful functions in this emulator:                *
 *                                                   *
 * void reset6502(uint8_t cmos)                      *
 *   - Call this once before you begin execution.    *
 *   - 65C02 emulation is enabled by setting the     *
 *     cmos flag.                                    *
 *                                                   *
 * void exec6502(uint32_t tickcount)                 *
 *   - Execute 6502 code up to the next specified    *
 *     count of clock ticks.                         *
 *                                                   *
 * void step6502()                                   *
 *   - Execute a single instrution.                  *
 *                                                   *
 * void irq6502()                                    *
 *   - Trigger a hardware IRQ in the 6502 core.      *
 *                                                   *
 * void nmi6502()                                    *
 *   - Trigger an NMI in the 6502 core.              *
 *                                                   *
 * void hookexternal(void *funcptr)                  *
 *   - Pass a pointer to a void function taking no   *
 *     parameters. This will cause Fake6502 to call  *
 *     that function once after each emulated        *
 *     instruction.                                  *
 *                                                   *
 *****************************************************
 * Useful variables in this emulator:                *
 *                                                   *
 * uint32_t clockticks6502                           *
 *   - A running total of the emulated cycle count.  *
 *                                                   *
 * uint32_t instructions                             *
 *   - A running total of the total emulated         *
 *     instruction count. This is not related to     *
 *     clock cycle timing.                           *
 *                                                   *
 *****************************************************/

#include <stdio.h>
#include <stdint.h>

//6502 defines
#define UNDOCUMENTED //when this is defined, undocumented opcodes are handled.
                     //otherwise, they're simply treated as NOPs.

//#define NES_CPU    //when this is defined, the binary-coded decimal (BCD)
                     //status flag is not honored by ADC and SBC. the 2A03
                     //CPU in the Nintendo Entertainment System does not
                     //support BCD operation.

#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_CONSTANT  0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_SIGN      0x80

#define BASE_STACK     0x100

#define saveaccum(n) a = (uint8_t)((n) & 0x00FF)


//flag modifier macros
#define setcarry() status |= FLAG_CARRY
#define clearcarry() status &= (~FLAG_CARRY)
#define setzero() status |= FLAG_ZERO
#define clearzero() status &= (~FLAG_ZERO)
#define setinterrupt() status |= FLAG_INTERRUPT
#define clearinterrupt() status &= (~FLAG_INTERRUPT)
#define setdecimal() status |= FLAG_DECIMAL
#define cleardecimal() status &= (~FLAG_DECIMAL)
#define setoverflow() status |= FLAG_OVERFLOW
#define clearoverflow() status &= (~FLAG_OVERFLOW)
#define setsign() status |= FLAG_SIGN
#define clearsign() status &= (~FLAG_SIGN)


//flag calculation macros
#define zerocalc(n) {\
    if ((n) & 0x00FF) clearzero();\
        else setzero();\
}

#define signcalc(n) {\
    if ((n) & 0x0080) setsign();\
        else clearsign();\
}

#define carrycalc(n) {\
    if ((n) & 0xFF00) setcarry();\
        else clearcarry();\
}

#define overflowcalc(n, m, o) { /* n = result, m = accumulator, o = memory */ \
    if (((n) ^ (uint16_t)(m)) & ((n) ^ (o)) & 0x0080) setoverflow();\
        else clearoverflow();\
}


//6502 CPU registers
uint16_t pc;
uint8_t sp, a, x, y, status;


//helper variables
uint32_t instructions = 0; //keep track of total instructions executed
uint32_t clockticks6502 = 0, clockgoal6502 = 0;
uint16_t oldpc, ea, reladdr, value, result;
uint8_t opcode, oldstatus;

//externally supplied functions
extern uint8_t read6502(uint16_t address);
extern void write6502(uint16_t address, uint8_t value);

//a few general functions used by various other functions
void push16(uint16_t pushval) {
    write6502(BASE_STACK + sp, (pushval >> 8) & 0xFF);
    write6502(BASE_STACK + ((sp - 1) & 0xFF), pushval & 0xFF);
    sp -= 2;
}

void push8(uint8_t pushval) {
    write6502(BASE_STACK + sp--, pushval);
}

uint16_t pull16() {
    uint16_t temp16;
    temp16 = read6502(BASE_STACK + ((sp + 1) & 0xFF)) | ((uint16_t)read6502(BASE_STACK + ((sp + 2) & 0xFF)) << 8);
    sp += 2;
    return(temp16);
}

uint8_t pull8() {
    return (read6502(BASE_STACK + ++sp));
}


static void (**addrtable)() = NULL;
static void (**optable)() = NULL;
static const uint32_t *ticktable = NULL;
uint8_t penaltyop, penaltyaddr;

//addressing mode functions, calculates effective addresses
static void imp() { //implied
}

static void acc() { //accumulator
}

static void imm() { //immediate
    ea = pc++;
}

static void zp() { //zero-page
    ea = (uint16_t)read6502((uint16_t)pc++);
}

static void zpx() { //zero-page,X
    ea = ((uint16_t)read6502((uint16_t)pc++) + (uint16_t)x) & 0xFF; //zero-page wraparound
}

static void zpy() { //zero-page,Y
    ea = ((uint16_t)read6502((uint16_t)pc++) + (uint16_t)y) & 0xFF; //zero-page wraparound
}

static void rel() { //relative for branch ops (8-bit immediate value, sign-extended)
    reladdr = (uint16_t)read6502(pc++);
    if (reladdr & 0x80) reladdr |= 0xFF00;
}

static void zpr() { //combined zp, rel for bbr/bbs
    ea = (uint16_t)read6502((uint16_t)pc++);
    reladdr = (uint16_t)read6502(pc++);
    if (reladdr & 0x80) reladdr |= 0xFF00;
}

static void abso() { //absolute
    ea = (uint16_t)read6502(pc) | ((uint16_t)read6502(pc+1) << 8);
    pc += 2;
}

static void absx() { //absolute,X
    uint16_t startpage;
    ea = ((uint16_t)read6502(pc) | ((uint16_t)read6502(pc+1) << 8));
    startpage = ea & 0xFF00;
    ea += (uint16_t)x;

    if (startpage != (ea & 0xFF00)) { //one cycle penlty for page-crossing on some opcodes
        penaltyaddr = 1;
    }

    pc += 2;
}

static void absy() { //absolute,Y
    uint16_t startpage;
    ea = ((uint16_t)read6502(pc) | ((uint16_t)read6502(pc+1) << 8));
    startpage = ea & 0xFF00;
    ea += (uint16_t)y;

    if (startpage != (ea & 0xFF00)) { //one cycle penlty for page-crossing on some opcodes
        penaltyaddr = 1;
    }

    pc += 2;
}

static void ind() { //indirect
    uint16_t eahelp, eahelp2;
    eahelp = (uint16_t)read6502(pc) | (uint16_t)((uint16_t)read6502(pc+1) << 8);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //replicate 6502 page-boundary wraparound bug
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    pc += 2;
}

static void inzp() { //indirectZP
    uint16_t eahelp;
    eahelp = (uint16_t)(((uint16_t)read6502(pc++)) & 0xFF); //zero-page wraparound for table pointer
    ea = (uint16_t)read6502(eahelp & 0x00FF) | ((uint16_t)read6502((eahelp+1) & 0x00FF) << 8);
}

static void indx() { // (indirect,X)
    uint16_t eahelp;
    eahelp = (uint16_t)(((uint16_t)read6502(pc++) + (uint16_t)x) & 0xFF); //zero-page wraparound for table pointer
    ea = (uint16_t)read6502(eahelp & 0x00FF) | ((uint16_t)read6502((eahelp+1) & 0x00FF) << 8);
}

static void inax() { // (indirectABS,X)
    uint16_t eahelp, eahelp2;
    eahelp = ((uint16_t)read6502(pc) | (uint16_t)((uint16_t)read6502(pc+1) << 8)) + (uint16_t)x;
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //replicate 6502 page-boundary wraparound bug
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    pc += 2;
}

static void indy() { // (indirect),Y
    uint16_t eahelp, eahelp2, startpage;
    eahelp = (uint16_t)read6502(pc++);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //zero-page wraparound
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    startpage = ea & 0xFF00;
    ea += (uint16_t)y;

    if (startpage != (ea & 0xFF00)) { //one cycle penlty for page-crossing on some opcodes
        penaltyaddr = 1;
    }
}

static uint16_t getvalue() {
    if (addrtable[opcode] == acc) return((uint16_t)a);
        else return((uint16_t)read6502(ea));
}

static void putvalue(uint16_t saveval) {
    if (addrtable[opcode] == acc) a = (uint8_t)(saveval & 0x00FF);
        else write6502(ea, (saveval & 0x00FF));
}


//instruction handler functions
static void adc() {
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a + value + (uint16_t)(status & FLAG_CARRY);

    carrycalc(result);
    zerocalc(result);
    overflowcalc(result, a, value);
    signcalc(result);

    #ifndef NES_CPU
    if (status & FLAG_DECIMAL) {
        clearcarry();

        if ((a & 0x0F) > 0x09) {
            a += 0x06;
        }
        if ((a & 0xF0) > 0x90) {
            a += 0x60;
            setcarry();
        }

        clockticks6502++;
    }
    #endif

    saveaccum(result);
}

static void and() {
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a & value;

    zerocalc(result);
    signcalc(result);

    saveaccum(result);
}

static void asl() {
    value = getvalue();
    result = value << 1;

    carrycalc(result);
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void bcc() {
    if ((status & FLAG_CARRY) == 0) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void bcs() {
    if ((status & FLAG_CARRY) == FLAG_CARRY) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void beq() {
    if ((status & FLAG_ZERO) == FLAG_ZERO) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void bra() {
    oldpc = pc;
    pc += reladdr;
    if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 1; //check if jump crossed a page boundary
}

static void bit() {
    value = getvalue();
    result = (uint16_t)a & value;

    zerocalc(result);
    status = (status & 0x3F) | (uint8_t)(value & 0xC0);
}

static void bmi() {
    if ((status & FLAG_SIGN) == FLAG_SIGN) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void bne() {
    if ((status & FLAG_ZERO) == 0) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void bpl() {
    if ((status & FLAG_SIGN) == 0) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void brk() {
    pc++;
    push16(pc); //push next instruction address onto stack
    push8(status | FLAG_BREAK); //push CPU status to stack
    setinterrupt(); //set interrupt flag
    pc = (uint16_t)read6502(0xFFFE) | ((uint16_t)read6502(0xFFFF) << 8);
}

static void bvc() {
    if ((status & FLAG_OVERFLOW) == 0) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void bvs() {
    if ((status & FLAG_OVERFLOW) == FLAG_OVERFLOW) {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2; //check if jump crossed a page boundary
            else clockticks6502++;
    }
}

static void clc() {
    clearcarry();
}

static void cld() {
    cleardecimal();
}

static void cli() {
    clearinterrupt();
}

static void clv() {
    clearoverflow();
}

static void cmp() {
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a - value;

    if (a >= (uint8_t)(value & 0x00FF)) setcarry();
        else clearcarry();
    if (a == (uint8_t)(value & 0x00FF)) setzero();
        else clearzero();
    signcalc(result);
}

static void cpx() {
    value = getvalue();
    result = (uint16_t)x - value;

    if (x >= (uint8_t)(value & 0x00FF)) setcarry();
        else clearcarry();
    if (x == (uint8_t)(value & 0x00FF)) setzero();
        else clearzero();
    signcalc(result);
}

static void cpy() {
    value = getvalue();
    result = (uint16_t)y - value;

    if (y >= (uint8_t)(value & 0x00FF)) setcarry();
        else clearcarry();
    if (y == (uint8_t)(value & 0x00FF)) setzero();
        else clearzero();
    signcalc(result);
}

static void dec() {
    value = getvalue();
    result = value - 1;

    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void dex() {
    x--;

    zerocalc(x);
    signcalc(x);
}

static void dey() {
    y--;

    zerocalc(y);
    signcalc(y);
}

static void eor() {
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a ^ value;

    zerocalc(result);
    signcalc(result);

    saveaccum(result);
}

static void inc() {
    value = getvalue();
    result = value + 1;

    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void inx() {
    x++;

    zerocalc(x);
    signcalc(x);
}

static void iny() {
    y++;

    zerocalc(y);
    signcalc(y);
}

static void jmp() {
    pc = ea;
}

static void jsr() {
    push16(pc - 1);
    pc = ea;
}

static void lda() {
    penaltyop = 1;
    value = getvalue();
    a = (uint8_t)(value & 0x00FF);

    zerocalc(a);
    signcalc(a);
}

static void ldx() {
    penaltyop = 1;
    value = getvalue();
    x = (uint8_t)(value & 0x00FF);

    zerocalc(x);
    signcalc(x);
}

static void ldy() {
    penaltyop = 1;
    value = getvalue();
    y = (uint8_t)(value & 0x00FF);

    zerocalc(y);
    signcalc(y);
}

static void lsr() {
    value = getvalue();
    result = value >> 1;

    if (value & 1) setcarry();
        else clearcarry();
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void nop() {
    switch (opcode) {
        case 0x1C:
        case 0x3C:
        case 0x5C:
        case 0x7C:
        case 0xDC:
        case 0xFC:
            penaltyop = 1;
            break;
    }
}

static void ora() {
    penaltyop = 1;
    value = getvalue();
    result = (uint16_t)a | value;

    zerocalc(result);
    signcalc(result);

    saveaccum(result);
}

static void pha() {
    push8(a);
}

static void php() {
    push8(status | FLAG_BREAK);
}

static void phx() {
    push8(x);
}

static void phy() {
    push8(y);
}

static void pla() {
    a = pull8();

    zerocalc(a);
    signcalc(a);
}

static void plp() {
    status = pull8() | FLAG_CONSTANT;
}

static void plx() {
    x = pull8();

    zerocalc(x);
    signcalc(x);
}

static void ply() {
    y = pull8();

    zerocalc(y);
    signcalc(y);
}

static void rol() {
    value = getvalue();
    result = (value << 1) | (status & FLAG_CARRY);

    carrycalc(result);
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void ror() {
    value = getvalue();
    result = (value >> 1) | ((status & FLAG_CARRY) << 7);

    if (value & 1) setcarry();
        else clearcarry();
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

static void rti() {
    status = pull8();
    value = pull16();
    pc = value;
}

static void rts() {
    value = pull16();
    pc = value + 1;
}

static void sbc() {
    penaltyop = 1;
    value = getvalue() ^ 0x00FF;
    result = (uint16_t)a + value + (uint16_t)(status & FLAG_CARRY);

    carrycalc(result);
    zerocalc(result);
    overflowcalc(result, a, value);
    signcalc(result);

    #ifndef NES_CPU
    if (status & FLAG_DECIMAL) {
        clearcarry();

        a -= 0x66;
        if ((a & 0x0F) > 0x09) {
            a += 0x06;
        }
        if ((a & 0xF0) > 0x90) {
            a += 0x60;
            setcarry();
        }

        clockticks6502++;
    }
    #endif

    saveaccum(result);
}

static void sec() {
    setcarry();
}

static void sed() {
    setdecimal();
}

static void sei() {
    setinterrupt();
}

static void sta() {
    putvalue(a);
}

static void stx() {
    putvalue(x);
}

static void sty() {
    putvalue(y);
}

static void stz() {
    putvalue(0);
}

static void tax() {
    x = a;

    zerocalc(x);
    signcalc(x);
}

static void tay() {
    y = a;

    zerocalc(y);
    signcalc(y);
}

static void tsx() {
    x = sp;

    zerocalc(x);
    signcalc(x);
}

static void txa() {
    a = x;

    zerocalc(a);
    signcalc(a);
}

static void txs() {
    sp = x;
}

static void tya() {
    a = y;

    zerocalc(a);
    signcalc(a);
}

static void tsb() {
    value = getvalue();
    zerocalc(value & a);
    putvalue(value | a);
}

static void trb() {
    value = getvalue();
    zerocalc(value & a);
    putvalue(value & ~a);
}

#define DEF_BBR(idx)                                                           \
static void bbr##idx() {                                                       \
    value = getvalue();                                                        \
    if ((value & (1 << (idx))) == 0) {                                         \
        oldpc = pc;                                                            \
        pc += reladdr;                                                         \
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2;            \
            else clockticks6502++;                                             \
    }                                                                          \
}
DEF_BBR(0)
DEF_BBR(1)
DEF_BBR(2)
DEF_BBR(3)
DEF_BBR(4)
DEF_BBR(5)
DEF_BBR(6)
DEF_BBR(7)

#define DEF_BBS(idx)                                                           \
static void bbs##idx() {                                                       \
    value = getvalue();                                                        \
    if ((value & (1 << (idx))) != 0) {                                         \
        oldpc = pc;                                                            \
        pc += reladdr;                                                         \
        if ((oldpc & 0xFF00) != (pc & 0xFF00)) clockticks6502 += 2;            \
            else clockticks6502++;                                             \
    }                                                                          \
}
DEF_BBS(0)
DEF_BBS(1)
DEF_BBS(2)
DEF_BBS(3)
DEF_BBS(4)
DEF_BBS(5)
DEF_BBS(6)
DEF_BBS(7)

#define DEF_RMB(idx)                                                           \
static void rmb##idx() {                                                       \
    value = getvalue();                                                        \
    value &= ~(1 << (idx));                                                    \
    putvalue(value);                                                           \
}
DEF_RMB(0)
DEF_RMB(1)
DEF_RMB(2)
DEF_RMB(3)
DEF_RMB(4)
DEF_RMB(5)
DEF_RMB(6)
DEF_RMB(7)

#define DEF_SMB(idx)                                                           \
static void smb##idx() {                                                       \
    value = getvalue();                                                        \
    value |= 1 << (idx);                                                       \
    putvalue(value);                                                           \
}
DEF_SMB(0)
DEF_SMB(1)
DEF_SMB(2)
DEF_SMB(3)
DEF_SMB(4)
DEF_SMB(5)
DEF_SMB(6)
DEF_SMB(7)

// TODO: Implement these by adding emulation wait and stop states.
static void wai() {}
static void stp() {}

//undocumented instructions
#ifdef UNDOCUMENTED
    static void lax() {
        lda();
        ldx();
    }

    static void sax() {
        sta();
        stx();
        putvalue(a & x);
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void dcp() {
        dec();
        cmp();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void isb() {
        inc();
        sbc();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void slo() {
        asl();
        ora();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void rla() {
        rol();
        and();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void sre() {
        lsr();
        eor();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }

    static void rra() {
        ror();
        adc();
        if (penaltyop && penaltyaddr) clockticks6502--;
    }
#else
    #define lax nop
    #define sax nop
    #define dcp nop
    #define isb nop
    #define slo nop
    #define rla nop
    #define sre nop
    #define rra nop
#endif


static void (*addrtable_nmos[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 0 */
/* 1 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 1 */
/* 2 */    abso, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 2 */
/* 3 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 3 */
/* 4 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 4 */
/* 5 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 5 */
/* 6 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm,  ind, abso, abso, abso, /* 6 */
/* 7 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 7 */
/* 8 */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* 8 */
/* 9 */     rel, indy,  imp, indy,  zpx,  zpx,  zpy,  zpy,  imp, absy,  imp, absy, absx, absx, absy, absy, /* 9 */
/* A */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* A */
/* B */     rel, indy,  imp, indy,  zpx,  zpx,  zpy,  zpy,  imp, absy,  imp, absy, absx, absx, absy, absy, /* B */
/* C */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* C */
/* D */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* D */
/* E */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* E */
/* F */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx  /* F */
};

static void (*optable_nmos[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */     brk,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  php,  ora,  asl,  nop,  nop,  ora,  asl,  slo, /* 0 */
/* 1 */     bpl,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  clc,  ora,  nop,  slo,  nop,  ora,  asl,  slo, /* 1 */
/* 2 */     jsr,  and,  nop,  rla,  bit,  and,  rol,  rla,  plp,  and,  rol,  nop,  bit,  and,  rol,  rla, /* 2 */
/* 3 */     bmi,  and,  nop,  rla,  nop,  and,  rol,  rla,  sec,  and,  nop,  rla,  nop,  and,  rol,  rla, /* 3 */
/* 4 */     rti,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  pha,  eor,  lsr,  nop,  jmp,  eor,  lsr,  sre, /* 4 */
/* 5 */     bvc,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  cli,  eor,  nop,  sre,  nop,  eor,  lsr,  sre, /* 5 */
/* 6 */     rts,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  pla,  adc,  ror,  nop,  jmp,  adc,  ror,  rra, /* 6 */
/* 7 */     bvs,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  sei,  adc,  nop,  rra,  nop,  adc,  ror,  rra, /* 7 */
/* 8 */     nop,  sta,  nop,  sax,  sty,  sta,  stx,  sax,  dey,  nop,  txa,  nop,  sty,  sta,  stx,  sax, /* 8 */
/* 9 */     bcc,  sta,  nop,  nop,  sty,  sta,  stx,  sax,  tya,  sta,  txs,  nop,  nop,  sta,  nop,  nop, /* 9 */
/* A */     ldy,  lda,  ldx,  lax,  ldy,  lda,  ldx,  lax,  tay,  lda,  tax,  nop,  ldy,  lda,  ldx,  lax, /* A */
/* B */     bcs,  lda,  nop,  lax,  ldy,  lda,  ldx,  lax,  clv,  lda,  tsx,  lax,  ldy,  lda,  ldx,  lax, /* B */
/* C */     cpy,  cmp,  nop,  dcp,  cpy,  cmp,  dec,  dcp,  iny,  cmp,  dex,  nop,  cpy,  cmp,  dec,  dcp, /* C */
/* D */     bne,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp,  cld,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp, /* D */
/* E */     cpx,  sbc,  nop,  isb,  cpx,  sbc,  inc,  isb,  inx,  sbc,  nop,  sbc,  cpx,  sbc,  inc,  isb, /* E */
/* F */     beq,  sbc,  nop,  isb,  nop,  sbc,  inc,  isb,  sed,  sbc,  nop,  isb,  nop,  sbc,  inc,  isb  /* F */
};

static const uint32_t ticktable_nmos[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */      7,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    4,    4,    6,    6,  /* 0 */
/* 1 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 1 */
/* 2 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    4,    4,    6,    6,  /* 2 */
/* 3 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 3 */
/* 4 */      6,    6,    2,    8,    3,    3,    5,    5,    3,    2,    2,    2,    3,    4,    6,    6,  /* 4 */
/* 5 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 5 */
/* 6 */      6,    6,    2,    8,    3,    3,    5,    5,    4,    2,    2,    2,    5,    4,    6,    6,  /* 6 */
/* 7 */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* 7 */
/* 8 */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* 8 */
/* 9 */      2,    6,    2,    6,    4,    4,    4,    4,    2,    5,    2,    5,    5,    5,    5,    5,  /* 9 */
/* A */      2,    6,    2,    6,    3,    3,    3,    3,    2,    2,    2,    2,    4,    4,    4,    4,  /* A */
/* B */      2,    5,    2,    5,    4,    4,    4,    4,    2,    4,    2,    4,    4,    4,    4,    4,  /* B */
/* C */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* C */
/* D */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7,  /* D */
/* E */      2,    6,    2,    8,    3,    3,    5,    5,    2,    2,    2,    2,    4,    4,    6,    6,  /* E */
/* F */      2,    5,    2,    8,    4,    4,    6,    6,    2,    4,    2,    7,    4,    4,    7,    7   /* F */
};

static void (*addrtable_cmos[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */     imp, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imp, abso, abso, abso,  zpr, /* 0 */
/* 1 */     rel, indy, inzp,  imp,   zp,  zpx,  zpx,   zp,  imp, absy,  imp,  imp, abso, absx, absx,  zpr, /* 1 */
/* 2 */    abso, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imp, abso, abso, abso,  zpr, /* 2 */
/* 3 */     rel, indy, inzp,  imp,  zpx,  zpx,  zpx,   zp,  imp, absy,  imp,  imp, absx, absx, absx,  zpr, /* 3 */
/* 4 */     imp, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imp, abso, abso, abso,  zpr, /* 4 */
/* 5 */     rel, indy, inzp,  imp,  zpx,  zpx,  zpx,   zp,  imp, absy,  imp,  imp, abso, absx, absx,  zpr, /* 5 */
/* 6 */     imp, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imp,  ind, abso, abso,  zpr, /* 6 */
/* 7 */     rel, indy, inzp,  imp,  zpx,  zpx,  zpx,   zp,  imp, absy,  imp,  imp, inax, absx, absx,  zpr, /* 7 */
/* 8 */     rel, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imp, abso, abso, abso,  zpr, /* 8 */
/* 9 */     rel, indy, inzp,  imp,  zpx,  zpx,  zpy,   zp,  imp, absy,  imp,  imp, abso, absx, absx,  zpr, /* 9 */
/* A */     imm, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imp, abso, abso, abso,  zpr, /* A */
/* B */     rel, indy, inzp,  imp,  zpx,  zpx,  zpy,   zp,  imp, absy,  imp,  imp, absx, absx, absy,  zpr, /* B */
/* C */     imm, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imp, abso, abso, abso,  zpr, /* C */
/* D */     rel, indy, inzp,  imp,  zpx,  zpx,  zpx,   zp,  imp, absy,  imp,  imp, abso, absx, absx,  zpr, /* D */
/* E */     imm, indx,  imm,  imp,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imp, abso, abso, abso,  zpr, /* E */
/* F */     rel, indy, inzp,  imp,  zpx,  zpx,  zpx,   zp,  imp, absy,  imp,  imp, abso, absx, absx,  zpr  /* F */
};

static void (*optable_cmos[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7   |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F   |     */
/* 0 */     brk,  ora,  nop,  nop,  tsb,  ora,  asl,  rmb0,  php,  ora,  asl,  nop,  tsb,  ora,  asl,  bbr0, /* 0 */
/* 1 */     bpl,  ora,  ora,  nop,  trb,  ora,  asl,  rmb1,  clc,  ora,  inc,  nop,  trb,  ora,  asl,  bbr1, /* 1 */
/* 2 */     jsr,  and,  nop,  nop,  bit,  and,  rol,  rmb2,  plp,  and,  rol,  nop,  bit,  and,  rol,  bbr2, /* 2 */
/* 3 */     bmi,  and,  and,  nop,  bit,  and,  rol,  rmb3,  sec,  and,  dec,  nop,  bit,  and,  rol,  bbr3, /* 3 */
/* 4 */     rti,  eor,  nop,  nop,  nop,  eor,  lsr,  rmb4,  pha,  eor,  lsr,  nop,  jmp,  eor,  lsr,  bbr4, /* 4 */
/* 5 */     bvc,  eor,  eor,  nop,  nop,  eor,  lsr,  rmb5,  cli,  eor,  phy,  nop,  nop,  eor,  lsr,  bbr5, /* 5 */
/* 6 */     rts,  adc,  nop,  nop,  stz,  adc,  ror,  rmb6,  pla,  adc,  ror,  nop,  jmp,  adc,  ror,  bbr6, /* 6 */
/* 7 */     bvs,  adc,  adc,  nop,  stz,  adc,  ror,  rmb7,  sei,  adc,  ply,  nop,  jmp,  adc,  ror,  bbr7, /* 7 */
/* 8 */     bra,  sta,  nop,  nop,  sty,  sta,  stx,  smb0,  dey,  bit,  txa,  nop,  sty,  sta,  stx,  bbs0, /* 8 */
/* 9 */     bcc,  sta,  sta,  nop,  sty,  sta,  stx,  smb1,  tya,  sta,  txs,  nop,  stz,  sta,  stz,  bbs1, /* 9 */
/* A */     ldy,  lda,  ldx,  nop,  ldy,  lda,  ldx,  smb2,  tay,  lda,  tax,  nop,  ldy,  lda,  ldx,  bbs2, /* A */
/* B */     bcs,  lda,  lda,  nop,  ldy,  lda,  ldx,  smb3,  clv,  lda,  tsx,  nop,  ldy,  lda,  ldx,  bbs3, /* B */
/* C */     cpy,  cmp,  nop,  nop,  cpy,  cmp,  dec,  smb4,  iny,  cmp,  dex,  wai,  cpy,  cmp,  dec,  bbs4, /* C */
/* D */     bne,  cmp,  cmp,  nop,  nop,  cmp,  dec,  smb5,  cld,  cmp,  phx,  stp,  nop,  cmp,  dec,  bbs5, /* D */
/* E */     cpx,  sbc,  nop,  nop,  cpx,  sbc,  inc,  smb6,  inx,  sbc,  nop,  nop,  cpx,  sbc,  inc,  bbs6, /* E */
/* F */     beq,  sbc,  sbc,  nop,  nop,  sbc,  inc,  smb7,  sed,  sbc,  plx,  nop,  nop,  sbc,  inc,  bbs7  /* F */
};

static const uint32_t ticktable_cmos[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */      7,    6,    2,    1,    5,    3,    5,    5,    3,    2,    2,    1,    6,    4,    6,    5,  /* 0 */
/* 1 */      2,    5,    5,    1,    5,    4,    6,    5,    2,    4,    2,    1,    6,    4,    6,    5,  /* 1 */
/* 2 */      6,    6,    2,    1,    3,    3,    5,    5,    4,    2,    2,    1,    4,    4,    6,    5,  /* 2 */
/* 3 */      2,    5,    5,    1,    4,    4,    6,    5,    2,    4,    2,    1,    4,    4,    6,    5,  /* 3 */
/* 4 */      6,    6,    2,    1,    3,    3,    5,    5,    3,    2,    2,    1,    3,    4,    6,    5,  /* 4 */
/* 5 */      2,    5,    5,    1,    4,    4,    6,    5,    2,    4,    3,    1,    8,    4,    6,    5,  /* 5 */
/* 6 */      6,    6,    2,    1,    3,    3,    5,    5,    4,    2,    2,    1,    6,    4,    6,    5,  /* 6 */
/* 7 */      2,    5,    5,    1,    4,    4,    6,    5,    2,    4,    4,    1,    6,    4,    6,    5,  /* 7 */
/* 8 */      3,    6,    2,    1,    3,    3,    3,    5,    2,    2,    2,    1,    4,    4,    4,    5,  /* 8 */
/* 9 */      2,    6,    5,    1,    4,    4,    4,    5,    2,    5,    2,    1,    4,    5,    5,    5,  /* 9 */
/* A */      2,    6,    2,    1,    3,    3,    3,    5,    2,    2,    2,    1,    4,    4,    4,    5,  /* A */
/* B */      2,    5,    5,    1,    4,    4,    4,    5,    2,    4,    2,    1,    4,    4,    4,    5,  /* B */
/* C */      2,    6,    2,    1,    3,    3,    5,    5,    2,    2,    2,    1,    4,    4,    6,    5,  /* C */
/* D */      2,    5,    5,    1,    4,    4,    6,    5,    2,    4,    3,    1,    4,    4,    7,    5,  /* D */
/* E */      2,    6,    2,    1,    3,    3,    5,    5,    2,    2,    2,    1,    4,    4,    6,    5,  /* E */
/* F */      2,    5,    5,    1,    4,    4,    6,    5,    2,    4,    4,    1,    4,    4,    7,    5   /* F */
};

// TODO: These are not emitted by LLVM, but may be used eventually.
static void immd() { pc += 2; }
static void ddds() { pc += 2; }
static void cbne() { pc += 2; }
static void rel2() { pc += 2; }
static void up() { pc += 1; }

static void (*addrtable_spc700[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */     imp,  imp,  zp,   zpr,  zp,  abso, imp,  indx, imm,  ddds, abso, zp,   abso,  imp, abso, imp,  /* 0 */
/* 1 */     rel,  imp,  zp,   zpr,  zpx, absx, absy, indy, immd, imp,  zp,   zpx,  acc,   imp, abso, abso, /* 1 */
/* 2 */     imp,  imp,  zp,   zpr,  zp,  abso, imp,  indx, imm,  ddds, abso, zp,   abso,  imp, rel2, rel,  /* 2 */
/* 3 */     rel,  imp,  zp,   zpr,  zpx, absx, absy, indy, immd, imp,  zp,   zpx,  acc,   imp, zp,   abso, /* 3 */
/* 4 */     imp,  imp,  zp,   zpr,  zp,  abso, imp,  indx, imm,  ddds, abso, zp,   abso,  imp, abso, up,   /* 4 */
/* 5 */     rel,  imp,  zp,   zpr,  zpx, absx, absy, indy, immd, imp,  zp,   zpx,  acc,   imp, abso, abso, /* 5 */
/* 6 */     imp,  imp,  zp,   zpr,  zp,  abso, imp,  indx, imm,  ddds, abso, zp,   abso,  imp, rel2, imp,  /* 6 */
/* 7 */     rel,  imp,  zp,   zpr,  zpx, absx, absy, indy, immd, imp,  zp,   zpx,  acc,   imp, zp,   imp,  /* 7 */
/* 8 */     imp,  imp,  zp,   zpr,  zp,  abso, imp,  indx, imm,  ddds, abso, zp,   abso,  imm, imp,  immd, /* 8 */
/* 9 */     rel,  imp,  zp,   zpr,  zpx, absx, absy, indy, immd, imp,  zp,   zpx,  acc,   imp, imp,  acc,  /* 9 */
/* A */     imp,  imp,  zp,   zpr,  zp,  abso, imp,  indx, imm,  ddds, abso, zp,   abso,  imm, imp,  imp,  /* A */
/* B */     rel,  imp,  zp,   zpr,  zpx, absx, absy, indy, immd, imp,  zp,   zpx,  acc,   imp, imp,  imp,  /* B */
/* C */     imp,  imp,  zp,   zpr,  zp,  abso, imp,  indx, imm,  abso, abso, zp,   abso,  imm, imp,  imp,  /* C */
/* D */     rel,  imp,  zp,   zpr,  zpx, absx, absy, indy, zp,   zpy,  zp,   zpx,  imp,   imp, cbne, imp,  /* D */
/* E */     imp,  imp,  zp,   zpr,  zp,  abso, imp,  indx, imm,  abso, abso, zp,   abso,  imp, imp,  imp,  /* E */
/* F */     rel,  imp,  zp,   zpr,  zpx, absx, absy, indy, zp,   zpy,  ddds, zpx,  imp,   imp, rel2, imp,  /* F */
};

// TODO: These are not emitted by LLVM, but may be used eventually.
static void clp() {}
static void sep() {}

static void (*optable_spc700[256])() = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */     nop,  nop,  nop,  nop,  ora,  ora,  nop,  ora,  ora,  nop,  nop,  asl,  asl,  php,  nop,  nop, /* 0 */
/* 1 */     bpl,  nop,  nop,  nop,  ora,  ora,  ora,  ora,  nop,  nop,  nop,  asl,  asl,  dex,  cpx,  jmp, /* 1 */
/* 2 */     clp,  nop,  nop,  nop,  and,  and,  nop,  and,  and,  nop,  nop,  rol,  rol,  pha,  nop,  bra, /* 2 */
/* 3 */     bmi,  nop,  nop,  nop,  and,  and,  and,  and,  nop,  nop,  nop,  rol,  rol,  inx,  cpx,  jsr, /* 3 */
/* 4 */     sep,  nop,  nop,  nop,  eor,  eor,  nop,  eor,  eor,  nop,  nop,  lsr,  lsr,  phx,  nop,  nop, /* 4 */
/* 5 */     bvc,  nop,  nop,  nop,  eor,  eor,  eor,  eor,  nop,  nop,  nop,  lsr,  lsr,  tax,  cpy,  jmp, /* 5 */
/* 6 */     clc,  nop,  nop,  nop,  cmp,  cmp,  nop,  cmp,  cmp,  nop,  nop,  ror,  ror,  phy,  nop,  rts, /* 6 */
/* 7 */     bvs,  nop,  nop,  nop,  cmp,  cmp,  cmp,  cmp,  nop,  nop,  nop,  ror,  ror,  txa,  cpy,  rti, /* 7 */
/* 8 */     sec,  nop,  nop,  nop,  adc,  adc,  nop,  adc,  adc,  nop,  nop,  dec,  dec,  ldy,  plp,  nop, /* 8 */
/* 9 */     bcc,  nop,  nop,  nop,  adc,  adc,  adc,  adc,  nop,  nop,  nop,  dec,  dec,  tsx,  nop,  nop, /* 9 */
/* A */     cli,  nop,  nop,  nop,  sbc,  sbc,  nop,  sbc,  sbc,  nop,  nop,  inc,  inc,  cpy,  pla,  nop, /* A */
/* B */     bcs,  nop,  nop,  nop,  sbc,  sbc,  sbc,  sbc,  nop,  nop,  nop,  inc,  inc,  txs,  nop,  nop, /* B */
/* C */     sei,  nop,  nop,  nop,  sta,  sta,  nop,  sta,  cpx,  stx,  nop,  sty,  sty,  ldx,  plx,  nop, /* C */
/* D */     bne,  nop,  nop,  nop,  sta,  sta,  sta,  sta,  stx,  stx,  nop,  sty,  dey,  tya,  nop,  nop, /* D */
/* E */     clv,  nop,  nop,  nop,  lda,  lda,  nop,  lda,  lda,  ldx,  nop,  ldy,  ldy,  nop,  ply,  nop, /* E */
/* F */     beq,  nop,  nop,  nop,  lda,  lda,  lda,  lda,  ldx,  ldx,  nop,  ldy,  iny,  tay,  nop,  nop, /* F */
};

static const uint32_t ticktable_spc700[256] = {
/*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
/* 0 */      2,    8,    4,    5,    3,    4,    3,    6,    2,    6,    5,    4,    5,    4,    6,    8,  /* 0 */
/* 1 */      2,    8,    4,    5,    4,    5,    5,    6,    5,    5,    6,    5,    2,    2,    4,    6,  /* 1 */
/* 2 */      2,    8,    4,    5,    3,    4,    3,    6,    2,    6,    5,    4,    5,    4,    5,    4,  /* 2 */
/* 3 */      2,    8,    4,    5,    4,    5,    5,    6,    5,    5,    6,    5,    2,    2,    3,    8,  /* 3 */
/* 4 */      2,    8,    4,    5,    3,    4,    3,    6,    2,    6,    4,    4,    5,    4,    6,    6,  /* 4 */
/* 5 */      2,    8,    4,    5,    4,    5,    5,    6,    5,    5,    4,    5,    2,    2,    4,    3,  /* 5 */
/* 6 */      2,    8,    4,    5,    3,    4,    3,    6,    2,    6,    4,    4,    5,    4,    5,    5,  /* 6 */
/* 7 */      2,    8,    4,    5,    4,    5,    5,    6,    5,    5,    5,    5,    2,    2,    3,    6,  /* 7 */
/* 8 */      2,    8,    4,    5,    3,    4,    3,    6,    2,    6,    5,    4,    5,    2,    4,    5,  /* 8 */
/* 9 */      2,    8,    4,    5,    4,    5,    5,    6,    5,    5,    5,    5,    2,    2,    12,   5,  /* 9 */
/* A */      3,    8,    4,    5,    3,    4,    3,    6,    2,    6,    4,    4,    5,    2,    4,    4,  /* A */
/* B */      2,    8,    4,    5,    4,    5,    5,    6,    5,    5,    5,    5,    2,    2,    3,    4,  /* B */
/* C */      3,    8,    4,    5,    3,    5,    4,    7,    2,    5,    6,    4,    5,    2,    4,    9,  /* C */
/* D */      2,    8,    4,    5,    4,    6,    6,    7,    4,    5,    5,    5,    2,    2,    6,    3,  /* D */
/* E */      2,    8,    4,    5,    3,    4,    3,    6,    2,    4,    5,    3,    4,    3,    4,    3,  /* E */
/* F */      2,    8,    4,    5,    4,    5,    5,    6,    3,    4,    5,    4,    2,    2,    5,    3   /* F */
};

void nmi6502() {
    push16(pc);
    push8(status);
    status |= FLAG_INTERRUPT;
    pc = (uint16_t)read6502(0xFFFA) | ((uint16_t)read6502(0xFFFB) << 8);
}

void irq6502() {
    push16(pc);
    push8(status);
    status |= FLAG_INTERRUPT;
    pc = (uint16_t)read6502(0xFFFE) | ((uint16_t)read6502(0xFFFF) << 8);
}

uint8_t callexternal = 0;
void (*loopexternal)();

void exec6502(uint32_t tickcount) {
    clockgoal6502 += tickcount;

    while (clockticks6502 < clockgoal6502) {
        opcode = read6502(pc++);
        status |= FLAG_CONSTANT;

        penaltyop = 0;
        penaltyaddr = 0;

        (*addrtable[opcode])();
        (*optable[opcode])();
        clockticks6502 += ticktable[opcode];
        if (penaltyop && penaltyaddr) clockticks6502++;

        instructions++;

        if (callexternal) (*loopexternal)();
    }

}

void reset6502(uint8_t cmos, uint8_t spc700) {
    if (spc700 != 0) {
        addrtable = addrtable_spc700;
        optable = optable_spc700;
        ticktable = ticktable_spc700;
    } else if (cmos != 0) {
        addrtable = addrtable_cmos;
        optable = optable_cmos;
        ticktable = ticktable_cmos;
    } else {
        addrtable = addrtable_nmos;
        optable = optable_nmos;
        ticktable = ticktable_nmos;
    }

    pc = (uint16_t)read6502(0xFFFC) | ((uint16_t)read6502(0xFFFD) << 8);
    a = 0;
    x = 0;
    y = 0;
    sp = 0xFD;
    status |= FLAG_CONSTANT;
}

void step6502() {
    opcode = read6502(pc++);
    status |= FLAG_CONSTANT;

    penaltyop = 0;
    penaltyaddr = 0;

    (*addrtable[opcode])();
    (*optable[opcode])();
    clockticks6502 += ticktable[opcode];
    if (penaltyop && penaltyaddr) clockticks6502++;
    clockgoal6502 = clockticks6502;

    instructions++;

    if (callexternal) (*loopexternal)();
}

void hookexternal(void *funcptr) {
    if (funcptr != (void *)NULL) {
        loopexternal = funcptr;
        callexternal = 1;
    } else callexternal = 0;
}
