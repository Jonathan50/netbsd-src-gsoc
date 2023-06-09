/*	$NetBSD: amdgpu_module.c,v 1.11 2022/07/23 12:52:09 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amdgpu_module.c,v 1.11 2022/07/23 12:52:09 riastradh Exp $");

#include <sys/types.h>
#include <sys/module.h>
#ifndef _MODULE
#include <sys/once.h>
#endif
#include <sys/systm.h>

#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_sysctl.h>

#include <linux/idr.h>
#include <linux/mutex.h>

#include "amdgpu.h"
#include "amdgpu_amdkfd.h"
#include "amdgpu_drv.h"

MODULE(MODULE_CLASS_DRIVER, amdgpu, "drmkms,drmkms_pci,drmkms_sched,drmkms_ttm"); /* XXX drmkms_i2c */

#ifdef _MODULE
#include "ioconf.c"
#endif

/* XXX Kludge to get these from amdgpu_drv.c.  */
extern struct drm_driver *const amdgpu_drm_driver;
extern struct amdgpu_mgpu_info mgpu_info;

/* XXX Kludge to replace DEFINE_IDA in amdgpu_ids.c.  */
extern struct ida amdgpu_pasid_ida;

struct drm_sysctl_def amdgpu_def = DRM_SYSCTL_INIT();

static int
amdgpu_init(void)
{
	int error;

	error = drm_guarantee_initialized();
	if (error)
		return error;

	amdgpu_drm_driver->num_ioctls = amdgpu_max_kms_ioctl;
	amdgpu_drm_driver->driver_features &= ~DRIVER_ATOMIC;

	linux_mutex_init(&mgpu_info.mutex);
	ida_init(&amdgpu_pasid_ida);

	error = amdgpu_sync_init();
	KASSERT(error == 0);
	error = amdgpu_fence_slab_init();
	KASSERT(error == 0);

#if notyet			/* XXX amdgpu acpi */
	amdgpu_register_atpx_handler();
#endif

	amdgpu_amdkfd_init();
	drm_sysctl_init(&amdgpu_def);

	return 0;
}

int	amdgpu_guarantee_initialized(void); /* XXX */
int
amdgpu_guarantee_initialized(void)
{
#ifdef _MODULE
	return 0;
#else
	static ONCE_DECL(amdgpu_init_once);

	return RUN_ONCE(&amdgpu_init_once, &amdgpu_init);
#endif
}

static void
amdgpu_fini(void)
{

	drm_sysctl_fini(&amdgpu_def);
	amdgpu_amdkfd_fini();
#if notyet			/* XXX amdgpu acpi */
	amdgpu_unregister_atpx_handler();
#endif
	amdgpu_fence_slab_fini();
	amdgpu_sync_fini();

	ida_destroy(&amdgpu_pasid_ida);
	linux_mutex_destroy(&mgpu_info.mutex);
}

static int
amdgpu_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		/* XXX Kludge it up...  Must happen before attachment.  */
#ifdef _MODULE
		error = amdgpu_init();
#else
		error = amdgpu_guarantee_initialized();
#endif
		if (error) {
			aprint_error("amdgpu: failed to initialize: %d\n",
			    error);
			return error;
		}
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_amdgpu,
		    cfattach_ioconf_amdgpu, cfdata_ioconf_amdgpu);
		if (error) {
			aprint_error("amdgpu: failed to init component"
			    ": %d\n", error);
			amdgpu_fini();
			return error;
		}
#endif
		return 0;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_amdgpu,
		    cfattach_ioconf_amdgpu, cfdata_ioconf_amdgpu);
		if (error) {
			aprint_error("amdgpu: failed to fini component"
			    ": %d\n", error);
			return error;
		}
#endif
		amdgpu_fini();
		return 0;

	default:
		return ENOTTY;
	}
}
