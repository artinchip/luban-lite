/*
 * Copyright (C) 2020 ArtInChip Technology Co.,Ltd
 * Author: Dehuang Wu <dehuang.wu@artinchip.com>
 */

#define LREG			lw
#define SREG			sw
#define REGBYTES		4

.extern	boot_params_stash

.global save_boot_params
.type   save_boot_params, %function
save_boot_params:
	la	t0, boot_params_stash
	SREG	a0, REGBYTES * 0(t0)
	SREG	a1, REGBYTES * 1(t0)
	SREG	a2, REGBYTES * 2(t0)
	SREG	a3, REGBYTES * 3(t0)
	SREG	a4, REGBYTES * 4(t0)
	SREG	a5, REGBYTES * 5(t0)
	SREG	a6, REGBYTES * 6(t0)
	SREG	a7, REGBYTES * 7(t0)
	SREG	s0, REGBYTES * 8(t0)
	SREG	s1, REGBYTES * 9(t0)
	SREG	s2, REGBYTES * 10(t0)
	SREG	s3, REGBYTES * 11(t0)
	SREG	s4, REGBYTES * 12(t0)
	SREG	s5, REGBYTES * 13(t0)
	SREG	s6, REGBYTES * 14(t0)
	SREG	s7, REGBYTES * 15(t0)
	SREG	s8, REGBYTES * 16(t0)
	SREG	s9, REGBYTES * 17(t0)
	SREG	s10, REGBYTES * 18(t0)
	SREG	s11, REGBYTES * 19(t0)
	SREG	sp, REGBYTES * 20(t0)
	SREG	ra, REGBYTES * 21(t0)
	j	save_boot_params_ret

