#      $NetBSD: Makefile,v 1.6 2021/08/19 09:05:23 mrg Exp $

.include <bsd.own.mk>

LIB=	drm_amdgpu
.PATH: ${X11SRCDIR.drm}
.PATH: ${X11SRCDIR.drm}/amdgpu

SRCS= \
	amdgpu_asic_id.c \
	amdgpu_bo.c \
	amdgpu_cs.c \
	amdgpu_device.c \
	amdgpu_gpu_info.c \
	amdgpu_vamgr.c \
	amdgpu_vm.c \
	handle_table.c

LIBDRMDATADIR=	${X11ROOTDIR}/share/libdrm
# XXX goes away in new libdrm.
IDS_NENT!=	egrep -ci '^[0-9a-f]{4},.*[0-9a-f]+,' ${X11SRCDIR.drm}/data/amdgpu.ids

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 -I${X11SRCDIR.drm}
CPPFLAGS+=	-DAMDGPU_ASIC_ID_TABLE=\"${LIBDRMDATADIR}/amdgpu.ids\" \
		-DAMDGPU_ASIC_ID_TABLE_NUM_ENTRIES=$(IDS_NENT)

LIBDPLIBS=	drm	${.CURDIR}/../libdrm

PKGCONFIG=	libdrm_amdgpu
PKGDIST=	drm

COPTS.amdgpu_cs.c+= -Wno-stack-protector

FILES=		amdgpu.ids
FILESDIR=	${LIBDRMDATADIR}

.PATH:          ${X11SRCDIR.drm}/data

.include "../libdrm/Makefile.defines"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
