OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(kmain)
PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
    dynamic PT_DYNAMIC FLAGS((1 << 1) | (1 << 2)) ; /* Dynamic PHDR for relocations */
}

SECTIONS
{
    . = 0xffffffff80000000;
    section_text_begin = .;
    .text : {
        *(.text .text.*)
    } :text
    section_text_end = .;

    . += CONSTANT(MAXPAGESIZE);
    section_const_data_begin = .;
    .rodata : {
        *(.rodata .rodata.*)
    } :rodata
    section_const_data_end = .;

    . += CONSTANT(MAXPAGESIZE);
    section_mut_data_begin = .;
    .data : {
        *(.data .data.*)
    } :data

    .dynamic : {
        *(.dynamic)
    } :data :dynamic

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data
    section_mut_data_end = .;
    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
