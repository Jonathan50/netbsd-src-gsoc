/*	$NetBSD: mail_task.c,v 1.3 2022/10/08 16:12:45 christos Exp $	*/

/*++
/* NAME
/*	mail_task 3
/* SUMMARY
/*	set task name for logging purposes
/* SYNOPSIS
/*	#include <mail_task.h>
/*
/*	const char *mail_task(argv0)
/*	const char *argv0;
/* DESCRIPTION
/*	mail_task() enforces consistent naming of mailer processes.
/*	It strips pathname information from the process name, and
/*	prepends the name of the mail system so that logfile entries
/*	are easier to recognize. The mail system name is specified
/*	with the "syslog_name" configuration parameter.
/*
/*	The result is overwritten with each call.
/*
/*	A null argv0 argument requests that the current result is
/*	returned, or "unknown" when no current result exists.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include <vstring.h>
#include <safe.h>

/* Global library. */

#include "mail_params.h"
#include "mail_conf.h"
#include "mail_task.h"

/* mail_task - clean up and decorate the process name */

const char *mail_task(const char *argv0)
{
    static VSTRING *canon_name;
    const char *slash;
    const char *tag;

    if (argv0 == 0 && canon_name == 0)
	argv0 = "unknown";
    if (argv0) {
	if (canon_name == 0)
	    canon_name = vstring_alloc(10);
	if ((slash = strrchr(argv0, '/')) != 0 && slash[1])
	    argv0 = slash + 1;
	/* Setenv()-ed from main.cf, or inherited from master. */
	if ((tag = safe_getenv(CONF_ENV_LOGTAG)) == 0)
	    /* Check main.cf settings directly, in case set-gid. */
	    tag = var_syslog_name ? var_syslog_name :
		mail_conf_eval(DEF_SYSLOG_NAME);
	vstring_sprintf(canon_name, "%s/%s", tag, argv0);
    }
    return (vstring_str(canon_name));
}
