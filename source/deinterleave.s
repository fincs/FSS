@ Taken from Tomwi's soundStream

@.equ BUF_BYTES, 8192*2
.global deinterleave16
.hidden deinterleave16
.type deinterleave16, %function

.align 2

deinterleave16: @ r0=inBuf r1=leftBuf r2=bytes r3=rightBuf
	@ Commented out because it's already ensured by caller
	@cmp r2, #4
	@bxlt lr   @ nothing to split
	@bic r2, r2, #3

	@ Convert bytes to sample pair count
	lsr r2, #2

	push {r4-r9}

	mvn r8, #0
	lsl r8, #16

.Ld16_loop:
	ldmia r0!,{r4-r7}

	@ split left
	bic r9, r4, r8
	orr r9, r9, r5, lsl #16

	@ split left
	bic ip, r6, r8
	orr ip, ip, r7, lsl #16

	@ store left
	stmia r1!,{r9,ip}

	@ split right
	and r9, r5, r8
	orr r9, r9, r4, lsr #16

	@ split right
	and ip, r7, r8
	orr ip, ip, r6, lsr #16

	@ store right
	stmia r3!,{r9,ip}

	subs r2,#4
	bne .Ld16_loop

	pop {r4-r9}
	bx lr

.global deinterleave8
.hidden deinterleave8
.type deinterleave8, %function

.align 2

deinterleave8: @ r0=inBuf r1=leftBuf r2=bytes r3=rightBuf
	@ Commented out because it's already ensured by caller
	@cmp r2, #4
	@bxlt lr   @ nothing to split
	@bic r2, r2, #3

	@ Convert bytes to sample pair count
	lsr r2, #1

	push {r4-r9}

	mov r12, #0xFF
	mov r9, r12, lsl #24

.Ld8_loop:
	ldmia r0!, {r4, r5}

	@ left channel
	and r6, ip, r4, lsr #16
	orr r6, r6, r4, lsl #24

	and r7, ip, r5, lsr #16
	orr r7, r7, r5, lsl #24

	ror r7, r7, #8
	orr r7, r7, r6, ror #24

	str r7, [r1], #4

	@ right channel
	and r6, r9, r4, lsl #16
	orr r6, r6, r4, lsr #24

	and r7, r9, r5, lsl #16
	orr r7, r7, r5, lsr #24

	ror r7, r7, #8
	orr r7, r7, r6, ror #24

	str r7, [r3], #4

	subs r2, #4
	bne .Ld8_loop

	pop {r4-r9}
	bx lr
