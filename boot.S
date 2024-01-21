/*
What does the boot.S do?

    boot.S is the start of our program which is defined through linker.ld
    (linker.ld is a linker script that defines the output binary data layout, ensures 
    that we can specify that entrypoint of our binary)

What are the steps in this file?

    1. defines macros that represent specific values for intialization
    2. defines the section mentioned in linker.ld and the entrypoint
        .section ".text.boot"
        .global _start
        _start: 
    3. Checks if we are current in EL1 (exception level 1), if not we are in EL2 (default)

        Note:
            OS doesn't have to be in EL1, but EL1 is a natural choice because 
            this level has just the right set of privileges to implement all common OS tasks

            from: https://forums.raspberrypi.com/viewtopic.php?t=268116
            VMWare like programs would run EL2 and things like ARM trustzone O/S would run in EL3. 

            The standard bootstub if you are using it will do a minimal EL3 setup and drop to EL2. 
            It then parks core 1,2,3 in EL2 and only core 0 continues on to arrive to you in EL2. 
            Usually you will do a minimal setup in EL2 and drop into EL1 because your baremetal program is sort of acting like an O/S.

            The reason we generally don't stay up at EL2 or EL3 is because things like interrupts 
            have to be routed up to those levels. The sorts of programs that run EL2 and EL3 don't 
            deal with interrupts normally so by default they aren't routed up to those levels. 
            So you have the complication of working out how to route the interrupts etc up to higher levels. 
            As we said there is no advantage running higher and you just made work for yourself. 
            Worse if you actually did want to make an O/S that isn't what you want to do because of 
            the security issue and so it's better to learn on the right level.
        
        - If we are: go to next step
        - else:
    4. Execute main setup once we are in exception level 1 

Resources:
https://www.rpi4os.com/part1-bootstrapping/
https://s-matyukevich.github.io/raspberry-pi-os/docs/lesson01/rpi-os.html
*/

// ***************************************
// SCTLR_EL1, System Control Register (EL1)
// Architecture Reference Manual Section D17.2.118
// ***************************************
#define SCTLR_RESERVED (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)
#define USER_MASK_ACCESS (1 << 9)
#define SCTLR_WFE_WFI_ENABLED (1 << 18) | (1 << 16)
#define SCTLR_VALUE_MMU_DISABLED (SCTLR_RESERVED | USER_MASK_ACCESS | SCTLR_WFE_WFI_ENABLED)

// ***************************************
// HCR_EL2, Hypervisor Configuration Register (EL2)
// Architecture Reference Manual Section D17.2.48
// Controls some aspects of EL1 behavour, bit 31 --> controls EL1 AArch64 vs AArch32
// ***************************************
#define HCR_RW (1 << 31)

// ***************************************
// SPSR_EL2, Saved Program Status Register (EL2)
// Architecture Reference Manual Section C5.2.19
// SPSRs are 32 bit registers that store the process state of the current exception level when an exception 
// is taken to an exception level that uses AArch64 state, allows process state to be restored after exception handled
// ***************************************

#define SPSR_MASK_ALL (11 << 6) // SError interrupt and IRQ interrupt mask 
#define SPSR_EL1       (5 << 0) // sets EL1h stack pointer selection
#define SPSR_VALUE (SPSR_MASK_ALL | SPSR_EL1)

// ensure the linker puts this at the start of the kernel image
.section .text.boot
.global start
start:
    // check processor ID is zero (executing on main core), else loop
//    mrs  x0, mpidr_el1
//    and  x0, x0, #3
//    cbnz x0, exit

    // are we already in EL1?
    mrs  x9, CurrentEL
    and  x9, x9, #8
    cbz  x9, el1_entry

    // otherwise, switch from EL2 to EL1 using a fake exception
    // initial bootstub switches us from EL3 to El2 https://github.com/raspberrypi/tools/blob/master/armstubs/armstub8.S#L143

    // SETS EL1 EXECUTION STATE TO BE AArch64 
    /* (we don't use EL2, but this system register
    controls EL1 execution state AArch64 vs AArch32, 
    https://developer.arm.com/documentation/ddi0601/2023-12/AArch64-Registers/HCR-EL2--Hypervisor-Configuration-Register) */
    ldr x10, =HCR_RW
    msr hcr_el2, x10

    // sets SPSSR_EL2 some to use the EL1h stack pointer
    // https://developer.arm.com/documentation/101548/0002/AArch64-registers/AArch64-register-descriptions/AArch64-Special-purpose-register-description/SPSR-EL2--Saved-Program-Status-Register--EL2-
    ldr x11, =SPSR_VALUE
    msr spsr_el2, x11

    // loads e1_entry addr into x12, and elr_el2 register jumps to addr when exception
    adr x12, el1_entry
    msr elr_el2, x12

    eret // -> el1_entry

el1_entry:
    // configure processor and mmu
    ldr x13, =SCTLR_VALUE_MMU_DISABLED
    msr sctlr_el1, x13

    // initialize SP
    msr SPSel, #1
    ldr x14, =stackend
    mov sp, x14
    // Jump to our main() routine in C/C++
    bl  kmain

    // use watchdog to reboot
    mov x2, 36
    mov x0, 28
    movk  x2, 0xfe10, lsl 16
    movk  x0, 0xfe10, lsl 16
    mov w3, 1
    mov w1, 32
    movk  w3, 0x5a00, lsl 16
    str w3, [x2]
    movk  w1, 0x5a00, lsl 16
    str w1, [x0]

exit:
    // shouldn't be reached
    wfi
    b  exit

.section .bss
.balign 16
stack:
    .rept 0x10000
    .byte 0
    .endr
stackend:
