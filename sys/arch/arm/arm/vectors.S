/*	$NetBSD: vectors.S,v 1.11 2022/10/20 06:58:38 skrll Exp $	*/

/*
 * Copyright (C) 1994-1997 Mark Brinicombe
 * Copyright (C) 1994 Brini
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Brini.
 * 4. The name of Brini may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "assym.h"
#include "opt_cputypes.h"
#include "opt_cpuoptions.h"
#include <machine/asm.h>

/*
 * These are the exception vectors copied down to page 0.
 *
 * Note that FIQs are special; rather than using a level of
 * indirection, we actually copy the FIQ code down into the
 * vector page.
 */

	.text
	.global _C_LABEL(fiqvector)

#if defined(CPU_ARMV7) || defined(CPU_ARM11) || defined(ARM_HAS_VBAR)
	/*
	 * ARMv[67] processors with the Security Extension have the VBAR
	 * which redirects the low vector to any 32-byte aligned address.
	 * Since we are in kernel, we can just do a relative branch to the
	 * exception code and avoid the intermediate load.
	 */
	.global	_C_LABEL(page0rel)
	.p2align 5
_C_LABEL(page0rel):
	b	reset_entry
	b	undefined_entry
	b	swi_entry
	b	prefetch_abort_entry
	b	data_abort_entry
	b	address_exception_entry
	b	irq_entry
#ifdef __ARM_FIQ_INDIRECT
	b	_C_LABEL(fiqvector)
#else
_C_LABEL(fiqbranch):
	subs	pc, lr, #4
#endif
END(page0rel)

#endif /* CPU_ARMV7 || CPU_ARM11 || ARM_HAS_VBAR */

#if !defined(ARM_HAS_VBAR)

	.global	_C_LABEL(page0), _C_LABEL(page0_data), _C_LABEL(page0_end)
	.align	0
_C_LABEL(page0):
	ldr	pc, .Lreset_target
	ldr	pc, .Lundefined_target
	ldr	pc, .Lswi_target
	ldr	pc, .Lprefetch_abort_target
	ldr	pc, .Ldata_abort_target
	ldr	pc, .Laddress_exception_target
	ldr	pc, .Lirq_target
#ifdef __ARM_FIQ_INDIRECT
	ldr	pc, .Lfiq_target
#else
.Lfiqvector:
	.set	_C_LABEL(fiqvector), . - _C_LABEL(page0)
	subs	pc, lr, #4
	.org	.Lfiqvector + 0x100
END(page0)
#endif

_C_LABEL(page0_data):
.Lreset_target:
	.word	reset_entry

.Lundefined_target:
	.word	undefined_entry

.Lswi_target:
	.word	swi_entry

.Lprefetch_abort_target:
	.word	prefetch_abort_entry

.Ldata_abort_target:
	.word	data_abort_entry

.Laddress_exception_target:
	.word	address_exception_entry

.Lirq_target:
	.word	irq_entry

#ifdef __ARM_FIQ_INDIRECT
.Lfiq_target:
	.word	_C_LABEL(fiqvector)
#else
	.word	0	/* pad it out */
#endif
_C_LABEL(page0_end):
#endif /* ARM_HAS_VBAR */

#ifdef __ARM_FIQ_INDIRECT
	.data
	.align	0
_C_LABEL(fiqvector):
	subs	pc, lr, #4
	.org	_C_LABEL(fiqvector) + 0x100
#endif
