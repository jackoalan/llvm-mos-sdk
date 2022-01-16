#include <sfc.h>
#include <stddef.h>

extern "C" {
extern const u8 spc_prg_bin_data[];
extern const u16 spc_prg_bin_size;
}

void boot_spc(const u8* data, u16 len, u16 dest, u16 entry) {
  // Wait for SPC IPL to be ready
  while (__sfc_regs.APUIO0 != 0xaa) {}
  while (__sfc_regs.APUIO1 != 0xbb) {}

  // Initiate transfer to dest
  __sfc_regs.APUIO1 = 1;
  __sfc_regs.APUIO2 = dest;
  __sfc_regs.APUIO3 = dest >> 8;
  __sfc_regs.APUIO0 = 0xcc;
  while (__sfc_regs.APUIO0 != 0xcc) {}

  // Transfer loop
  u16 counter;
  for (counter = 0; counter < len; ++counter) {
    __sfc_regs.APUIO1 = *data++;
    __sfc_regs.APUIO0 = counter;
    while (__sfc_regs.APUIO0 != (u8)counter) {}
  }

  // Finish transfer and jump to entry point
  __sfc_regs.APUIO1 = 0;
  __sfc_regs.APUIO2 = entry;
  __sfc_regs.APUIO3 = entry >> 8;
  __sfc_regs.APUIO0 = ++counter;
  while (__sfc_regs.APUIO0 != (u8)counter) {}
}

template <u16 N>
void boot_spc(const u8 (&data)[N], u16 dest, u16 entry) {
  boot_spc(data, N, dest, entry);
}

int main() {
  // Load first palette entry with cyan
  __sfc_regs.CGADD = 0;
  __sfc_regs.CGDATA = 0xe0;
  __sfc_regs.CGDATA = 0x7f;

  // Set display to full brightness
  __sfc_regs.INIDISP = 0xf;

  boot_spc(spc_prg_bin_data, spc_prg_bin_size, 0x0200, 0x0200);

  __sfc_regs.CGADD = 0;
  __sfc_regs.CGDATA = 0x00;
  __sfc_regs.CGDATA = 0x7f;

  return 0;
}
