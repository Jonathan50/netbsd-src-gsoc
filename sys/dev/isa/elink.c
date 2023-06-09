/*	$NetBSD: elink.c,v 1.19 2022/09/25 17:11:48 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Charles M. Hannum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Common code for dealing with 3COM ethernet cards.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: elink.c,v 1.19 2022/09/25 17:11:48 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/queue.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/isa/elink.h>

/*
 * This list keeps track of which ISAs have gotten an elink_reset().
 */
struct elink_done_reset {
	LIST_ENTRY(elink_done_reset)	er_link;
	int				er_bus;
};
static LIST_HEAD(, elink_done_reset) elink_all_resets;
static int elink_all_resets_initialized;

/*
 * Issue a `global reset' to all cards, and reset the ID state machines.  We
 * have to be careful to do the global reset only once during autoconfig, to
 * prevent resetting boards that have already been configured.
 *
 * The "bus" argument here is the unit number of the ISA bus, e.g. "0"
 * if the bus is "isa0".
 *
 * NOTE: the caller MUST provide an access handle for ELINK_ID_PORT!
 */
void
elink_reset(bus_space_tag_t iot, bus_space_handle_t ioh, int bus)
{
	struct elink_done_reset *er;

	if (elink_all_resets_initialized == 0) {
		LIST_INIT(&elink_all_resets);
		elink_all_resets_initialized = 1;
	}

	/*
	 * Reset these cards if we haven't done so already.
	 */
	for (er = elink_all_resets.lh_first; er != NULL;
	    er = er->er_link.le_next)
		if (er->er_bus == bus)
			goto out;

	/* Mark this bus so we don't do it again. */
	er = kmem_alloc(sizeof(*er), KM_SLEEP);
	er->er_bus = bus;
	LIST_INSERT_HEAD(&elink_all_resets, er, er_link);

	/* Haven't reset the cards on this bus, yet. */
	bus_space_write_1(iot, ioh, 0, ELINK_RESET);

 out:
	bus_space_write_1(iot, ioh, 0, 0x00);
	bus_space_write_1(iot, ioh, 0, 0x00);
}

/*
 * The `ID sequence' is really just snapshots of an 8-bit CRC register as 0
 * bits are shifted in.  Different board types use different polynomials.
 *
 * NOTE: the caller MUST provide an access handle for ELINK_ID_PORT!
 */
void
elink_idseq(bus_space_tag_t iot, bus_space_handle_t ioh, u_char p)
{
	int i;
	u_char c;

	c = 0xff;
	for (i = 255; i; i--) {
		bus_space_write_1(iot, ioh, 0, c);
		if (c & 0x80) {
			c <<= 1;
			c ^= p;
		} else
			c <<= 1;
	}
}
