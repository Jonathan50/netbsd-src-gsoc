/*	$NetBSD: buf.c,v 1.28 2023/03/14 10:36:06 kre Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Luke Mewburn for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(__lint)
__RCSID("$NetBSD: buf.c,v 1.28 2023/03/14 10:36:06 kre Exp $");
#endif	/* !__lint */

#include <sys/param.h>
#include <sys/time.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#include "makefs.h"
#include "buf.h"

TAILQ_HEAD(buftailhead,buf) buftail;

int
bread(struct vnode *vp, daddr_t blkno, int size, int u2 __unused,
	struct buf **bpp)
{
	off_t	offset;
	ssize_t	rv;
	fsinfo_t *fs = vp->fs;
	int saved_errno;

	assert (bpp != NULL);

	if (debug & DEBUG_BUF_BREAD)
		printf("%s: blkno %jd size %d\n", __func__,
		    (intmax_t)blkno, size);
	*bpp = getblk(vp, blkno, size, 0, 0);
	offset = (*bpp)->b_blkno * fs->sectorsize + fs->offset;
	if (debug & DEBUG_BUF_BREAD)
		printf("%s: blkno %jd offset %jd bcount %ld\n", __func__,
		    (intmax_t)(*bpp)->b_blkno, (intmax_t) offset,
		    (*bpp)->b_bcount);
	if (lseek((*bpp)->b_fs->fd, offset, SEEK_SET) == -1) {
		saved_errno = errno;
		warn("%s: lseek %jd (%jd)", __func__,
		    (intmax_t)(*bpp)->b_blkno, (intmax_t)offset);
		goto out;
	}
	rv = read((*bpp)->b_fs->fd, (*bpp)->b_data, (size_t)(*bpp)->b_bcount);
	saved_errno = errno;
	if (debug & DEBUG_BUF_BREAD)
		printf("%s: read %ld (%jd) returned %zd\n", __func__,
		    (*bpp)->b_bcount, (intmax_t)offset, rv);
	if (rv == -1) {				/* read error */
		warn("%s: read %ld (%jd) returned %zd", __func__,
		    (*bpp)->b_bcount, (intmax_t)offset, rv);
		goto out;
	}
	if (rv != (*bpp)->b_bcount) {	/* short read */
		saved_errno = ENOSPC;
		warn("%s: read %ld (%jd) returned %zd", __func__,
		    (*bpp)->b_bcount, (intmax_t)offset, rv);
		goto out;
	}
	return 0;
out:
	brelse(*bpp, 0);
	*bpp = NULL;
#if 1
	__USE(saved_errno);
	exit(EXIT_FAILURE);
#else
	return saved_errno;
#endif
}

void
brelse(struct buf *bp, int u1 __unused)
{

	assert (bp != NULL);
	assert (bp->b_data != NULL);

	if (bp->b_lblkno < 0) {
		/*
		 * XXX	don't remove any buffers with negative logical block
		 *	numbers (lblkno), so that we retain the mapping
		 *	of negative lblkno -> real blkno that ffs_balloc()
		 *	sets up.
		 *
		 *	if we instead released these buffers, and implemented
		 *	ufs_strategy() (and ufs_bmaparray()) and called those
		 *	from bread() and bwrite() to convert the lblkno to
		 *	a real blkno, we'd add a lot more code & complexity
		 *	and reading off disk, for little gain, because this
		 *	simple hack works for our purpose.
		 */
		bp->b_bcount = 0;
		return;
	}

	TAILQ_REMOVE(&buftail, bp, b_tailq);
	free(bp->b_data);
	free(bp);
}

int
bwrite(struct buf *bp)
{
	off_t	offset;
	ssize_t	rv;
	size_t	bytes;
	int	e;
	fsinfo_t *fs = bp->b_fs;

	assert (bp != NULL);
	offset = bp->b_blkno * fs->sectorsize + fs->offset;
	bytes  = (size_t)bp->b_bcount;
	if (debug & DEBUG_BUF_BWRITE)
		printf("%s: blkno %jd offset %jd bcount %zu\n", __func__,
		    (intmax_t)bp->b_blkno, (intmax_t) offset, bytes);
	if (lseek(bp->b_fs->fd, offset, SEEK_SET) == -1)
		return errno;
	rv = write(bp->b_fs->fd, bp->b_data, bytes);
	e = errno;
	if (debug & DEBUG_BUF_BWRITE)
		printf("%s: write %ld (offset %jd) returned %jd\n", __func__,
		    bp->b_bcount, (intmax_t)offset, (intmax_t)rv);
	brelse(bp, 0);
	if (rv == (ssize_t)bytes)
		return 0;
	if (rv == -1)		/* write error */
		return e;
	return EAGAIN;
}

void
bcleanup(void)
{
	struct buf *bp;

	/*
	 * XXX	this really shouldn't be necessary, but i'm curious to
	 *	know why there's still some buffers lying around that
	 *	aren't brelse()d
	 */

	if (TAILQ_EMPTY(&buftail))
		return;

	printf("%s: unflushed buffers:\n", __func__);
	TAILQ_FOREACH(bp, &buftail, b_tailq) {
		printf("\tlblkno %10jd blkno %10jd count %6ld  bufsize %6ld\n",
		    (intmax_t)bp->b_lblkno, (intmax_t)bp->b_blkno,
		    bp->b_bcount, bp->b_bufsize);
	}
	printf("%s: done\n", __func__);
}

struct buf *
getblk(struct vnode *vp, daddr_t blkno, int size, int u1 __unused,
    int u2 __unused)
{
	static int buftailinitted;
	struct buf *bp;
	void *n;

	if (debug & DEBUG_BUF_GETBLK)
		printf("%s: blkno %jd size %d\n", __func__,
		    (intmax_t)blkno, size);

	bp = NULL;
	if (!buftailinitted) {
		if (debug & DEBUG_BUF_GETBLK)
			printf("%s: initialising tailq\n", __func__);
		TAILQ_INIT(&buftail);
		buftailinitted = 1;
	} else {
		TAILQ_FOREACH(bp, &buftail, b_tailq) {
			if (bp->b_lblkno != blkno)
				continue;
			break;
		}
	}
	if (bp == NULL) {
		bp = ecalloc(1, sizeof(*bp));
		bp->b_bufsize = 0;
		bp->b_blkno = bp->b_lblkno = blkno;
		bp->b_fs = vp->fs;
		bp->b_data = NULL;
		TAILQ_INSERT_HEAD(&buftail, bp, b_tailq);
	}
	bp->b_bcount = size;
	if (bp->b_data == NULL || bp->b_bcount > bp->b_bufsize) {
		n = erealloc(bp->b_data, (size_t)size);
		memset(n, 0, (size_t)size);
		bp->b_data = n;
		bp->b_bufsize = size;
	}

	return bp;
}
