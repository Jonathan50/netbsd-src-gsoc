/*	$NetBSD: cpufunc_asm_arm10.S,v 1.12 2022/10/20 06:58:38 skrll Exp $	*/

/*
 * Copyright (c) 2002 ARM Limited
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ARM10 assembly functions for CPU / MMU / TLB specific operations
 */

#include <machine/asm.h>
#include <arm/locore.h>
#include "assym.h"

/*
 * TLB functions
 */
ENTRY(arm10_tlb_flushID_SE)
	mcr	p15, 0, r0, c8, c6, 1	/* flush D tlb single entry */
	mcr	p15, 0, r0, c8, c5, 1	/* flush I tlb single entry */
#if PAGE_SIZE == 2 * L2_S_SIZE
	add	r0, r0, #L2_S_SIZE
	mcr	p15, 0, r0, c8, c6, 1	/* flush D tlb single entry */
	mcr	p15, 0, r0, c8, c5, 1	/* flush I tlb single entry */
#endif
	RET
END(arm10_tlb_flushID_SE)

ENTRY(arm10_tlb_flushI_SE)
	mcr	p15, 0, r0, c8, c5, 1	/* flush I tlb single entry */
#if PAGE_SIZE == 2 * L2_S_SIZE
	add	r0, r0, #L2_S_SIZE
	mcr	p15, 0, r0, c8, c5, 1	/* flush I tlb single entry */
#endif
	RET
END(arm10_tlb_flushI_SE)


/*
 * Context switch.
 *
 * These are the CPU-specific parts of the context switcher cpu_switch()
 * These functions actually perform the TTB reload.
 */
ENTRY(arm10_context_switch)
	/*
	 * We can assume that the caches will only contain kernel addresses
	 * at this point.  So no need to flush them again.
	 */
	mcr	p15, 0, r0, c7, c10, 4	/* drain the write buffer */
	mcr	p15, 0, r0, c2, c0, 0	/* set the new TTB */
	mcr	p15, 0, r0, c8, c7, 0	/* and flush the I+D tlbs */

	/* Paranoia -- make sure the pipeline is empty. */
	nop
	nop
	nop
	RET
END(arm10_context_switch)
