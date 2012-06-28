.global swiGetPitchTable
.hidden swiGetPitchTable
.align 2
.thumb_func
swiGetPitchTable:
	swi 0x1B
	bx lr