#	$NetBSD: Makefile,v 1.59 2023/06/03 09:09:17 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

USE_FORT?= yes	# setuid
PROG=	login
SRCS=	copyrightstr.c
DPADD+=	${LIBUTIL} ${LIBCRYPT}
LDADD+=	-lutil -lcrypt
BINOWN=	root
BINMODE=4555

SRCS+=	common.c
.if (${USE_PAM} != "no")
SRCS+=	login_pam.c
LDADD+=	-lpam ${PAM_STATIC_LDADD}
DPADD+= ${LIBPAM} ${PAM_STATIC_DPADD}
.else	# USE_PAM == no
SRCS+=	login.c
CPPFLAGS+=-DLOGIN_CAP -DSUPPORT_UTMP -DSUPPORT_UTMPX

.if (${USE_KERBEROS} != "no")
SRCS+= k5login.c
CPPFLAGS+=-DKERBEROS5
LDADD+= ${LIBKRB5_LDADD}
DPADD+= ${LIBKRB5_DPADD}
.endif

.if (${USE_SKEY} != "no")
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+=	-lskey
.endif
.endif	# USE_PAM == no

CLEANFILES+=	copyrightstr.c

copyrightstr.c: ${NETBSDSRCDIR}/sys/conf/copyright
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${TOOL_AWK} '\
		BEGIN { print "const char copyrightstr[] =" }\
		{ print "\""$$0"\\n\""}\
		END { print "\"\\n\";" }\
	' ${.ALLSRC} > ${.TARGET}

COPTS.parse.c+=		${CC_WNO_STRINGOP_TRUNCATION}
COPTS.common.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
