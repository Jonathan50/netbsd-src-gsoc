/*	$NetBSD: crt0-efi-arm.S,v 1.1.1.2 2021/09/30 18:50:09 jmcneill Exp $	*/

/*
 * crt0-efi-arm.S - PE/COFF header for ARM EFI applications
 *
 * Copright (C) 2014 Linaro Ltd. <ard.biesheuvel@linaro.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice and this list of conditions, without modification.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later version.
 */

	.section	.text.head

	/*
	 * Magic "MZ" signature for PE/COFF
	 */
	.globl	ImageBase
ImageBase:
	.ascii	"MZ"
	.skip	58				// 'MZ' + pad + offset == 64
	.long	pe_header - ImageBase		// Offset to the PE header.
pe_header:
	.ascii	"PE"
	.short 	0
coff_header:
	.short	0x1c2				// Mixed ARM/Thumb
	.short	2				// nr_sections
	.long	0 				// TimeDateStamp
	.long	0				// PointerToSymbolTable
	.long	0				// NumberOfSymbols
	.short	section_table - optional_header	// SizeOfOptionalHeader
	.short	0x306				// Characteristics.
						// IMAGE_FILE_32BIT_MACHINE |
						// IMAGE_FILE_DEBUG_STRIPPED |
						// IMAGE_FILE_EXECUTABLE_IMAGE |
						// IMAGE_FILE_LINE_NUMS_STRIPPED
optional_header:
	.short	0x10b				// PE32+ format
	.byte	0x02				// MajorLinkerVersion
	.byte	0x14				// MinorLinkerVersion
	.long	_edata - _start			// SizeOfCode
	.long	0				// SizeOfInitializedData
	.long	0				// SizeOfUninitializedData
	.long	_start - ImageBase		// AddressOfEntryPoint
	.long	_start - ImageBase		// BaseOfCode
	.long	0				// BaseOfData

extra_header_fields:
	.long	0				// ImageBase
	.long	0x20				// SectionAlignment
	.long	0x8				// FileAlignment
	.short	0				// MajorOperatingSystemVersion
	.short	0				// MinorOperatingSystemVersion
	.short	0				// MajorImageVersion
	.short	0				// MinorImageVersion
	.short	0				// MajorSubsystemVersion
	.short	0				// MinorSubsystemVersion
	.long	0				// Win32VersionValue

	.long	_edata - ImageBase		// SizeOfImage

	// Everything before the kernel image is considered part of the header
	.long	_start - ImageBase		// SizeOfHeaders
	.long	0				// CheckSum
	.short	EFI_SUBSYSTEM			// Subsystem
	.short	0				// DllCharacteristics
	.long	0				// SizeOfStackReserve
	.long	0				// SizeOfStackCommit
	.long	0				// SizeOfHeapReserve
	.long	0				// SizeOfHeapCommit
	.long	0				// LoaderFlags
	.long	0x6				// NumberOfRvaAndSizes

	.quad	0				// ExportTable
	.quad	0				// ImportTable
	.quad	0				// ResourceTable
	.quad	0				// ExceptionTable
	.quad	0				// CertificationTable
	.quad	0				// BaseRelocationTable

	// Section table
section_table:

	/*
	 * The EFI application loader requires a relocation section
	 * because EFI applications must be relocatable.  This is a
	 * dummy section as far as we are concerned.
	 */
	.ascii	".reloc"
	.byte	0
	.byte	0			// end of 0 padding of section name
	.long	0
	.long	0
	.long	0			// SizeOfRawData
	.long	0			// PointerToRawData
	.long	0			// PointerToRelocations
	.long	0			// PointerToLineNumbers
	.short	0			// NumberOfRelocations
	.short	0			// NumberOfLineNumbers
	.long	0x42100040		// Characteristics (section flags)


	.ascii	".text"
	.byte	0
	.byte	0
	.byte	0        		// end of 0 padding of section name
	.long	_edata - _start		// VirtualSize
	.long	_start - ImageBase	// VirtualAddress
	.long	_edata - _start		// SizeOfRawData
	.long	_start - ImageBase	// PointerToRawData

	.long	0		// PointerToRelocations (0 for executables)
	.long	0		// PointerToLineNumbers (0 for executables)
	.short	0		// NumberOfRelocations  (0 for executables)
	.short	0		// NumberOfLineNumbers  (0 for executables)
	.long	0xe0500020	// Characteristics (section flags)

_start:
	stmfd		sp!, {r0-r2, lr}

	mov		r2, r0
	mov		r3, r1
	adr		r1, .L_DYNAMIC
	ldr		r0, [r1]
	add		r1, r0, r1
	adr		r0, ImageBase
	bl		_relocate
	teq		r0, #0
	bne		0f

	ldmfd		sp, {r0-r1}
	bl		efi_main

0:	add		sp, sp, #12
	ldr		pc, [sp], #4

.L_DYNAMIC:
	.word		_DYNAMIC - .
