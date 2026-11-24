/* ELF layout objcopy turns into an EFI PE32+ application.
 * The entry symbol is efi_main. Firmware relocates the image, so the
 * dynamic relocation sections stay in the link.
 */
OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(efi_main)

SECTIONS {
    /* Entry address 0 makes objcopy drop the PE optional header. The
     * first page belongs to the DOS and PE headers, so code starts
     * at the next page.
     */
    . = 0x1000;

    .text : ALIGN(4096) {
        *(.text)
        *(.text.*)
    }

    .rodata : ALIGN(4096) {
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(4096) {
        *(.data)
        *(.data.*)
        *(.sdata)
    }

    .bss : ALIGN(4096) {
        *(.bss)
        *(.bss.*)
        *(.sbss)
    }

    .dynamic : { *(.dynamic) }
    .dynsym : { *(.dynsym) }
    .dynstr : { *(.dynstr) }
    /* One name. objcopy turns this section into the PE base-reloc table.
     * gcc emits .rela.data.rel.local, and a section left under that name
     * is ignored by the EFI target.
     */
    .rela.dyn : {
        *(.rela.*)
        *(.rela)
        *(.rel.*)
        *(.rel)
    }
    .reloc : { *(.reloc) }
    .hash : { *(.hash) }

    /DISCARD/ : {
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.note.*)
        *(.note.gnu.property)
        *(.comment)
        *(.gnu.hash)
    }
}
