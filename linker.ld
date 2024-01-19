ENTRY(_start)             /* ELF entry symbol - no strictly needed */
SECTIONS {
  . = 0x80000;            /* start text at this location */
  .text.boot : {          /* boot code must come first */
    KEEP(*(.text.boot))
  }
}
