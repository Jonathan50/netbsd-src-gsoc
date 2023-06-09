#	$NetBSD: Makefile,v 1.46 2023/06/03 21:26:28 lukem Exp $

STRIPFLAG=
PROGSOURCE=	boot.c net.c netif_sun.c conf.c openfirm.c bootinfo.c \
		mmu.c prompatch.c
NEWVERSWHAT=	"Secondary Boot"
FILES=		boot.net bootjs.net ${RELOCS:S/^/boot./g}
CLEANFILES:=	${FILES}
LINKS=		${BINDIR}/boot.${RELOC_DEFAULT} ${BINDIR}/boot

CPPFLAGS+=	-DBOOT_AOUT -DBOOT_ELF32 -DSUPPORT_DHCP
CFLAGS+=	-freestanding

INCLUDE_LIBZ=	yes
SAMISCMAKEFLAGS= SA_USE_CREAD=yes

# Follow the suit of Makefile.kern.inc; needed for the lfs64 union
# accessors -- they don't actually dereference the resulting pointer,
# just use it for type-checking.
CWARNFLAGS+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include "../Makefile.buildboot"

.if ${MACHINE} == sparc64
CFLAGS+=	-m32
LDFLAGS+=	-m elf32_sparc
AFLAGS+=	-Wa,-32
.endif

.include <bsd.own.mk>

OBJS=${SRCS:N*.h:N*.sh:N*.fth:R:S/$/.o/g}

LINKFLAGS=-N -e start -Map $@.map

.MAIN: all
realall: ${FILES}


# conjure up a magic header that is accepted by all Sun PROMS;
# see installboot.c for details.

SUN_MAGIC_HEADER='\01\03\01\07\060\200\0\07\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0'

boot.net: boot.${RELOC_DEFAULT}
	(printf ${SUN_MAGIC_HEADER}; cat ${.ALLSRC} ) > ${.TARGET}


.include "${S}/conf/newvers_stand.mk"

.include <bsd.prog.mk>

bootjs.net: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -S -o ${.TARGET} ${LINKFLAGS} -Ttext 300000 ${OBJS} \
	    ${LIBSA} ${LIBZ} ${LIBKERN}
	${SIZE} ${.TARGET}
CLEANFILES+=	bootjs.net.map

.for RELOC in ${RELOCS}
boot.${RELOC}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	${LD} -S -o ${.TARGET}.tmp ${LINKFLAGS} -Ttext ${RELOC} ${OBJS} \
	    ${LIBSA} ${LIBZ} ${LIBKERN}
	${SIZE} ${.TARGET}.tmp
	${OBJCOPY} -O binary ${.TARGET}.tmp ${.TARGET}
	rm -f ${.TARGET}.tmp
CLEANFILES+=	boot.${RELOC}.map
.endfor
