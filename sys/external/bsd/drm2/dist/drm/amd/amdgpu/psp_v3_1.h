/*	$NetBSD: psp_v3_1.h,v 1.2 2021/12/18 23:44:59 riastradh Exp $	*/

/*
 * Copyright 2016 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author: Huang Rui
 *
 */
#ifndef __PSP_V3_1_H__
#define __PSP_V3_1_H__

#include "amdgpu_psp.h"

enum { PSP_DIRECTORY_TABLE_ENTRIES = 4 };
enum { PSP_BINARY_ALIGNMENT = 64 };
enum { PSP_BOOTLOADER_1_MEG_ALIGNMENT = 0x100000 };
enum { PSP_BOOTLOADER_8_MEM_ALIGNMENT = 0x800000 };

void psp_v3_1_set_psp_funcs(struct psp_context *psp);

#endif