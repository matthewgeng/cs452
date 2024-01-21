// not stored in memory (macro which is preprocessed during compile time)
.macro evec label
    .balign 0x80 // byte align 128
    b \label
.endm

// EXCEPTION VECTOR TABLE
.section .text.evt
.balign 0x800 // byte align 2KB 2048
.global exception_vector
exception_vector:

    // EL1t

    // The exception handler for a synchronous 
    // exception from the current EL using SP0.
    evec curr_el_sp0_sync
    
    // The exception handler for an IRQ exception
    // from the current EL using SP0.
    evec curr_el_sp0_irq

    // The exception handler for an FIQ exception
    // from the current EL using SP0.
    evec curr_el_sp0_fiq                    

    // The exception handler for a System Error 
    // exception from the current EL using SP0.
    evec curr_el_sp0_serror                        

    // EL1h
    
    // The exception handler for a synchrous 
    // exception from the current EL using the current SP. 
    evec curr_el_spx_sync

    // The exception handler for an IRQ exception from 
    // the current EL using the current SP.
    evec curr_el_spx_irq

    // The exception handler for an FIQ from 
    // the current EL using the current SP.
    evec curr_el_spx_fiq

    // The exception handler for a System Error 
    // exception from the current EL using the current SP.
    evec curr_el_spx_serror

    // EL0, AArch64

    // The exception handler for a synchronous 
    // exception from a lower EL (AArch64).
    evec lower_el_aarch64_sync 

    // The exception handler for an IRQ from a lower EL (AArch64).
    evec lower_el_aarch64_irq

    // The exception handler for an FIQ from a lower EL (AArch64).
    evec lower_el_aarch64_fiq

    // The exception handler for a System Error 
    // exception from a lower EL(AArch64).
    evec lower_el_aarch64_serror
    
    // EL0, AArch32

    // The exception handler for a synchronous 
    // exception from a lower EL(AArch32).
    evec lower_el_aarch32_sync
    
    // The exception handler for an IRQ exception 
    // from a lower EL (AArch32).
    evec lower_el_aarch32_irq

    // The exception handler for an FIQ exception from 
    // a lower EL (AArch32).
    evec lower_el_aarch32_fiq
    
    // The exception handler for a System Error
    // exception from a lower EL(AArch32).
    evec lower_el_aarch32_serror


curr_el_sp0_sync:
curr_el_sp0_irq:
curr_el_sp0_fiq:
curr_el_sp0_serror:

curr_el_spx_sync:
    // store throwing registers
    mov x0, #4
    bl invalid_exception
    // store kernel registers
curr_el_spx_irq:
curr_el_spx_fiq:
curr_el_spx_serror:

lower_el_aarch64_sync:
lower_el_aarch64_irq:
lower_el_aarch64_fiq:
lower_el_aarch64_serror:

lower_el_aarch32_sync:
lower_el_aarch32_irq:
lower_el_aarch32_fiq:
lower_el_aarch32_serror:
