#	$NetBSD: Makefile,v 1.14 2023/06/03 09:09:20 lukem Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	altq
SRCS=	parser.c qop.c qop_blue.c qop_cbq.c qop_cdnr.c qop_conf.c \
	qop_dummy.c qop_errlist.c qop_fifoq.c qop_hfsc.c qop_jobs.c \
	qop_priq.c qop_red.c qop_rio.c qop_wfq.c quip_server.c

CPPFLAGS+= -DALTQ

.if (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

COPTS+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.lib.mk>
