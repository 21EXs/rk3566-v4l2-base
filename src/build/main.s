	.arch armv8-a
	.file	"main.c"
	.section	.text.startup,"ax",@progbits
	.align	2
	.p2align 3,,7
	.global	main
	.type	main, %function
main:
	stp	x29, x30, [sp, -16]!
	adrp	x0, .LC0
	add	x0, x0, :lo12:.LC0
	add	x29, sp, 0
	bl	printf
	mov	w0, 0
	ldp	x29, x30, [sp], 16
	ret
	.size	main, .-main
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align	3
.LC0:
	.string	"hello world"
	.ident	"GCC: (Linaro GCC 6.3-2017.05) 6.3.1 20170404"
	.section	.note.GNU-stack,"",@progbits
