/*	$NetBSD: talink_58.h,v 1.5 2022/09/23 12:15:31 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * http://www.iana.org/assignments/dns-parameters/TALINK/talink-completed-template
 */

#ifndef GENERIC_TALINK_58_H
#define GENERIC_TALINK_58_H 1

typedef struct dns_rdata_talink {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_name_t prev;
	dns_name_t next;
} dns_rdata_talink_t;

#endif /* GENERIC_TALINK_58_H */