#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>

#define GIC_BASE 0xFF840000
#define GICD_BASE (GIC_BASE+0x00001000)
#define GICC_BASE (GIC_BASE+0x00002000)

#define GICD_ISENABLER (GICD_BASE+0x00000100)

#define GICC_IAR (GICC_BASE+0x0000000C)
#define GICC_EOIR (GICC_BASE+0x00000010)

#define GICD_ITARGETSR (GICD_BASE+0x00000800)

// page 89 of bcm2711-periperals  
//VC (=VideoCore) starts at 96
#define SYSTEM_TIMER_IRQ_0 0x60 //96, used by GPU
#define SYSTEM_TIMER_IRQ_1 0x61 //97
#define SYSTEM_TIMER_IRQ_2 0x62 //98, used by GPU
#define SYSTEM_TIMER_IRQ_3 0x63 //99

// UART
#define UART_IRQ 153

void enable_irq(int irq);
void disable_irq(int irq);
void enable_irqs();
void disable_irqs();

#endif
