# $NetBSD: Makefile,v 1.10 2023/06/03 21:33:02 lukem Exp $

.include <bsd.own.mk>

SRCS   = dhcpd.c dhcp.c dhcpv6.c bootp.c confpars.c db.c class.c failover.c \
         omapi.c mdb.c mdb6.c stables.c salloc.c ddns.c dhcpleasequery.c
PROG   = dhcpd
MAN    = dhcpd.8 dhcpd.conf.5 dhcpd.leases.5

DHCPSRCDIR=	server

.if ${MKSHARE} != "no"
FILESDIR= /usr/share/examples/dhcp

FILES=	dhcpd.conf
.endif

.if !empty(MACHINE_ARCH:Msh3*) || ${MACHINE_ARCH} == "m68000"
COPTS.ddns.c +=		${CC_WNO_STRINGOP_OVERFLOW}
.endif

COPTS.omapi.c +=	-Wno-stack-protector
COPTS.confpars.c+=	${CC_WNO_STRINGOP_TRUNCATION}
COPTS.mdb6.c+=		${CC_WNO_STRINGOP_OVERFLOW}

.include <bsd.prog.mk>
