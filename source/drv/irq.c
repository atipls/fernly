#include "irq.h"
#include "memio.h"
#include "printf.h"
#include "serial.h"
#include <stdint.h>

static struct {
    void (*handler)(enum irq_number irq_num, void *opaque);
    void *opaque;
} handlers[__irq_max__];

extern uint32_t exception_vectors[];

typedef union {
    struct
    {
        uint32_t M : 1;          /*!< bit:     0  MMU enable */
        uint32_t A : 1;          /*!< bit:     1  Alignment check enable */
        uint32_t C : 1;          /*!< bit:     2  Cache enable */
        uint32_t _reserved0 : 2; /*!< bit: 3.. 4  Reserved */
        uint32_t CP15BEN : 1;    /*!< bit:     5  CP15 barrier enable */
        uint32_t _reserved1 : 1; /*!< bit:     6  Reserved */
        uint32_t B : 1;          /*!< bit:     7  Endianness model */
        uint32_t _reserved2 : 2; /*!< bit: 8.. 9  Reserved */
        uint32_t SW : 1;         /*!< bit:    10  SWP and SWPB enable */
        uint32_t Z : 1;          /*!< bit:    11  Branch prediction enable */
        uint32_t I : 1;          /*!< bit:    12  Instruction cache enable */
        uint32_t V : 1;          /*!< bit:    13  Vectors bit */
        uint32_t RR : 1;         /*!< bit:    14  Round Robin select */
        uint32_t _reserved3 : 2; /*!< bit:15..16  Reserved */
        uint32_t HA : 1;         /*!< bit:    17  Hardware Access flag enable */
        uint32_t _reserved4 : 1; /*!< bit:    18  Reserved */
        uint32_t WXN : 1;        /*!< bit:    19  Write permission implies XN */
        uint32_t UWXN : 1;       /*!< bit:    20  Unprivileged write permission implies PL1 XN */
        uint32_t FI : 1;         /*!< bit:    21  Fast interrupts configuration enable */
        uint32_t U : 1;          /*!< bit:    22  Alignment model */
        uint32_t _reserved5 : 1; /*!< bit:    23  Reserved */
        uint32_t VE : 1;         /*!< bit:    24  Interrupt Vectors Enable */
        uint32_t EE : 1;         /*!< bit:    25  Exception Endianness */
        uint32_t _reserved6 : 1; /*!< bit:    26  Reserved */
        uint32_t NMFI : 1;       /*!< bit:    27  Non-maskable FIQ (NMFI) support */
        uint32_t TRE : 1;        /*!< bit:    28  TEX remap enable. */
        uint32_t AFE : 1;        /*!< bit:    29  Access flag enable */
        uint32_t TE : 1;         /*!< bit:    30  Thumb Exception enable */
        uint32_t _reserved7 : 1; /*!< bit:    31  Reserved */
    } b;                         /*!< Structure used for bit  access */
    uint32_t w;                  /*!< Type      used for word access */
} SCTLR_Type;

typedef union {
    struct
    {
        uint32_t M : 5;
        uint32_t T : 1;
        uint32_t F : 1;
        uint32_t I : 1;
        uint32_t A : 1;
        uint32_t E : 1;
        uint32_t IT1 : 6;
        uint32_t GE : 4;
        uint32_t _reserved0 : 4;
        uint32_t J : 1;
        uint32_t IT0 : 2;
        uint32_t Q : 1;
        uint32_t V : 1;
        uint32_t C : 1;
        uint32_t Z : 1;
        uint32_t N : 1;
    } b;
    uint32_t w;
} CPSR_Type;


int irq_init(void) {
    register int var;

    CPSR_Type cpsr;
    asm volatile("mrs %0, cpsr"
                 : "=r"(cpsr.w));
    printf("CPSR: M: %d T: %d F: %d I: %d A: %d E: %d IT1: %d GE: %d J: %d IT0: %d Q: %d V: %d C: %d Z: %d N: %d\n",
           cpsr.b.M, cpsr.b.T, cpsr.b.F, cpsr.b.I, cpsr.b.A, cpsr.b.E, cpsr.b.IT1, cpsr.b.GE, cpsr.b.J, cpsr.b.IT0, cpsr.b.Q, cpsr.b.V, cpsr.b.C, cpsr.b.Z, cpsr.b.N);

    volatile uint32_t *vectors = (uint32_t *) 0x00000000;

    serial_puts("Our exception vectors:\n");
    serial_print_hex(exception_vectors, 8 * sizeof(uint32_t));

    serial_puts("Original exception vectors:\n");
    serial_print_hex((const void *) vectors, 8 * sizeof(uint32_t));

    // Copy exception vectors to 0x00000000
    for (int i = 0; i < 8; i++)
        vectors[i] = exception_vectors[i];

    serial_puts("Copied exception vectors:\n");
    serial_print_hex((const void *) vectors, 8 * sizeof(uint32_t));

    /* Acknowledge all interrupts */
    writel(0xffffffff, IRQ_BASE + IRQ_MASK_OFF + IRQ_NUM_ADJ(0));
    writel(0xffffffff, IRQ_BASE + IRQ_MASK_OFF + IRQ_NUM_ADJ(32));

    asm volatile("mrs %0, cpsr"
                 : "=r"(var));
    if (!(var & 0x80)) {
        serial_puts("Interrupts already enabled\n");
        return -1;
    }

    serial_puts("Interrupts were disabled.  Re-enabling...\n");
    var &= ~0x80;
    var |= 0x40;
    var &= ~0x1f;
    var |= 0x10;
    asm volatile("msr cpsr, %0"
                 : "=r"(var));

    serial_puts("Interrupts enabled\n");

    return 0;
}

int fiq_init(void) {
    serial_puts("FIQs compiled out\n");
    return -1;
    /*
  register int var;
  asm volatile ("mrs %0, cpsr":"=r" (var));
  if (!(var & 0x40)) {
          serial_puts("FIQ already enabled\n");
          return -1;
  }

  serial_puts("FIQ was disabled.  Re-enabling...\n");
  var &= ~0x40;
  asm volatile ("msr cpsr, %0":"=r" (var));

  return 0;
  */
}

int irq_enable(enum irq_number irq_num) {
    uint32_t reg = IRQ_BASE + IRQ_MASK_OFF + IRQ_CLR + IRQ_NUM_ADJ(irq_num);
    if (irq_num >= __irq_max__)
        return -1;

    writel(1 << (irq_num & 31), reg);
    return 0;
}

int irq_disable(enum irq_number irq_num) {
    uint32_t reg = IRQ_BASE + IRQ_MASK_OFF + IRQ_SET + IRQ_NUM_ADJ(irq_num);

    if (irq_num >= __irq_max__)
        return -1;

    writel(1 << (irq_num & 31), reg);
    return 0;
}

void irq_stimulate(enum irq_number irq_num) {
    uint32_t reg = IRQ_BASE + IRQ_STIM_OFF + IRQ_SET + IRQ_NUM_ADJ(irq_num);
    writel(1 << (irq_num & 31), reg);
}

void irq_stimulate_reset(enum irq_number irq_num) {
    uint32_t reg = IRQ_BASE + IRQ_STIM_OFF + IRQ_CLR + IRQ_NUM_ADJ(irq_num);
    writel(1 << (irq_num & 31), reg);
}

void irq_acknowledge(enum irq_number irq_num) {
    uint32_t reg = IRQ_BASE + IRQ_ACK_OFF + IRQ_NUM_ADJ(irq_num);

    if (irq_num >= __irq_max__)
        return;

    writel(1 << (irq_num & 31), reg);
    return;
}

void irq_mask_acknowledge(enum irq_number irq_num) {
    irq_disable(irq_num);
    irq_acknowledge(irq_num);
}

void irq_register_handler(enum irq_number irq_num,
                          void (*handler)(enum irq_number irq_num,
                                          void *opaque),
                          void *opaque) {
    if (irq_num >= __irq_max__)
        return;
    handlers[irq_num].handler = handler;
    handlers[irq_num].opaque = opaque;
}

static void irq_dispatch_one(enum irq_number irq_num) {
    if (handlers[irq_num].handler)
        handlers[irq_num].handler(irq_num, handlers[irq_num].opaque);
    else
        printf("Unhandled IRQ: %d\n", irq_num);
    irq_acknowledge(irq_num);
}

void irq_dispatch(void) {
    uint32_t reg;
    uint32_t val;
    int i;

    printf("Dispatching IRQs...\n");
    reg = IRQ_BASE + IRQ_STATUS_OFF;
    val = readl(reg);
    printf("Lower Mask: 0x%08X\n", val);

    for (i = 0; i < 32; i++)
        if (val & (1 << i))
            irq_dispatch_one(i);

    reg += IRQ_BASE + IRQ_STATUS_OFF + 4;
    val = readl(reg);
    printf("Upper Mask: 0x%08X\n", val);
    for (i = 0; i < (__irq_max__ - 32); i++)
        if (val & (1 << i))
            irq_dispatch_one(32 + i);

    printf("Done dispatch\n");
}
