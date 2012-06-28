.global swiGetVolumeTable
.hidden swiGetVolumeTable
.align 2
.thumb_func
swiGetVolumeTable:
	swi 0x1C
	bx lr