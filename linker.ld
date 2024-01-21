/* ELF entry symbol - no strictly needed since, by default, the entry point 
is the start of the first section with code*/

ENTRY(_start)

/* defines the layout of specific sections in the output binary */
SECTIONS {
    /* sets current address to 0x80000 */
    . = 0x80000;

    /* defines .text.boot section */
    .text.boot : {
        /* ensures any content in input object files matching ".text.boot" 
        is kept and placed in the ".text.boot" section of output binary */
        KEEP(*(.text.boot))
    }
}
