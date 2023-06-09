#	$NetBSD: siphash.inc,v 1.1.1.1 2023/04/18 14:19:04 christos Exp $
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/siphash
#

SIPHASH=${OPENSSLSRC}/crypto/siphash
.PATH:	${SIPHASH}


SIPHASH_SRCS += \
siphash.c \
siphash_ameth.c \
siphash_pmeth.c

SRCS += ${SIPHASH_SRCS}

.for cryptosrc in ${SIPHASH_SRCS}
CPPFLAGS.${cryptosrc} = -I${SIPHASH} ${SIPHASHCPPFLAGS}
.endfor
