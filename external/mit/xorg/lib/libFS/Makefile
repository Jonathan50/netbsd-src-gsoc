#	$NetBSD: Makefile,v 1.5 2022/10/28 09:43:58 wiz Exp $

.include <bsd.own.mk>

LIB=	FS
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/fonts

SRCS=	FSCloseFt.c FSClServ.c FSConnServ.c FSErrDis.c FSErrHndlr.c FSFlush.c \
	FSFontInfo.c FSFtNames.c FSGetCats.c FSlibInt.c FSListCats.c \
	FSListExt.c FSMisc.c FSNextEv.c FSOpenFont.c FSOpenServ.c FSQGlyphs.c \
	FSQuExt.c FSQXExt.c FSQXInfo.c FSServName.c FSSetCats.c FSSync.c \
	FSSynchro.c fs_transport.c

CWARNFLAGS.clang+=	-Wno-string-plus-int

INCS=	FSlib.h
INCSDIR=${X11INCDIR}/X11/fonts

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/fonts \
		-DHAVE_REALLOCARRAY \
		-DHAVE_STRNLEN \
		-DHAVE_STRLCPY \
		-DHAVE_SETENV

CONN_DEFINES = ${X11FLAGS.CONNECTION} -DFS_t -DTRANS_CLIENT
SOCK_DEFINES = -DBSD44SOCKETS

CPPFLAGS+=	$(CONN_DEFINES) $(SOCK_DEFINES)

# XXX
COPTS.FSGetCats.c+=	-Wno-error
COPTS.FSListExt.c+=	-Wno-error
COPTS.FSOpenServ.c+=	-Wno-error

PKGCONFIG=	libfs
PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
