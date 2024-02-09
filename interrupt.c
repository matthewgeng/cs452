#include "interrupt.h"
#include "assert.h"
#include "rpi.h"

// GICD_ITARGETSR0 to GICD_ITARGETSR7 are read-only, and each field returns a value that corresponds only to the processor reading the register.
void route_irq(int irq, int cpu) {
    KASSERT(cpu < 4, "CPU value must be between 0 and 3");
    // page 107 of Arm Generic Interrupt Controller Architecture Specification GICv2
    int offset = irq/4;
    // GICD_ITARGETRs = 8 bits, 8 bits, 8 bits, readonly 8bits
    // each 8 bits represent 8 potential CPU interfaces
    int bit0 = 1;
    int bit1 = 1 << (8*1+cpu);
    int bit2 = 1 << (8*2+cpu);
    int bit3 = 1 << (8*3+cpu);
    // int bit2 = 0;
    // int bit3 = 0;

    uint32_t* irq_addr = GICD_ITARGETSR + 4*offset;
    uart_printf(CONSOLE, "route val %d\r\n", *irq_addr);

    *irq_addr = *irq_addr | (bit3 | bit2 | bit1 | bit0);
    uart_printf(CONSOLE, "route val after %d\r\n", *irq_addr);
}

void enable_irq(int irq) {
    // page 93 of Arm Generic Interrupt Controller Architecture Specification GICv2
    int offset = irq/32;
    // GICD_ITARGETRs = 8 bits, 8 bits, 8 bits, readonly 8bits
    // each 8 bits represent 8 potential CPU interfaces
    int bit_offset = irq % 32;

    uint32_t* irq_addr = GICD_ISENABLER + 4*offset;
    uart_printf(CONSOLE, "irq val before %d\r\n", *irq_addr);
    *irq_addr = *irq_addr | (1 << bit_offset);
    uart_printf(CONSOLE, "irq val after %d\r\n", *irq_addr);
}

void disable_irq(int irq) {
    int offset = irq/32;
    // GICD_ITARGETRs = 8 bits, 8 bits, 8 bits, readonly 8bits
    // each 8 bits represent 8 potential CPU interfaces
    int bit_offset = irq % 32;
    int mask = ~(1 << bit_offset);

    uint32_t* irq_addr = GICD_ISENABLER + 4*offset;
    *irq_addr = *irq_addr & mask;
}

void enable_irqs() {
    asm volatile (
        "msr daifclr, #2"
    );
    route_irq(SYSTEM_TIMER_IRQ_1, 0);
    enable_irq(SYSTEM_TIMER_IRQ_1);
}

void disable_irqs() {
    disable_irq(SYSTEM_TIMER_IRQ_1);
}
