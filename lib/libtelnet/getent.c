/*	$NetBSD: getent.c,v 1.11 2012/03/21 05:33:27 matt Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)getent.c	8.2 (Berkeley) 12/15/93";
#else
__RCSID("$NetBSD: getent.c,v 1.11 2012/03/21 05:33:27 matt Exp $");
#endif
#endif /* not lint */

#include <stdlib.h>
#include "misc-proto.h"

static char *area;

int getent(char *, char *);
char *getstr(char *, char **);

/*ARGSUSED*/
int
getent(char *cp, char *name)
{
#ifdef	HAS_CGETENT
	const char *dba[2];

	dba[0] = "/etc/gettytab";
	dba[1] = 0;
	return((cgetent(&area, dba, name) == 0) ? 1 : 0);
#else
	return(0);
#endif
}

#ifndef	SOLARIS
/*ARGSUSED*/
char *
getstr(char *id, char **cpp)
{
# ifdef	HAS_CGETENT
	char *answer;
	return((cgetstr(area, id, &answer) > 0) ? answer : 0);
# else
	return(0);
# endif
}
#endif