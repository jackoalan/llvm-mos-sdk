/* SPC700 linker script. */

/*OUTPUT_FORMAT(binary)*/

MEMORY {
    spc_regs (!rx) : ORIGIN = 0x00F0, LENGTH = 0x10
    ram (!rx)      : ORIGIN = 0x0200, LENGTH = 0xFDBF
}

SECTIONS {
    .spc_regs (NOLOAD) : {
        __spc_regs = .;
    } >spc_regs

    .init : {
        *(.init)
    } >ram

    .text : {
        *(.text .stub .text.* .gnu.linkonce.t.*)
    } >ram

    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        SORT(CONSTRUCTORS)
    } >ram

    .data : {
        *(.data .data.* .gnu.linkonce.d.*)
    } >ram

    .bss : {
        __start_bss = .;
        *(.bss .bss.*)
        __end_bss = .;
    } >ram
}

/* Provide 128 imaginary registers */
PROVIDE(__rc0 = 0x0000);
INCLUDE imag_reg/128.ld
ASSERT(__rc0 == 0x0000, "Inconsistent zero page map.")
ASSERT(__rc127 == 0x007f, "Inconsistent zero page map.")

/* Set initial soft stack address to end of SPC RAM. (It grows down.) */
__stack = 0xFFBF;
