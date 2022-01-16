#include <spc.h>

extern "C" {
extern const u8 jarrea_brr_data[];
__attribute__((aligned(256)))
const u8 *const sdir[][2] = {{jarrea_brr_data, jarrea_brr_data}};
}

int main() {
  // Initialize DSP registers
  SPC_DSP_WRITE_REG(MLVOL, 0);
  SPC_DSP_WRITE_REG(MRVOL, 0);
  SPC_DSP_WRITE_REG(ELVOL, 0);
  SPC_DSP_WRITE_REG(ERVOL, 0);
  SPC_DSP_WRITE_REG(KON, 0);
  SPC_DSP_WRITE_REG(KOFF, 0);
  SPC_DSP_WRITE_REG(EFB, 0);
  SPC_DSP_WRITE_REG(PMON, 0);
  SPC_DSP_WRITE_REG(NON, 0);
  SPC_DSP_WRITE_REG(EON, 0);
  SPC_DSP_WRITE_REG(DIR, ((u16)sdir) >> 8);
  SPC_DSP_WRITE_REG(ESA, 0);
  SPC_DSP_WRITE_REG(EDL, 0);

  SPC_DSP_WRITE_REG(COEF[0].VAL, 127);
  SPC_DSP_WRITE_REG(COEF[1].VAL, 0);
  SPC_DSP_WRITE_REG(COEF[2].VAL, 0);
  SPC_DSP_WRITE_REG(COEF[3].VAL, 0);
  SPC_DSP_WRITE_REG(COEF[4].VAL, 0);
  SPC_DSP_WRITE_REG(COEF[5].VAL, 0);
  SPC_DSP_WRITE_REG(COEF[6].VAL, 0);
  SPC_DSP_WRITE_REG(COEF[7].VAL, 0);

#define SPC_DSP_INIT_VOICE(voice)                                              \
  SPC_DSP_WRITE_REG(VOICE[voice].LVOL, 0);                                     \
  SPC_DSP_WRITE_REG(VOICE[voice].RVOL, 0);                                     \
  SPC_DSP_WRITE_PITCH(voice, 0);                                               \
  SPC_DSP_WRITE_REG(VOICE[voice].SRCN, 0);                                     \
  SPC_DSP_WRITE_REG(VOICE[voice].ADSR1, 0);                                    \
  SPC_DSP_WRITE_REG(VOICE[voice].ADSR2, 0);                                    \
  SPC_DSP_WRITE_REG(VOICE[voice].GAIN, 0)
  SPC_DSP_INIT_VOICE(0);
  SPC_DSP_INIT_VOICE(1);
  SPC_DSP_INIT_VOICE(2);
  SPC_DSP_INIT_VOICE(3);
  SPC_DSP_INIT_VOICE(4);
  SPC_DSP_INIT_VOICE(5);
  SPC_DSP_INIT_VOICE(6);
  SPC_DSP_INIT_VOICE(7);

  // Play jarrea sample
  SPC_DSP_WRITE_REG(VOICE[0].SRCN, 0);
  SPC_DSP_WRITE_REG(VOICE[0].LVOL, 127);
  SPC_DSP_WRITE_REG(VOICE[0].RVOL, 127);
  SPC_DSP_WRITE_REG(VOICE[0].GAIN, 127);
  SPC_DSP_WRITE_REG(MLVOL, 127);
  SPC_DSP_WRITE_REG(MRVOL, 127);
  SPC_DSP_WRITE_PITCH(0, (1 << 12) * 44100 / 32000);
  SPC_DSP_WRITE_REG(FLG, 0x20); // Global unmute, echo disabled
  SPC_DSP_WRITE_REG(KON, 1);
  return 0;
}
