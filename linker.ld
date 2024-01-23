/* ELF entry symbol - no strictly needed since, by default, the entry point 
is _start and it is the beginning of the text section. If the beginning isn't code
we will need to define our own entrypoint */

ENTRY(start)

/* defines the layout of specific sections in the output binary */
SECTIONS {

    /* sets current address to 0x80000, start of kernel8.img which is loaded by RPI*/
    . = 0x80000;

    /* defines .text sections */
    .text.boot : {
        /* KEEP ensures any content in input object files matching what is in KEEP
        is kept and placed in the same section of output binary 
        even if no symbols are referenced inside. Important if garbage collection 
        is enabled at link time */
        KEEP(*(.text.boot))
    }

    .text.evt : {
        KEEP(*(.text.evt)) /* exception vector table */
    }

    .text : { 
        *(.text*)
    }

    /* define data section, initialized global and static data*/
    /* TODO: note sure if we need to align here or if 16 bytes is correct */
    . = ALIGN(16);
	.data : { *(.data) }

    /* define BSS (block starting symbol) section, uninitialized global and static data*/
    /* TODO: note sure if we need to align here or if 16 bytes is correct */
    /* TODO: memset all this data to 0*/
    . = ALIGN(16);
    bss_begin = .;
    .bss : { *(.bss) }
    bss_end = .;
}
