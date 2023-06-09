/*	$NetBSD: ixp425_mem.c,v 1.3 2023/06/24 05:43:25 msaitoh Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixp425_mem.c,v 1.3 2023/06/24 05:43:25 msaitoh Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

static uint32_t sdram_64bit[] = {
	0x00800000,	/* 8M:  One 2M x 32 chip */
	0x01000000,	/* 16M: Two 2M x 32 chips */
	0x01000000,	/* 16M: One 4M x 32 chip */
	0x02000000,	/* 32M: Two 4M x 32 chips */
	0, 0, 0, 0
};

static uint32_t sdram_other[] = {
	0x02000000,	/*  32M: Two   8M x 16 chips */
	0x04000000,	/*  64M: Four  8M x 16 chips */
	0x04000000,	/*  64M: Two  16M x 16 chips */
	0x08000000,	/* 128M: Four 16M x 16 chips */
	0x08000000,	/* 128M: Two  32M x 16 chips */
	0x10000000,	/* 256M: Four 32M x 16 chips */
	0, 0
};

#define MCU_REG_READ(x)	(*(volatile uint32_t *)(IXP425_MCU_VBASE + (x)))

uint32_t
ixp425_sdram_size(void)
{
	uint32_t size, sdr_config;

	sdr_config = MCU_REG_READ(MCU_SDR_CONFIG);

	if (sdr_config & MCU_SDR_CONFIG_64MBIT)
		size = sdram_64bit[MCU_SDR_CONFIG_MCONF(sdr_config)];
	else
		size = sdram_other[MCU_SDR_CONFIG_MCONF(sdr_config)];

	if (size == 0) {
		printf("** SDR_CONFIG returns unknown value, using 32M\n");
		size = 32 * 1024 * 1024;
	}

	return (size);
}
