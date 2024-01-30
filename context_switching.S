.global context_switch_to_task
context_switch_to_task:
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
    mov x0, sp
    str x0, [x11, #248]

    ldr x11, =currentTaskFrame
    ldr x11, [x11]
    ldr x0, [x11, #8]
    mov x1, x0
    ldr x0, [x11, #16]
    mov x2, x0
    ldr x0, [x11, #24]
    mov x3, x0
    ldr x0, [x11, #32]
    mov x4, x0
    ldr x0, [x11, #40]
    mov x5, x0
    ldr x0, [x11, #48]
    mov x6, x0
    ldr x0, [x11, #56]
    mov x7, x0
    ldr x0, [x11, #64]
    mov x8, x0
    ldr x0, [x11, #80]
    mov x10, x0
    ldr x0, [x11, #96]
    mov x12, x0
    ldr x0, [x11, #104]
    mov x13, x0
    ldr x0, [x11, #112]
    mov x14, x0
    ldr x0, [x11, #120]
    mov x15, x0
    ldr x0, [x11, #128]
    mov x16, x0
    ldr x0, [x11, #136]
    mov x17, x0
    ldr x0, [x11, #144]
    mov x18, x0
    ldr x0, [x11, #152]
    mov x19, x0
    ldr x0, [x11, #160]
    mov x20, x0
    ldr x0, [x11, #168]
    mov x21, x0
    ldr x0, [x11, #176]
    mov x22, x0
    ldr x0, [x11, #184]
    mov x23, x0
    ldr x0, [x11, #192]
    mov x24, x0
    ldr x0, [x11, #200]
    mov x25, x0
    ldr x0, [x11, #208]
    mov x26, x0
    ldr x0, [x11, #216]
    mov x27, x0
    ldr x0, [x11, #224]
    mov x28, x0
    ldr x0, [x11, #232]
    mov x29, x0
    ldr x0, [x11, #240]
    mov x30, x0

    ldr x0, [x11, #256]
    msr elr_el1, x0
    ldr x0, [x11, #248]
    msr sp_el0, x0
    ldr x0, [x11, #264]
    msr spsr_el1, x0
    
    ldr x0, [x11, #0]
    mov x0, x0

    mov lr, x30
    eret


.global return_to_kernel
return_to_kernel:
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
    mrs x0, sp_el0
    str x0, [x11, #248]
    mrs x0, elr_el1
    str x0, [x11, #256]
    mrs x0, spsr_el1
    str x0, [x11, #264]

    ldr x11, =kf
    ldr x11, [x11]
    
    ldr x0, [x11, #8]
    mov x1, x0
    ldr x0, [x11, #16]
    mov x2, x0
    ldr x0, [x11, #24]
    mov x3, x0
    ldr x0, [x11, #32]
    mov x4, x0
    ldr x0, [x11, #40]
    mov x5, x0
    ldr x0, [x11, #48]
    mov x6, x0
    ldr x0, [x11, #56]
    mov x7, x0
    ldr x0, [x11, #64]
    mov x8, x0

    ldr x11, =currentTaskFrame
    ldr x11, [x11]
    ldr x0, [x11, #72]
    mov x9, x0
    ldr x0, [x11, #80]
    mov x10, x0

    ldr x11, =kf
    ldr x11, [x11]
    ldr x0, [x11, #96]
    mov x12, x0
    ldr x0, [x11, #104]
    mov x13, x0
    ldr x0, [x11, #112]
    mov x14, x0
    ldr x0, [x11, #120]
    mov x15, x0
    ldr x0, [x11, #128]
    mov x16, x0
    ldr x0, [x11, #136]
    mov x17, x0
    ldr x0, [x11, #144]
    mov x18, x0
    ldr x0, [x11, #152]
    mov x19, x0
    ldr x0, [x11, #160]
    mov x20, x0
    ldr x0, [x11, #168]
    mov x21, x0
    ldr x0, [x11, #176]
    mov x22, x0
    ldr x0, [x11, #184]
    mov x23, x0
    ldr x0, [x11, #192]
    mov x24, x0
    ldr x0, [x11, #200]
    mov x25, x0
    ldr x0, [x11, #208]
    mov x26, x0
    ldr x0, [x11, #216]
    mov x27, x0
    ldr x0, [x11, #224]
    mov x28, x0
    ldr x0, [x11, #232]
    mov x29, x0
    ldr x0, [x11, #240]
    mov x30, x0

    ldr x0, [x11, #248]
    mov sp, x0

    ldr x0, [x11, #0]
    mov x0, x0

    ret
