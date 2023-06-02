/*	$NetBSD: gen6_renderstate.c,v 1.2 2021/12/18 23:45:30 riastradh Exp $	*/

/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Generated by: intel-gpu-tools-1.8-220-g01153e7
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gen6_renderstate.c,v 1.2 2021/12/18 23:45:30 riastradh Exp $");

#include "intel_renderstate.h"

static const u32 gen6_null_state_relocs[] = {
	0x00000020,
	0x00000024,
	0x0000002c,
	0x000001e0,
	0x000001e4,
	-1,
};

static const u32 gen6_null_state_batch[] = {
	0x69040000,
	0x790d0001,
	0x00000000,
	0x00000000,
	0x78180000,
	0x00000001,
	0x61010008,
	0x00000000,
	0x00000001,	 /* reloc */
	0x00000001,	 /* reloc */
	0x00000000,
	0x00000001,	 /* reloc */
	0x00000000,
	0x00000001,
	0x00000000,
	0x00000001,
	0x61020000,
	0x00000000,
	0x78050001,
	0x00000018,
	0x00000000,
	0x780d1002,
	0x00000000,
	0x00000000,
	0x00000420,
	0x78150003,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78100004,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78160003,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78110005,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78120002,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78170003,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x79050005,
	0xe0040000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x79100000,
	0x00000000,
	0x79000002,
	0xffffffff,
	0x00000000,
	0x00000000,
	0x780e0002,
	0x00000441,
	0x00000401,
	0x00000401,
	0x78021002,
	0x00000000,
	0x00000000,
	0x00000400,
	0x78130012,
	0x00400810,
	0x00000000,
	0x20000000,
	0x04000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x78140007,
	0x00000280,
	0x08080000,
	0x00000000,
	0x00060000,
	0x4e080002,
	0x00100400,
	0x00000000,
	0x00000000,
	0x78090005,
	0x02000000,
	0x22220000,
	0x02f60000,
	0x11330000,
	0x02850004,
	0x11220000,
	0x78011002,
	0x00000000,
	0x00000000,
	0x00000200,
	0x78080003,
	0x00002000,
	0x00000448,	 /* reloc */
	0x00000448,	 /* reloc */
	0x00000000,
	0x05000000,	 /* cmds end */
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000220,	 /* state start */
	0x00000240,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x0060005a,
	0x204077be,
	0x000000c0,
	0x008d0040,
	0x0060005a,
	0x206077be,
	0x000000c0,
	0x008d0080,
	0x0060005a,
	0x208077be,
	0x000000d0,
	0x008d0040,
	0x0060005a,
	0x20a077be,
	0x000000d0,
	0x008d0080,
	0x00000201,
	0x20080061,
	0x00000000,
	0x00000000,
	0x00600001,
	0x20200022,
	0x008d0000,
	0x00000000,
	0x02800031,
	0x21c01cc9,
	0x00000020,
	0x0a8a0001,
	0x00600001,
	0x204003be,
	0x008d01c0,
	0x00000000,
	0x00600001,
	0x206003be,
	0x008d01e0,
	0x00000000,
	0x00600001,
	0x208003be,
	0x008d0200,
	0x00000000,
	0x00600001,
	0x20a003be,
	0x008d0220,
	0x00000000,
	0x00600001,
	0x20c003be,
	0x008d0240,
	0x00000000,
	0x00600001,
	0x20e003be,
	0x008d0260,
	0x00000000,
	0x00600001,
	0x210003be,
	0x008d0280,
	0x00000000,
	0x00600001,
	0x212003be,
	0x008d02a0,
	0x00000000,
	0x05800031,
	0x24001cc8,
	0x00000040,
	0x90019000,
	0x0000007e,
	0x00000000,
	0x00000000,
	0x00000000,
	0x0000007e,
	0x00000000,
	0x00000000,
	0x00000000,
	0x0000007e,
	0x00000000,
	0x00000000,
	0x00000000,
	0x0000007e,
	0x00000000,
	0x00000000,
	0x00000000,
	0x0000007e,
	0x00000000,
	0x00000000,
	0x00000000,
	0x0000007e,
	0x00000000,
	0x00000000,
	0x00000000,
	0x0000007e,
	0x00000000,
	0x00000000,
	0x00000000,
	0x0000007e,
	0x00000000,
	0x00000000,
	0x00000000,
	0x30000000,
	0x00000124,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0xf99a130c,
	0x799a130c,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x80000031,
	0x00000003,
	0x00000000,	 /* state end */
};

RO_RENDERSTATE(6);