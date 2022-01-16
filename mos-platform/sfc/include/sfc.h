#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t u8;
typedef uint16_t u16;

extern volatile struct __attribute__((packed)) {
  u8 INIDISP;
  u8 OBSEL;
  u16 OAMADDR;
  u8 OAMDATA;
  u8 BGMODE;
  u8 MOSAIC;
  u8 BG1SC;
  u8 BG2SC;
  u8 BG3SC;
  u8 BG4SC;
  u8 BG12NBA;
  u8 BG34NBA;
  u8 BG1HOFS;
  u8 BG1VOFS;
  u8 BG2HOFS;
  u8 BG2VOFS;
  u8 BG3HOFS;
  u8 BG3VOFS;
  u8 BG4HOFS;
  u8 BG4VOFS;
  u8 VMAIN;
  u16 VMADDR;
  u16 VMDATA;
  u8 M7SEL;
  u8 M7A;
  u8 M7B;
  u8 M7C;
  u8 M7D;
  u8 M7X;
  u8 M7Y;
  u8 CGADDR;
  u8 CGDATA;
  u8 W12SEL;
  u8 W34SEL;
  u8 WOBJSEL;
  u8 WH0;
  u8 WH1;
  u8 WH2;
  u8 WH3;
  u8 WBGLOG;
  u8 WOBJLOG;
  u8 TM;
  u8 TS;
  u8 TMW;
  u8 TSW;
  u8 CGWSEL;
  u8 CGADSUB;
  u8 COLDATA;
  u8 SETINI;
  u8 MPYL;
  u8 MPYM;
  u8 MPYH;
  u8 SLHV;
  u8 OAMDATAREAD;
  u16 VMDATAREAD;
  u8 CGDATAREAD;
  u8 OPHCT;
  u8 OPVCT;
  u8 STAT77;
  u8 STAT78;
  u8 APUIO0;
  u8 APUIO1;
  u8 APUIO2;
  u8 APUIO3;
  u8 WMDATA;
  u8 WMADDRL;
  u8 WMADDRM;
  u8 WMADDRH;
} __sfc_regs;

extern volatile struct __attribute__((packed)) {
  u8 NMITIMEN;
  u8 WRIO;
  u8 WRMPYA;
  u8 WRMPYB;
  u16 WRDIV;
  u8 WRDIVB;
  u16 HTIME;
  u16 VTIME;
  u8 MDMAEN;
  u8 HDMAEN;
  u8 MEMSEL;
  u8 RDNMI;
  u8 TIMEUP;
  u8 HVBJOY;
  u8 RDIO;
  u16 RDDIV;
  u16 RDMPY;
  u16 JOY1;
  u16 JOY2;
  u16 JOY3;
  u16 JOY4;
} __cpu_regs;

extern volatile struct __attribute__((packed)) {
  union {
    struct __attribute__((packed)) {
      u8 DMAP;
      u8 BBAD;
      u16 A1T;
      u8 A1B;
      u16 DAS;
      u8 __padding[9];
    } dma[8];
    struct __attribute__((packed)) {
      u8 DMAP;
      u8 BBAD;
      u16 A1T;
      u8 A1B;
      u16 DAS;
      u8 DASB;
      u16 A2A;
      u8 NTLR;
      u8 __padding[5];
    } hdma[8];
  };
} __dma_regs;

#ifdef __cplusplus
}
#endif
