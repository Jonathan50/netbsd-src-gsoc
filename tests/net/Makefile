# $NetBSD: Makefile,v 1.41 2022/11/17 08:45:35 ozaki-r Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net

TESTS_SUBDIRS=		fdpass in_cksum net sys
.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_SUBDIRS+=		altq arp bpf bpfilter can carp icmp if if_bridge if_gif
TESTS_SUBDIRS+=		if_ipsec if_l2tp if_lagg if_loop if_pppoe if_tap
TESTS_SUBDIRS+=		if_tun if_vether if_vlan if_wg inpcb ipsec mcast mpls
TESTS_SUBDIRS+=		ndp npf route tcp
.if (${MKSLJIT} != "no")
TESTS_SUBDIRS+=		bpfjit
.endif
.endif

.include <bsd.test.mk>
