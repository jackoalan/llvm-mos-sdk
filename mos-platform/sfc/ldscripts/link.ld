/* SFC lorom linker script. */

/*OUTPUT_FORMAT(binary)*/

MEMORY {
    ram (!rx)      : ORIGIN = 0x0100, LENGTH = 0x1F00
    sfc_regs (!rx) : ORIGIN = 0x2100, LENGTH = 0xFF
    cpu_regs (!rx) : ORIGIN = 0x4200, LENGTH = 0x20
    dma_regs (!rx) : ORIGIN = 0x4300, LENGTH = 0x80
    rom (rx)       : ORIGIN = 0x8000, LENGTH = 0x8000
}

SECTIONS {
    .sfc_regs (NOLOAD) : {
        __sfc_regs = .;
    } >sfc_regs

    .cpu_regs (NOLOAD) : {
        __cpu_regs = .;
    } >cpu_regs

    .dma_regs (NOLOAD) : {
        __dma_regs = .;
    } >dma_regs

    .init : {
        *(.init)
    } >rom

    .text : {
        *(.text .stub .text.* .gnu.linkonce.t.*)
    } >rom

    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        SORT(CONSTRUCTORS)
        __start_data_rom = .;
    } >rom

    .data : AT(__start_data_rom) {
        __start_data_ram = .;
        *(.data .data.* .gnu.linkonce.d.*)
        __end_data_ram = .;
        __data_dma_length = __end_data_ram - __start_data_ram;
    } >ram

    .bss : {
        __start_bss = .;
        *(.bss .bss.*)
        __end_bss = .;
        __heap_start = .;
    } >ram

    .sfc.header 0xFFB0 : {
        /* Maker Code */
        SHORT(0x0);
        /* Game Code */
        LONG(0x0);
        /* Reserved */
        LONG(0x0);
        SHORT(0x0);
        BYTE(0x0);
        /* Expansion RAM size */
        BYTE(0x0);
        /* Special Version */
        BYTE(0x0);
        /* Cartridge Type */
        BYTE(0x0);
        /* Title */
        LONG(0x4D564C4C);
        LONG(0x534F4D2D);
        LONG(0x53455420);
        LONG(0x50412054);
        LONG(0x20202050);
        BYTE(0x20);
        /* ROM makeup */
        BYTE(0x20);
        /* ROM type */
        BYTE(0x0);
        /* ROM size */
        BYTE(0x5);
        /* SRAM size */
        BYTE(0x0);
        /* Creator ID */
        SHORT(0xAAAA);
        /* Version */
        BYTE(0x0);
        /* Checksum complement */
        SHORT(0x0F0F);
        /* Checksum */
        SHORT(0xF0F0);

        /* Native mode interrupts */
        LONG(0xFFFFFFFF);
        /* COP */
        SHORT(0xFFFF);
        /* BRK */
        SHORT(0xFFFF);
        /* ABORT */
        SHORT(0xFFFF);
        /* NMI */
        SHORT(0xFFFF);
        /* Unused */
        SHORT(0xFFFF);
        /* IRQ */
        SHORT(0xFFFF);

        /* Emulation mode interrupts */
        LONG(0xFFFFFFFF);
        /* COP */
        SHORT(0xFFFF);
        /* Unused */
        SHORT(0xFFFF);
        /* ABORT */
        SHORT(0xFFFF);
        /* NMI */
        SHORT(DEFINED(_nmi) ? _nmi : 0xFFFF);
        /* RES */
        SHORT(_start);
        /* IRQ/BRK */
        SHORT(0xFFFF);
    } >rom

    .spc.elf (INFO) : {
        *(.init)
    }
}

/* Provide 128 imaginary registers */
PROVIDE(__rc0 = 0x0000);
INCLUDE imag_reg/127.ld
ASSERT(__rc0 == 0x0000, "Inconsistent zero page map.")
ASSERT(__rc127 == 0x007f, "Inconsistent zero page map.")

/* Set initial soft stack address to end of WRAM. (It grows down.) */
__stack = 0x1FFF;
