	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 12, 0	sdk_version 12, 3
	.globl	_f64                            ; -- Begin function f64
	.p2align	2
_f64:                                   ; @f64
; %bb.0:
    ; スタック伸長
	sub	sp, sp, #32
	; fp, lr を積む(上16バイト)
	stp	x29, x30, [sp, #16]             ; 16-byte Folded Spill
	; fp <- fp, lr退避場所
	add	x29, sp, #16
	; x0 <- SP+8 (&a)
	add	x0, sp, #8
	mov	x8, #1
	; a = 1
	str	x8, [sp, #8]
	; x1 <- SP (&b)
	mov	x1, sp
	mov	x8, #2
	; b = 2
	str	x8, [sp]
	bl	_add
	ldp	x29, x30, [sp, #16]             ; 16-byte Folded Reload
	add	sp, sp, #32
	ret
                                        ; -- End function
	.globl	_f32                            ; -- Begin function f32
	.p2align	2
_f32:                                   ; @f32
; %bb.0:
    ; スタック伸長
	sub	sp, sp, #32
	; fp, lr退避
	stp	x29, x30, [sp, #16]             ; 16-byte Folded Spill
	; fp <- fp, lr退避場所
	add	x29, sp, #16
	; x0 <- fp - 4 (&a)
	sub	x0, x29, #4
	mov	w8, #1
	; a = 1
	stur	w8, [x29, #-4]
	; x1 <- sp + 8 (&b)
	add	x1, sp, #8
	mov	w8, #2
	str	w8, [sp, #8]
	bl	_add
	ldp	x29, x30, [sp, #16]             ; 16-byte Folded Reload
	add	sp, sp, #32
	ret
                                        ; -- End function
.subsections_via_symbols
