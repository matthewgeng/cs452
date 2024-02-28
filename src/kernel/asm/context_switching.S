.macro save_task
    ldr x11, =currentTaskFrame
    ldr x11, [x11]
    str x0, [x11, #0]
    str x1, [x11, #8]
    str x2, [x11, #16]
    str x3, [x11, #24]
    str x4, [x11, #32]
    str x5, [x11, #40]
    str x6, [x11, #48]
    str x7, [x11, #56]
    str x8, [x11, #64]
    str x9, [x11, #72]
    str x10, [x11, #80]
    str x12, [x11, #96]
    str x13, [x11, #104]
    str x14, [x11, #112]
    str x15, [x11, #120]
    str x16, [x11, #128]
    str x17, [x11, #136]
    str x18, [x11, #144]
    str x19, [x11, #152]
    str x20, [x11, #160]
    str x21, [x11, #168]
    str x22, [x11, #176]
    str x23, [x11, #184]
    str x24, [x11, #192]
    str x25, [x11, #200]
    str x26, [x11, #208]
    str x27, [x11, #216]
    str x28, [x11, #224]
    str x29, [x11, #232]
    str x30, [x11, #240]
    mrs x12, sp_el0
    str x12, [x11, #248]
    mrs x12, elr_el1
    str x12, [x11, #256]
    mrs x12, spsr_el1
    str x12, [x11, #264]

.endm

.macro load_task
    ldr x11, =currentTaskFrame
    ldr x11, [x11]
    ldr x0, [x11, #0]
    ldr x1, [x11, #8]
    ldr x2, [x11, #16]
    ldr x3, [x11, #24]
    ldr x4, [x11, #32]
    ldr x5, [x11, #40]
    ldr x6, [x11, #48]
    ldr x7, [x11, #56]
    ldr x8, [x11, #64]
    ldr x9, [x11, #72]
    ldr x10, [x11, #80]
    ldr x13, [x11, #104]
    ldr x14, [x11, #112]
    ldr x15, [x11, #120]
    ldr x16, [x11, #128]
    ldr x17, [x11, #136]
    ldr x18, [x11, #144]
    ldr x19, [x11, #152]
    ldr x20, [x11, #160]
    ldr x21, [x11, #168]
    ldr x22, [x11, #176]
    ldr x23, [x11, #184]
    ldr x24, [x11, #192]
    ldr x25, [x11, #200]
    ldr x26, [x11, #208]
    ldr x27, [x11, #216]
    ldr x28, [x11, #224]
    ldr x29, [x11, #232]
    ldr x30, [x11, #240]

    ldr x12, [x11, #256]
    msr elr_el1, x12
    ldr x12, [x11, #248]
    msr sp_el0, x12
    ldr x12, [x11, #264]
    msr spsr_el1, x12

    mov lr, x30
.endm

.macro save_kernel
    ldr x11, =kf
    ldr x11, [x11]
    str x0, [x11, #0]
    str x1, [x11, #8]
    str x2, [x11, #16]
    str x3, [x11, #24]
    str x4, [x11, #32]
    str x5, [x11, #40]
    str x6, [x11, #48]
    str x7, [x11, #56]
    str x8, [x11, #64]
    str x9, [x11, #72]
    str x10, [x11, #80]
    str x12, [x11, #96]
    str x13, [x11, #104]
    str x14, [x11, #112]
    str x15, [x11, #120]
    str x16, [x11, #128]
    str x17, [x11, #136]
    str x18, [x11, #144]
    str x19, [x11, #152]
    str x20, [x11, #160]
    str x21, [x11, #168]
    str x22, [x11, #176]
    str x23, [x11, #184]
    str x24, [x11, #192]
    str x25, [x11, #200]
    str x26, [x11, #208]
    str x27, [x11, #216]
    str x28, [x11, #224]
    str x29, [x11, #232]
    str x30, [x11, #240]
    mov x12, sp
    str x12, [x11, #248]
.endm

.macro load_kernel
    ldr x11, =kf
    ldr x11, [x11]

    ldr x0, [x11, #0]
    ldr x1, [x11, #8]
    ldr x2, [x11, #16]
    ldr x3, [x11, #24]
    ldr x4, [x11, #32]
    ldr x5, [x11, #40]
    ldr x6, [x11, #48]
    ldr x7, [x11, #56]
    ldr x8, [x11, #64]
    ldr x9, [x11, #72]
    ldr x10, [x11, #80]
    ldr x13, [x11, #104]
    ldr x14, [x11, #112]
    ldr x15, [x11, #120]
    ldr x16, [x11, #128]
    ldr x17, [x11, #136]
    ldr x18, [x11, #144]
    ldr x19, [x11, #152]
    ldr x20, [x11, #160]
    ldr x21, [x11, #168]
    ldr x22, [x11, #176]
    ldr x23, [x11, #184]
    ldr x24, [x11, #192]
    ldr x25, [x11, #200]
    ldr x26, [x11, #208]
    ldr x27, [x11, #216]
    ldr x28, [x11, #224]
    ldr x29, [x11, #232]
    ldr x30, [x11, #240]

    ldr x0, [x11, #248]
    mov sp, x0
.endm

.global context_switch_to_task
context_switch_to_task:
    save_kernel
    load_task
    eret

.global return_to_kernel_syscall
return_to_kernel_syscall:
    save_task
    load_kernel
    // return value from context_switch_to_task, 0 --> SYSCALL
    mov x0, 0
    ret

.global return_to_kernel_irq
return_to_kernel_irq:
    save_task
    load_kernel
    // return value from context_switch_to_task, 1 --> IRQ
    mov x0, 1
    ret
