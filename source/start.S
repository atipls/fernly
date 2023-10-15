.section vectors

.global reset
reset:
	ldr	r0, =0x7000affc // stack_start
	mov	sp, r0
	mov	r2, #0xffffffff
	ldr	r1, =0x7000a000 // stack_end

# Clear the stack:
#	cmp	r1, r0
#	str	r2, [r0]
#	sub	r0, r0, #4
#	bcc	clear_stack
	
# Jump to the reset handler:
	ldr	r0, =reset_handler
	mov	pc, r0

asm_memcpy:
	mov	r3, r1
	add	r3, r3, r2

asm_memcpy_loop:
	cmp	r1, r3
	ldrcc	r2, [r1], #4
	strcc	r2, [r0], #4
	bcc	asm_memcpy_loop
	bx	lr

uart_putc:
	ldr	r2, =0xa0080014 // uart offset
uart_putc_ready_wait:
	ldr	r1, [r2]
	tst	r1, #0x20
	beq	uart_putc_ready_wait
	sub	r2, r2, #0x14
	str	r0, [r2]
	bx	lr

.global exception_vectors
exception_vectors:
	ldr	pc, .Lreset_handler
	ldr	pc, .Lundef_handler
	ldr	pc, .Lswi_handler
	ldr	pc, .Lprefetch_abort_handler
	ldr	pc, .Ldata_abort_handler
	ldr	pc, .Lreserved_handler
	ldr	pc, .Lirq_handler
	ldr	pc, .Lfiq_handler

.Lreset_handler:
	.long	reset_handler
.Lundef_handler:
	.long	undef_handler
.Lswi_handler:
	.long	swi_handler
.Lprefetch_abort_handler:
	.long	prefetch_abort_handler
.Ldata_abort_handler:
	.long	data_abort_handler
.Lreserved_handler:
	.long	reserved_handler
.Lirq_handler:
	.long	irq_handler
.Lfiq_handler:
	.long	fiq_handler

.global exception_vectors_end
exception_vectors_end:

.text


.global ram_memcpy
ram_memcpy: 
	ldr	r0, =ram_memcpy_addr
	ldr	r0, [r0]
	mov	pc, r0
ram_memcpy_addr:
        .long	0x70007388

.global ram_bzero
ram_bzero:
	ldr	r0, =ram_bzero_addr
	ldr	r0, [r0]
	mov	pc, r0
ram_bzero_addr:
        .long	0x700073bc
