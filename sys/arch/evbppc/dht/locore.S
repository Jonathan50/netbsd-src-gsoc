/*	$NetBSD: locore.S,v 1.3 2022/05/07 04:12:54 rin Exp $	*/

/*
 * Taken from src/sys/arch/powerpc/ibm4xx/openbios/locore.s:
 *	NetBSD: locore.S,v 1.17 2021/03/30 01:57:20 rin Exp
 */

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef PPC_4XX_NOCACHE
#error Not tested.
#endif

#include "assym.h"
#include "ksyms.h"

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_modular.h"
#include "opt_ppcparam.h"
#endif

#include <machine/param.h>
#include <machine/asm.h>
#include <machine/psl.h>
#include <machine/trap.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>

#include <powerpc/ibm4xx/dcr4xx.h>

/*
 * This symbol is here for the benefit of kvm_mkdb, and is supposed to
 * mark the start of kernel text.
 */
	.text
	.globl	_C_LABEL(kernel_text)
_C_LABEL(kernel_text):

/*
 * Startup entry.  Note, this must be the first thing in the text
 * segment!
 */
	.text
	.globl	__start
__start:
	li	%r0,0
	mtmsr	%r0			/* Disable FPU/MMU/exceptions */
	isync

	/*
	 * Errata 213:  Incorrect data may be flushed from the data cache.
	 * Cores:       PPC405D5X1, PPC405D5X2
	 * Workaround:  #1, CCR0 modification sequence #2
	 * Note:        Meaning of bits we need to set is undocumented.
	 */
	sync
	mfspr   %r0,SPR_CCR0	/* XXXclang: mfccr0 %r0 */
	oris    %r0,%r0,0x50000000@h
	mtspr   SPR_CCR0,%r0	/* XXXclang: mtccr0 %r0 */
	isync

	/* PPC405GP errata, item #58.
	 * Load string instructions may write incorrect data into the last GPR
	 * targeted in the operation.
	 * Workaround: set OCM0_DSCNTL[DSEN]=0 and OCM0_DSCNTL[DOF]=0 */
	li %r0,0
	mtdcr	DCR_OCM0_DSCNTL, %r0  	/* Disable Data access to OCM */
	mtdcr	DCR_OCM0_ISCNTL, %r0  	/* Disable Instruction access to OCM.
					   Just in case */
#ifdef PPC_4XX_NOCACHE
	/* Disable all caches for physical addresses */
	li	%r0,0
#else
	/* Allow cacheing for only the first 2GB of RAM */
	lis	%r0,0xffff
#endif
	mtdccr	%r0
	mticcr	%r0

	/* Invalidate all TLB entries */
	tlbia
	sync
	isync

	/* Get start of BSS */
	lis	%r3,_C_LABEL(_edata)-4@ha
	addi	%r3,%r3,_C_LABEL(_edata)-4@l
	/* Get end of kernel memory */
	lis	%r8,_C_LABEL(end)@ha
	addi	%r8,%r8,_C_LABEL(end)@l
	/* Zero BSS */
	li	%r4,0
2:	stwu	%r4,4(%r3)
	cmpw	%r3,%r8
	bne+	2b

#if NKSYMS > 0 || defined(DDB) || defined(MODULAR)
	/*
	 * If we had symbol table location we'd store it here and would've
	 * adjusted r8 here.
	 */
	lis	%r7,_C_LABEL(startsym)@ha
	addi	%r7,%r7,_C_LABEL(startsym)@l
	stw	%r8,0(%r7)
	lis	%r7,_C_LABEL(endsym)@ha
	addi	%r7,%r7,_C_LABEL(endsym)@l
	stw	%r8,0(%r7)
#endif

	/* Set kernel MMU context. */
	li	%r0,KERNEL_PID
	mtpid	%r0
	sync

	INIT_CPUINFO(%r8,%r1,%r9,%r0)
	mr	%r4,%r8

	lis	%r3,__start@ha
	addi	%r3,%r3,__start@l

	mr	%r6,%r31		/* info_block address */
	bl	_C_LABEL(initppc)
	bl	_C_LABEL(main)

loop:	b	loop			/* XXX not reached */

#include <powerpc/ibm4xx/4xx_locore.S>
