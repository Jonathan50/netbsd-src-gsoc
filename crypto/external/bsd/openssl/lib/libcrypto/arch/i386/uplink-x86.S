#include <machine/asm.h>
.text
.globl	OPENSSL_UplinkTable
.type	_$lazy1,@function
.align	16
_$lazy1:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$1
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*4(%eax)
.size	_$lazy1,.-_$lazy1
.type	_$lazy2,@function
.align	16
_$lazy2:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$2
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*8(%eax)
.size	_$lazy2,.-_$lazy2
.type	_$lazy3,@function
.align	16
_$lazy3:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$3
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*12(%eax)
.size	_$lazy3,.-_$lazy3
.type	_$lazy4,@function
.align	16
_$lazy4:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$4
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*16(%eax)
.size	_$lazy4,.-_$lazy4
.type	_$lazy5,@function
.align	16
_$lazy5:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$5
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*20(%eax)
.size	_$lazy5,.-_$lazy5
.type	_$lazy6,@function
.align	16
_$lazy6:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$6
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*24(%eax)
.size	_$lazy6,.-_$lazy6
.type	_$lazy7,@function
.align	16
_$lazy7:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$7
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*28(%eax)
.size	_$lazy7,.-_$lazy7
.type	_$lazy8,@function
.align	16
_$lazy8:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$8
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*32(%eax)
.size	_$lazy8,.-_$lazy8
.type	_$lazy9,@function
.align	16
_$lazy9:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$9
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*36(%eax)
.size	_$lazy9,.-_$lazy9
.type	_$lazy10,@function
.align	16
_$lazy10:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$10
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*40(%eax)
.size	_$lazy10,.-_$lazy10
.type	_$lazy11,@function
.align	16
_$lazy11:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$11
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*44(%eax)
.size	_$lazy11,.-_$lazy11
.type	_$lazy12,@function
.align	16
_$lazy12:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$12
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*48(%eax)
.size	_$lazy12,.-_$lazy12
.type	_$lazy13,@function
.align	16
_$lazy13:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$13
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*52(%eax)
.size	_$lazy13,.-_$lazy13
.type	_$lazy14,@function
.align	16
_$lazy14:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$14
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*56(%eax)
.size	_$lazy14,.-_$lazy14
.type	_$lazy15,@function
.align	16
_$lazy15:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$15
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*60(%eax)
.size	_$lazy15,.-_$lazy15
.type	_$lazy16,@function
.align	16
_$lazy16:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$16
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*64(%eax)
.size	_$lazy16,.-_$lazy16
.type	_$lazy17,@function
.align	16
_$lazy17:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$17
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*68(%eax)
.size	_$lazy17,.-_$lazy17
.type	_$lazy18,@function
.align	16
_$lazy18:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$18
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*72(%eax)
.size	_$lazy18,.-_$lazy18
.type	_$lazy19,@function
.align	16
_$lazy19:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$19
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*76(%eax)
.size	_$lazy19,.-_$lazy19
.type	_$lazy20,@function
.align	16
_$lazy20:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$20
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*80(%eax)
.size	_$lazy20,.-_$lazy20
.type	_$lazy21,@function
.align	16
_$lazy21:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$21
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*84(%eax)
.size	_$lazy21,.-_$lazy21
.type	_$lazy22,@function
.align	16
_$lazy22:
	#ifdef __CET__

.byte	243,15,30,251
	#endif

	leal	OPENSSL_UplinkTable,%eax
	pushl	$22
	pushl	%eax
	call	OPENSSL_Uplink
	popl	%eax
	addl	$4,%esp
	jmp	*88(%eax)
.size	_$lazy22,.-_$lazy22
.data
.align	4
OPENSSL_UplinkTable:
.long	22
.long	_$lazy1
.long	_$lazy2
.long	_$lazy3
.long	_$lazy4
.long	_$lazy5
.long	_$lazy6
.long	_$lazy7
.long	_$lazy8
.long	_$lazy9
.long	_$lazy10
.long	_$lazy11
.long	_$lazy12
.long	_$lazy13
.long	_$lazy14
.long	_$lazy15
.long	_$lazy16
.long	_$lazy17
.long	_$lazy18
.long	_$lazy19
.long	_$lazy20
.long	_$lazy21
.long	_$lazy22

	.section ".note.gnu.property", "a"
	.p2align 2
	.long 1f - 0f
	.long 4f - 1f
	.long 5
0:
	.asciz "GNU"
1:
	.p2align 2
	.long 0xc0000002
	.long 3f - 2f
2:
	.long 3
3:
	.p2align 2
4:
