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
        /* .init_array is a sorted list of initialization function pointers.
            Clang registers global constructor calls as function pointers in .init_array */
        __init_array_begin = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;

        *(.rodata .rodata.* .gnu.linkonce.r.*)
        SORT(CONSTRUCTORS)
    } >ram

    .data : {
        *(.data .data.* .gnu.linkonce.d.*)

        /* .fini_array is a sorted list of deinitialization function pointers.
        The compiler could place function pointers in this array. The equivalent of
        __cxa_finalize has an entry here. For this reason, .fini_array must be writeable. */
        __fini_array_begin = .;
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array))
        __fini_array_end = .;
    } >ram

    .bss : {
        __start_bss = .;
        *(.bss .bss.*)
        __end_bss = .;
        __heap_start = .;
    } >ram
}

/* Provide 128 imaginary registers */
PROVIDE(__rc0 = 0x0000);
INCLUDE imag_reg/127.ld
ASSERT(__rc0 == 0x0000, "Inconsistent zero page map.")
ASSERT(__rc127 == 0x007f, "Inconsistent zero page map.")

/* Set initial soft stack address to end of SPC RAM. (It grows down.) */
__stack = 0xFFBF;
