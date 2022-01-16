#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;

__attribute__((section(".zp")))
extern volatile struct {
  u8 UNDOCUMENTED;
  u8 CTRL;
  u8 DSPADDR;
  u8 DSPDATA;
  u8 APUIO0;
  u8 APUIO1;
  u8 APUIO2;
  u8 APUIO3;
  u8 REGMEM0;
  u8 REGMEM1;
  u8 TIMER0;
  u8 TIMER1;
  u8 TIMER2;
  u8 COUNTER0;
  u8 COUNTER1;
  u8 COUNTER2;
} __spc_regs;

typedef union dsp_regs {
  struct {
    u8 __p1[12];
    s8 MLVOL;
    u8 __p2[15];
    s8 MRVOL;
    u8 __p3[15];
    s8 ELVOL;
    u8 __p4[15];
    s8 ERVOL;
    u8 __p5[15];
    u8 KON;
    u8 __p6[15];
    u8 KOFF;
    u8 __p7[15];
    u8 FLG;
    u8 __p8[15];
    u8 ENDX;
  };
  struct {
    u8 __p9[13];
    s8 EFB;
    u8 __p10[31];
    u8 PMON;
    u8 __p11[15];
    u8 NON;
    u8 __p12[15];
    u8 EON;
    u8 __p13[15];
    u8 DIR;
    u8 __p14[15];
    u8 ESA;
    u8 __p15[15];
    u8 EDL;
  };
  struct {
    u8 __p16[15];
    s8 VAL;
  } COEF[8];
  struct {
    s8 LVOL;
    s8 RVOL;
    u8 PITCHLO;
    u8 PITCHHI;
    u8 SRCN;
    u8 ADSR1;
    u8 ADSR2;
    u8 GAIN;
    u8 ENVX;
    u8 OUTX;
    u8 __p17[6];
  } VOICE[8];
} dsp_regs_t;

inline u8 __spc_dsp_read_reg(u8 addr) {
  __spc_regs.DSPADDR = addr;
  return __spc_regs.DSPDATA;
}

#define SPC_DSP_READ_REG(reg)                                                  \
  __spc_dsp_read_reg(__builtin_offsetof(dsp_regs_t, reg))

inline void __spc_dsp_write_reg(u8 addr, u8 val) {
  __spc_regs.DSPADDR = addr;
  __spc_regs.DSPDATA = val;
}

#define SPC_DSP_WRITE_REG(reg, val)                                            \
  __spc_dsp_write_reg(__builtin_offsetof(dsp_regs_t, reg), val)

#define SPC_DSP_READ_PITCH(voice)                                              \
  (SPC_DSP_READ_REG(VOICE[voice].PITCHLO) |                                    \
   (SPC_DSP_READ_REG(VOICE[voice].PITCHHI) << 8))

#define SPC_DSP_WRITE_PITCH(voice, val)                                        \
  {                                                                            \
    SPC_DSP_WRITE_REG(VOICE[voice].PITCHLO, val & 0xff);                       \
    SPC_DSP_WRITE_REG(VOICE[voice].PITCHHI, val >> 8);                         \
  }

#ifdef __cplusplus
}
#endif
