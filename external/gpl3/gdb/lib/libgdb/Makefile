#	$NetBSD: Makefile,v 1.32 2021/09/18 01:47:09 christos Exp $

NOCTF=
HOSTPROG_CXX=   1

.include <bsd.own.mk>

.include "../Makefile.inc"

LIBISPRIVATE=	yes
LIB=		gdb
GDBARCHDIR=	${.CURDIR}/arch/${GDB_MACHINE_ARCH}

.include "${GDBARCHDIR}/defs.mk"

SRCS=		${G_LIBGDB_OBS:.o=.c} dwarf2-frame.c
GCPPFLAGS=	${G_INTERNAL_CFLAGS}
CPPFLAGS+=	-I${.CURDIR} \
		-I${GDBARCHDIR} \
		-I${DIST} \
		-I${DIST}/gdb \
		-I${DIST}/gdbsupport \
		-I${DIST}/gdb/config \
		-I${DIST}/gnulib/import \
		-I${DIST}/include/opcode \
		-I${DIST}/readline \
		-I${DIST}/libdecnumber \
		-I${.CURDIR}/../libgnulib/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../libgnulib/arch/${GDB_MACHINE_ARCH}/gnulib/import \
		-I${.CURDIR}/../libgdbsupport/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../libbfd/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../libdecnumber/arch/${GDB_MACHINE_ARCH} \
		-I${DIST}/bfd \
		-I${DIST}/include \
		-Dsighandler_t=sig_t \
		-DTARGET_SYSTEM_ROOT=\"\" \
		-DTARGET_SYSTEM_ROOT_RELOCATABLE=0 \
		-DBINDIR=\"${BINDIR}\" \
		${GCPPFLAGS:M-D*}

CWARNFLAGS.clang+=	-Wno-switch

.include "../../Makefile.inc"
.include <bsd.lib.mk>

# Force c++
CWARNFLAGS.clang+=	-Wno-deprecated
HOST_CC:=		${HOST_CXX}
HOST_CFLAGS+=		${HOST_CXXFLAGS}
CC:=			${CXX}
CFLAGS:=		${CXXFLAGS} -std=gnu++17 -Wno-error=stack-protector

.PATH: ${DIST}/gdb ${DIST}/gdb/arch ${DIST}/gdb/mi ${DIST}/gdb/cli  \
    ${DIST}/gdb/tui ${DIST}/gdb/python ${DIST}/gdb/common ${DIST}/gdb/target \
    ${DIST}/gdb/nat ${DIST}/gdb/guile ${DIST}/gdb/compile ${DIST}/gdb/doc \
    ${DIST}/gdb/dwarf2 ${DIST}/gdb/unittests \
    ${GDBARCHDIR}

#

ada-exp.c: ada-lex.c

.if ${MACHINE} == "vax"
. if ${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 8
COPTS.read.c+=	-O0
. endif
.endif

# These are generated by implicit rules and are not easy to generate
CLEANDIRFILES+= \
	ada-exp.c ada-lex.c \
	c-exp.c \
	d-exp.c \
	cp-name-parser.c \
	f-exp.c \
	go-exp.c \
	jv-exp.c \
	m2-exp.c \
	p-exp.c \
	rust-exp.c

# corelow.c
CWARNFLAGS.gcc+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 9:? -Wno-error=alloca-larger-than= :}
