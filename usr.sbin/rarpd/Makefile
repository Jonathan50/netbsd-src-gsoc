#	$NetBSD: Makefile,v 1.16 2023/06/03 09:09:22 lukem Exp $

USE_FORT?= yes	# network server

PROG=	rarpd
SRCS=	rarpd.c mkarp.c
MAN=	rarpd.8

# Uncomment the following to require a boot file in TFTP_DIR
# to exist for a rarp reply to be sent.
# CPPFLAGS+=-DREQUIRE_TFTPBOOT -DTFTP_DIR=\"/tftpboot\"
CPPFLAGS+=-DHAVE_IFADDRS_H

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

COPTS.rarpd.c+=	${CC_WNO_STRINGOP_TRUNCATION}

COPTS.mkarp.c+=	-Wno-pointer-sign
