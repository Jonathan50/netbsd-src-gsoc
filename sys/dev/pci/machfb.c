/*	$NetBSD: machfb.c,v 1.107 2022/09/25 17:52:25 thorpej Exp $	*/

/*
 * Copyright (c) 2002 Bang Jun-Young
 * Copyright (c) 2005, 2006, 2007 Michael Lorenz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Some code is derived from ATI Rage Pro and Derivatives Programmer's Guide.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0,
	"$NetBSD: machfb.c,v 1.107 2022/09/25 17:52:25 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/lwp.h>
#include <sys/kauth.h>

#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>
#include <dev/pci/machfbreg.h>

#include <dev/wscons/wsdisplayvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/pci/wsdisplay_pci.h>

#include <dev/wscons/wsdisplay_vconsvar.h>
#include <dev/wscons/wsdisplay_glyphcachevar.h>

#include "opt_wsemul.h"
#include "opt_machfb.h"
#include "opt_glyphcache.h"

#ifdef MACHFB_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

#define MACH64_REG_SIZE		0x800
#define MACH64_REG_OFF		0x7ff800

#define	NBARS		3	/* number of Mach64 PCI BARs */

struct vga_bar {
	bus_addr_t vb_base;
	bus_size_t vb_size;
	pcireg_t vb_type;
	int vb_flags;
};

struct mach64_softc {
	device_t sc_dev;
	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;

	struct vga_bar sc_bars[NBARS];
	struct vga_bar sc_rom;

#define sc_aperbase 	sc_bars[0].vb_base
#define sc_apersize	sc_bars[0].vb_size

#define sc_iobase	sc_bars[1].vb_base
#define sc_iosize	sc_bars[1].vb_size

#define sc_regbase	sc_bars[2].vb_base
#define sc_regsize	sc_bars[2].vb_size

	bus_space_tag_t sc_regt;
	bus_space_tag_t sc_memt;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_regh;
	bus_space_handle_t sc_memh;
#if 0
	void *sc_aperture;		/* mapped aperture vaddr */
	void *sc_registers;		/* mapped registers vaddr */
#endif
	uint32_t sc_nbus, sc_ndev, sc_nfunc;
	size_t memsize;
	int memtype;

	int sc_mode;
	int sc_bg;
	int sc_locked;

	int has_dsp;
	int bits_per_pixel;
	int max_x;
	int max_y;
	int virt_x;
	int virt_y;
	int stride;	/* in pixels */
	int color_depth;

	int mem_freq;
	int ramdac_freq;
	int ref_freq;
	int vclk_freq;

	int ref_div;
	int log2_vclk_post_div;
	int vclk_post_div;
	int vclk_fb_div;
	int mclk_post_div;
	int mclk_fb_div;
	int sc_clock;	/* which clock to use */
	int minref, m;

	struct videomode *sc_my_mode;
	int sc_edid_size;
	uint8_t sc_edid_data[1024];
    	struct edid_info sc_ei;
    	int sc_setmode;
    	int sc_gen_cntl;

	u_char sc_cmap_red[256];
	u_char sc_cmap_green[256];
	u_char sc_cmap_blue[256];
	int sc_dacw, sc_blanked, sc_console;
	struct vcons_data vd;
	struct wsdisplay_accessops sc_accessops;
	glyphcache sc_gc;
};

struct mach64_crtcregs {
	uint32_t h_total_disp;
	uint32_t h_sync_strt_wid;
	uint32_t v_total_disp;
	uint32_t v_sync_strt_wid;
	uint32_t gen_cntl;
	uint32_t clock_cntl;
	uint32_t color_depth;
	uint32_t dot_clock;
};

static struct {
	uint16_t chip_id;
	uint32_t ramdac_freq;
} const mach64_info[] = {
	{ PCI_PRODUCT_ATI_MACH64_GX, 135000 },
	{ PCI_PRODUCT_ATI_MACH64_CX, 135000 },
	{ PCI_PRODUCT_ATI_MACH64_CT, 135000 },
	{ PCI_PRODUCT_ATI_RAGE_PRO_AGP, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_PRO_AGP1X, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_PRO_PCI_B, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_XL_AGP, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_PRO_PCI_P, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_PRO_PCI_L, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_XL_PCI, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_XL_PCI66, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_II, 135000 },
	{ PCI_PRODUCT_ATI_RAGE_IIP, 200000 },
	{ PCI_PRODUCT_ATI_RAGE_IIC_PCI, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_IIC_AGP_B, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_IIC_AGP_P, 230000 },
#if 0
	{ PCI_PRODUCT_ATI_RAGE_MOB_M3_PCI, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_MOB_M3_AGP, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_MOBILITY, 230000 },
#endif
	{ PCI_PRODUCT_ATI_RAGE_L_MOB_M1_PCI, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_LT_PRO_AGP, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_LT_PRO, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_LT, 230000 },
	{ PCI_PRODUCT_ATI_RAGE_LT_PRO_PCI, 230000 },
	{ PCI_PRODUCT_ATI_MACH64_VT, 170000 },
	{ PCI_PRODUCT_ATI_MACH64_VTB, 200000 },
	{ PCI_PRODUCT_ATI_MACH64_VT4, 230000 }
};

static int mach64_chip_id, mach64_chip_rev;
static struct videomode default_mode = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };

static const char *mach64_gx_memtype_names[] = {
	"DRAM", "VRAM", "VRAM", "DRAM",
	"DRAM", "VRAM", "VRAM", "(unknown type)"
};

static const char *mach64_memtype_names[] = {
	"(N/A)", "DRAM", "EDO DRAM", "EDO DRAM", "SDRAM", "SGRAM", "WRAM",
	"(unknown type)"
};

extern const u_char rasops_cmap[768];

static int	mach64_match(device_t, cfdata_t, void *);
static void	mach64_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(machfb, sizeof(struct mach64_softc), mach64_match,
    mach64_attach, NULL, NULL);

static void	mach64_init(struct mach64_softc *);
static int	mach64_get_memsize(struct mach64_softc *);
static int	mach64_get_max_ramdac(struct mach64_softc *);
static int	mach64_ref_freq(void);

#ifdef MACHFB_DEBUG
static void	mach64_get_mode(struct mach64_softc *, struct videomode *);
static void	mach64_print_reg(struct mach64_softc *);
#endif

static int	mach64_calc_crtcregs(struct mach64_softc *,
				     struct mach64_crtcregs *,
				     struct videomode *);
static void	mach64_set_crtcregs(struct mach64_softc *,
				    struct mach64_crtcregs *);

static int	mach64_modeswitch(struct mach64_softc *, struct videomode *);
static void	mach64_set_dsp(struct mach64_softc *);
static void	mach64_set_pll(struct mach64_softc *, int);
static void	mach64_reset_engine(struct mach64_softc *);
static void	mach64_init_engine(struct mach64_softc *);
#if 0
static void	mach64_adjust_frame(struct mach64_softc *, int, int);
#endif
static void	mach64_init_lut(struct mach64_softc *);

static void	mach64_init_screen(void *, struct vcons_screen *, int, long *);
static int	mach64_is_console(struct mach64_softc *);

static void	mach64_cursor(void *, int, int, int);
#if 0
static int	mach64_mapchar(void *, int, u_int *);
#endif
static void	mach64_putchar_mono(void *, int, int, u_int, long);
static void	mach64_putchar_aa8(void *, int, int, u_int, long);
static void	mach64_copycols(void *, int, int, int, int);
static void	mach64_erasecols(void *, int, int, int, long);
static void	mach64_copyrows(void *, int, int, int);
static void	mach64_eraserows(void *, int, int, long);
static void 	mach64_clearscreen(struct mach64_softc *);

static int	mach64_putcmap(struct mach64_softc *, struct wsdisplay_cmap *);
static int	mach64_getcmap(struct mach64_softc *, struct wsdisplay_cmap *);
static int	mach64_putpalreg(struct mach64_softc *, uint8_t, uint8_t,
				 uint8_t, uint8_t);
static void	mach64_bitblt(void *, int, int, int, int, int, int, int);
static void	mach64_rectfill(struct mach64_softc *, int, int, int, int, int);
static void	mach64_setup_mono(struct mach64_softc *, int, int, int, int,
				  uint32_t, uint32_t);
static void	mach64_feed_bytes(struct mach64_softc *, int, uint8_t *);
#if 0
static void	mach64_showpal(struct mach64_softc *);
#endif

static void	machfb_blank(struct mach64_softc *, int);
static int	machfb_drm_print(void *, const char *);

static struct wsscreen_descr mach64_defaultscreen = {
	"default",
	80, 30,
	NULL,
	8, 16,
	WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_UNDERLINE
	 | WSSCREEN_RESIZE ,
	NULL
};

static const struct wsscreen_descr *_mach64_scrlist[] = {
	&mach64_defaultscreen,
};

static struct wsscreen_list mach64_screenlist = {
	__arraycount(_mach64_scrlist),
	_mach64_scrlist
};

static int	mach64_ioctl(void *, void *, u_long, void *, int,
		             struct lwp *);
static paddr_t	mach64_mmap(void *, void *, off_t, int);

static struct vcons_screen mach64_console_screen;

/*
 * Inline functions for getting access to register aperture.
 */

static inline uint32_t
regr(struct mach64_softc *sc, uint32_t index)
{
	return bus_space_read_4(sc->sc_regt, sc->sc_regh, index + 0x400);
}

static inline uint8_t
regrb(struct mach64_softc *sc, uint32_t index)
{
	return bus_space_read_1(sc->sc_regt, sc->sc_regh, index + 0x400);
}

static inline void
regw(struct mach64_softc *sc, uint32_t index, uint32_t data)
{
	bus_space_write_4(sc->sc_regt, sc->sc_regh, index + 0x400, data);
	bus_space_barrier(sc->sc_regt, sc->sc_regh, index, 4,
	    BUS_SPACE_BARRIER_WRITE);
}

static inline void
regws(struct mach64_softc *sc, uint32_t index, uint32_t data)
{
	bus_space_write_stream_4(sc->sc_regt, sc->sc_regh, index + 0x400, data);
	bus_space_barrier(sc->sc_regt, sc->sc_regh, index + 0x400, 4,
	    BUS_SPACE_BARRIER_WRITE);
}

static inline void
regwb(struct mach64_softc *sc, uint32_t index, uint8_t data)
{
	bus_space_write_1(sc->sc_regt, sc->sc_regh, index + 0x400, data);
	bus_space_barrier(sc->sc_regt, sc->sc_regh, index + 0x400, 1,
	    BUS_SPACE_BARRIER_WRITE);
}

static inline void
regwb_pll(struct mach64_softc *sc, uint32_t index, uint8_t data)
{
	uint32_t reg;

	reg = regr(sc, CLOCK_CNTL);
	reg |= PLL_WR_EN;
	regw(sc, CLOCK_CNTL, reg);
	reg &= ~(PLL_ADDR | PLL_DATA);
	reg |= (index & 0x3f) << PLL_ADDR_SHIFT;
	reg |= data << PLL_DATA_SHIFT;
	reg |= CLOCK_STROBE;
	regw(sc, CLOCK_CNTL, reg);
	reg &= ~PLL_WR_EN;
	regw(sc, CLOCK_CNTL, reg);
}

static inline uint8_t
regrb_pll(struct mach64_softc *sc, uint32_t index)
{

	regwb(sc, CLOCK_CNTL + 1, index << 2);
	return regrb(sc, CLOCK_CNTL + 2);
}

static inline void
wait_for_fifo(struct mach64_softc *sc, uint8_t v)
{
	while ((regr(sc, FIFO_STAT) & 0xffff) > (0x8000 >> v))
		continue;
}

static inline void
wait_for_idle(struct mach64_softc *sc)
{
	wait_for_fifo(sc, 16);
	while ((regr(sc, GUI_STAT) & 1) != 0)
		continue;
}

static int
mach64_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	int i;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY ||
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_DISPLAY_VGA)
		return 0;

	for (i = 0; i < __arraycount(mach64_info); i++)
		if (PCI_PRODUCT(pa->pa_id) == mach64_info[i].chip_id) {
			mach64_chip_id = PCI_PRODUCT(pa->pa_id);
			mach64_chip_rev = PCI_REVISION(pa->pa_class);
			return 100;
		}

	return 0;
}

static void
mach64_attach(device_t parent, device_t self, void *aux)
{
	struct mach64_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	struct rasops_info *ri;
	const char *mptr = NULL;
	prop_data_t edid_data;
	const struct videomode *mode = NULL;
	int bar, id, expected_id;
	int is_gx;
	const char **memtype_names;
	struct wsemuldisplaydev_attach_args aa;
	long defattr;
	int width = 1024, height = 768;
	pcireg_t screg;
	uint32_t reg;
	const pcireg_t enables = PCI_COMMAND_MEM_ENABLE;
	int use_mmio = FALSE;

	sc->sc_dev = self;
	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_dacw = -1;
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_nbus = pa->pa_bus;
	sc->sc_ndev = pa->pa_device;
	sc->sc_nfunc = pa->pa_function;
	sc->sc_locked = 0;
	sc->sc_iot = pa->pa_iot;
	sc->sc_accessops.ioctl = mach64_ioctl;
	sc->sc_accessops.mmap = mach64_mmap;
	sc->sc_setmode = 0;

	pci_aprint_devinfo(pa, "Graphics processor");
#ifdef MACHFB_DEBUG
	printf(prop_dictionary_externalize(device_properties(self)));
#endif

	/* enable memory access */
	screg = pci_conf_read(sc->sc_pc, sc->sc_pcitag, PCI_COMMAND_STATUS_REG);
	if ((screg & enables) != enables) {
		screg |= enables;
		pci_conf_write(sc->sc_pc, sc->sc_pcitag,
		    PCI_COMMAND_STATUS_REG, screg);
	}
	for (bar = 0; bar < NBARS; bar++) {
		reg = PCI_MAPREG_START + (bar * 4);
		sc->sc_bars[bar].vb_type = pci_mapreg_type(sc->sc_pc,
		    sc->sc_pcitag, reg);
		(void)pci_mapreg_info(sc->sc_pc, sc->sc_pcitag, reg,
		    sc->sc_bars[bar].vb_type, &sc->sc_bars[bar].vb_base,
		    &sc->sc_bars[bar].vb_size, &sc->sc_bars[bar].vb_flags);
	}
	aprint_debug_dev(sc->sc_dev, "aperture size %08x\n",
	    (uint32_t)sc->sc_apersize);

	sc->sc_rom.vb_type = PCI_MAPREG_TYPE_ROM;
	pci_mapreg_info(sc->sc_pc, sc->sc_pcitag, PCI_MAPREG_ROM,
		    sc->sc_rom.vb_type, &sc->sc_rom.vb_base,
		    &sc->sc_rom.vb_size, &sc->sc_rom.vb_flags);
	sc->sc_memt = pa->pa_memt;

	/* use MMIO register aperture if available */
	if ((sc->sc_regbase != 0) && (sc->sc_regbase != 0xffffffff)) {
		if (pci_mapreg_map(pa, MACH64_BAR_MMIO,  PCI_MAPREG_TYPE_MEM,
		    0, &sc->sc_regt, &sc->sc_regh, &sc->sc_regbase,
		    &sc->sc_regsize) == 0) {

			/*
			 * the MMIO aperture maps both 1KB register blocks, but
			 * all register offsets are relative to the 2nd one so
			 * for now fix this up in MACH64_REG_OFF and the access
			 * functions
			 */
			aprint_normal_dev(sc->sc_dev, "using MMIO aperture\n");
			use_mmio = TRUE;
		}
	}
	if (!use_mmio) {
		if (bus_space_map(sc->sc_memt, sc->sc_aperbase,
		    sc->sc_apersize, BUS_SPACE_MAP_LINEAR, &sc->sc_memh)) {
			panic("%s: failed to map aperture",
			    device_xname(sc->sc_dev));
		}

		sc->sc_regt = sc->sc_memt;
		bus_space_subregion(sc->sc_regt, sc->sc_memh, MACH64_REG_OFF,
		    MACH64_REG_SIZE, &sc->sc_regh);
	}

	mach64_init(sc);

	aprint_normal_dev(sc->sc_dev,
	    "%d MB aperture at 0x%08x, %d KB registers at 0x%08x\n",
	    (u_int)(sc->sc_apersize / (1024 * 1024)),
	    (u_int)sc->sc_aperbase, (u_int)(sc->sc_regsize / 1024),
	    (u_int)sc->sc_regbase);

	printf("%s: %d KB ROM at 0x%08x\n", device_xname(sc->sc_dev),
	    (int)sc->sc_rom.vb_size >> 10, (uint32_t)sc->sc_rom.vb_base);
#ifdef MACHFB_DEBUG
	mach64_get_mode(sc, NULL);
	mach64_print_reg(sc);
#endif

	prop_dictionary_get_uint32(device_properties(self), "width", &width);
	prop_dictionary_get_uint32(device_properties(self), "height", &height);

	default_mode.hdisplay = width;
	default_mode.vdisplay = height;

	prop_dictionary_get_string(device_properties(sc->sc_dev),
	    "videomode", &mptr);

	memset(&sc->sc_ei, 0, sizeof(sc->sc_ei));
	if (mptr == NULL &&
	    (edid_data = prop_dictionary_get(device_properties(self), "EDID"))
	    != NULL) {

		sc->sc_edid_size = uimin(1024, prop_data_size(edid_data));
		memset(sc->sc_edid_data, 0, sizeof(sc->sc_edid_data));
		memcpy(sc->sc_edid_data, prop_data_value(edid_data),
		    sc->sc_edid_size);

		edid_parse(sc->sc_edid_data, &sc->sc_ei);

#ifdef MACHFB_DEBUG
		edid_print(&sc->sc_ei);
#endif
	}
	is_gx = 0;
	switch(mach64_chip_id) {
		case PCI_PRODUCT_ATI_MACH64_GX:
		case PCI_PRODUCT_ATI_MACH64_CX:
			is_gx = 1;
			/* FALLTHROUGH */
		case PCI_PRODUCT_ATI_MACH64_CT:
			sc->has_dsp = 0;
			break;
		case PCI_PRODUCT_ATI_MACH64_VT:
		case PCI_PRODUCT_ATI_RAGE_II:
			if((mach64_chip_rev & 0x07) == 0) {
				sc->has_dsp = 0;
				break;
			}
			/* FALLTHROUGH */
		default:
			sc->has_dsp = 1;
	}

	memtype_names = is_gx ? mach64_gx_memtype_names : mach64_memtype_names;

	sc->memsize = mach64_get_memsize(sc);

	if(is_gx)
		sc->memtype = (regr(sc, CONFIG_STAT0) >> 3) & 0x07;
	else
		sc->memtype = regr(sc, CONFIG_STAT0) & 0x07;

	sc->ref_freq = mach64_ref_freq();

	reg = regr(sc, CLOCK_CNTL);
	sc->sc_clock = reg & 3;
	DPRINTF("using clock %d\n", sc->sc_clock);

	DPRINTF("ref_freq: %d\n", sc->ref_freq);
	sc->ref_div = regrb_pll(sc, PLL_REF_DIV);
	DPRINTF("ref_div: %d\n", sc->ref_div);
	sc->mclk_fb_div = regrb_pll(sc, MCLK_FB_DIV);
	DPRINTF("mclk_fb_div: %d\n", sc->mclk_fb_div);
	sc->mem_freq = (2 * sc->ref_freq * sc->mclk_fb_div) /
	    (sc->ref_div * 2);
	sc->mclk_post_div = (sc->mclk_fb_div * 2 * sc->ref_freq) /
	    (sc->mem_freq * sc->ref_div);
	sc->ramdac_freq = mach64_get_max_ramdac(sc);
	{
		sc->minref = sc->ramdac_freq / 510;
		sc->m = sc->ref_freq / sc->minref;
		DPRINTF("minref: %d m: %d\n", sc->minref, sc->m);
	}
	aprint_normal_dev(sc->sc_dev,
	    "%ld KB %s %d.%d MHz, maximum RAMDAC clock %d MHz\n",
	    (u_long)sc->memsize,
	    memtype_names[sc->memtype],
	    sc->mem_freq / 1000, sc->mem_freq % 1000,
	    sc->ramdac_freq / 1000);

	id = regr(sc, CONFIG_CHIP_ID) & 0xffff;
	switch(mach64_chip_id) {
		case PCI_PRODUCT_ATI_MACH64_GX:
			expected_id = 0x00d7;
			break;
		case PCI_PRODUCT_ATI_MACH64_CX:
			expected_id = 0x0057;
			break;
		default:
			/* Most chip IDs match their PCI product ID. */
			expected_id = mach64_chip_id;
	}

	if (id != expected_id) {
		aprint_error_dev(sc->sc_dev,
		    "chip ID mismatch, 0x%x != 0x%x\n", id, expected_id);
		return;
	}

	sc->sc_console = mach64_is_console(sc);
	sc->sc_gen_cntl = regr(sc, CRTC_GEN_CNTL);
	aprint_debug("gen_cntl: %08x\n", sc->sc_gen_cntl);
	sc->sc_gen_cntl &= CRTC_CSYNC_EN;
	aprint_normal_dev(sc->sc_dev, "found composite sync %s\n",
	    sc->sc_gen_cntl ? "enabled" : "disabled");

#define MODE_IS_VALID(m) ((sc->ramdac_freq >= (m)->dot_clock) && \
			  ((m)->hdisplay <= 1280))

	/* no mode setting support on ancient chips with external clocks */
	sc->sc_setmode = 0;
	if (!is_gx) {
		/*
		 * Now pick a mode.
		 */
		if ((sc->sc_ei.edid_preferred_mode != NULL)) {
			struct videomode *m = sc->sc_ei.edid_preferred_mode;
			if (MODE_IS_VALID(m)) {
				memcpy(&default_mode, m,
				    sizeof(struct videomode));
				sc->sc_setmode = 1;
			} else {
				aprint_normal_dev(sc->sc_dev,
				    "unable to use EDID preferred mode "
				    "(%d x %d)\n", m->hdisplay, m->vdisplay);
			}
		}
		/*
		 * if we can't use the preferred mode go look for the
		 * best one we can support
		 */
		if (sc->sc_setmode == 0) {
			struct videomode *m = sc->sc_ei.edid_modes;

			mode = NULL;
			sort_modes(sc->sc_ei.edid_modes,
			    &sc->sc_ei.edid_preferred_mode,
			    sc->sc_ei.edid_nmodes);
			for (int n = 0; n < sc->sc_ei.edid_nmodes; n++)
				if (MODE_IS_VALID(&m[n])) {
					mode = &m[n];
					break;
				}
			if (mode != NULL) {
				memcpy(&default_mode, mode,
				    sizeof(struct videomode));
				sc->sc_setmode = 1;
			}
		}
	}

	/* make sure my_mode points at something sensible if the above fails */
	if (default_mode.dot_clock == 0) {
		sc->sc_setmode = 0;
		mode = pick_mode_by_ref(width, height, 60);
		if (mode != NULL) {	
			memcpy(&default_mode, mode, sizeof(default_mode));
		} else if ((width > 0) && (height > 0)) {
			default_mode.hdisplay = width;
			default_mode.vdisplay = height;
		} else {
			/*
			 * if we end up here we're probably dealing with
			 * uninitialized hardware - try to set 1024x768@60 and
			 * hope for the best...
			 */ 
			mode = pick_mode_by_ref(1024, 768, 60);
			if (mode == NULL) return; 
			memcpy(&default_mode, mode, sizeof(default_mode));
			if (!is_gx) sc->sc_setmode = 1;
		}			
	}

	sc->sc_my_mode = &default_mode;

	if ((width == sc->sc_my_mode->hdisplay) &&
	    (height == sc->sc_my_mode->vdisplay))
		sc->sc_setmode = 0;

	sc->bits_per_pixel = 8;
	sc->virt_x = sc->sc_my_mode->hdisplay;
	sc->virt_y = sc->sc_my_mode->vdisplay;
	sc->stride = (sc->virt_x + 7) & ~7;	/* hw needs multiples of 8 */
	sc->max_x = sc->virt_x - 1;
	sc->max_y = (sc->memsize * 1024) /
	    (sc->stride * (sc->bits_per_pixel / 8)) - 1;

	sc->color_depth = CRTC_PIX_WIDTH_8BPP;

	mach64_init_engine(sc);

	if (sc->sc_setmode)
		mach64_modeswitch(sc, sc->sc_my_mode);

	aprint_normal_dev(sc->sc_dev,
	    "initial resolution %dx%d at %d bpp\n",
	    sc->sc_my_mode->hdisplay, sc->sc_my_mode->vdisplay,
	    sc->bits_per_pixel);

	wsfont_init();

#ifdef GLYPHCACHE_DEBUG
	/* shrink the screen so we can see part of the glyph cache */
	sc->sc_my_mode->vdisplay -= 200;
#endif

	vcons_init(&sc->vd, sc, &mach64_defaultscreen, &sc->sc_accessops);
	sc->vd.init_screen = mach64_init_screen;
	sc->vd.show_screen_cookie = &sc->sc_gc;
	sc->vd.show_screen_cb = glyphcache_adapt;

	sc->sc_gc.gc_bitblt = mach64_bitblt;
	sc->sc_gc.gc_blitcookie = sc;
	sc->sc_gc.gc_rop = MIX_SRC;

	ri = &mach64_console_screen.scr_ri;
	if (sc->sc_console) {

		vcons_init_screen(&sc->vd, &mach64_console_screen, 1,
		    &defattr);
		mach64_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;

		mach64_defaultscreen.textops = &ri->ri_ops;
		mach64_defaultscreen.capabilities = ri->ri_caps;
		mach64_defaultscreen.nrows = ri->ri_rows;
		mach64_defaultscreen.ncols = ri->ri_cols;
		glyphcache_init(&sc->sc_gc, sc->sc_my_mode->vdisplay + 5,
		    ((sc->memsize * 1024) / sc->stride) -
		      sc->sc_my_mode->vdisplay - 5,
		    sc->stride,
		    ri->ri_font->fontwidth,
		    ri->ri_font->fontheight,
		    defattr);
		wsdisplay_cnattach(&mach64_defaultscreen, ri, 0, 0, defattr);
	} else {
		/*
		 * since we're not the console we can postpone the rest
		 * until someone actually allocates a screen for us
		 */
		if (mach64_console_screen.scr_ri.ri_rows == 0) {
			/* do some minimal setup to avoid weirdnesses later */
			vcons_init_screen(&sc->vd, &mach64_console_screen, 1,
			    &defattr);
		} else
			(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);

		glyphcache_init(&sc->sc_gc, sc->sc_my_mode->vdisplay + 5,
		    ((sc->memsize * 1024) / sc->stride) -
		      sc->sc_my_mode->vdisplay - 5,
		    sc->stride,
		    ri->ri_font->fontwidth,
		    ri->ri_font->fontheight,
		    defattr);
	}

	sc->sc_bg = mach64_console_screen.scr_ri.ri_devcmap[WS_DEFAULT_BG];
	mach64_clearscreen(sc);
	mach64_init_lut(sc);

	if (sc->sc_console)
		vcons_replay_msgbuf(&mach64_console_screen);

	machfb_blank(sc, 0);	/* unblank the screen */

	aa.console = sc->sc_console;
	aa.scrdata = &mach64_screenlist;
	aa.accessops = &sc->sc_accessops;
	aa.accesscookie = &sc->vd;

	config_found(self, &aa, wsemuldisplaydevprint,
	    CFARGS(.iattr = "wsemuldisplaydev"));
#if 0
	/* XXX
	 * turns out some firmware doesn't turn these back on when needed
	 * so we need to turn them off only when mapping vram in
	 * WSDISPLAYIO_MODE_DUMB would overlap ( unlikely but far from
	 * impossible )
	 */
	if (use_mmio) {
		/*
		 * Now that we took over, turn off the aperture registers if we
		 * don't use them. Can't do this earlier since on some hardware
		 * we use firmware calls as early console output which may in
		 * turn try to access these registers.
		 */
		reg = regr(sc, BUS_CNTL);
		aprint_debug_dev(sc->sc_dev, "BUS_CNTL: %08x\n", reg);
		reg |= BUS_APER_REG_DIS;
		regw(sc, BUS_CNTL, reg);
	}
#endif
	config_found(self, aux, machfb_drm_print,
	    CFARGS(.iattr = "drm"));
}

static int
machfb_drm_print(void *aux, const char *pnp)
{
	if (pnp)
		aprint_normal("direct rendering for %s", pnp);
	return (UNSUPP);
}

static void
mach64_init_screen(void *cookie, struct vcons_screen *scr, int existing,
    long *defattr)
{
	struct mach64_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = sc->bits_per_pixel;
	ri->ri_width = sc->sc_my_mode->hdisplay;
	ri->ri_height = sc->sc_my_mode->vdisplay;
	ri->ri_stride = sc->stride;
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR;
	if (ri->ri_depth == 8)
		ri->ri_flg |= RI_8BIT_IS_RGB | RI_ENABLE_ALPHA |
			      RI_PREFER_ALPHA;

#ifdef VCONS_DRAW_INTR
	scr->scr_flags |= VCONS_DONT_READ;
#endif
	scr->scr_flags |= VCONS_LOADFONT;

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_UNDERLINE |
		      WSSCREEN_RESIZE;
	rasops_reconfig(ri, sc->sc_my_mode->vdisplay / ri->ri_font->fontheight,
		    sc->sc_my_mode->hdisplay / ri->ri_font->fontwidth);

	/* enable acceleration */
	ri->ri_hw = scr;
	ri->ri_ops.copyrows = mach64_copyrows;
	ri->ri_ops.copycols = mach64_copycols;
	ri->ri_ops.eraserows = mach64_eraserows;
	ri->ri_ops.erasecols = mach64_erasecols;
	ri->ri_ops.cursor = mach64_cursor;
	if (FONT_IS_ALPHA(ri->ri_font)) {
		ri->ri_ops.putchar = mach64_putchar_aa8;
	} else
		ri->ri_ops.putchar = mach64_putchar_mono;
}

static void
mach64_init(struct mach64_softc *sc)
{
	sc->sc_blanked = 0;
}

static int
mach64_get_memsize(struct mach64_softc *sc)
{
	int tmp, memsize;
	int mem_tab[] = {
		512, 1024, 2048, 4096, 6144, 8192, 12288, 16384
	};
	tmp = regr(sc, MEM_CNTL);
#ifdef DIAGNOSTIC
	aprint_debug_dev(sc->sc_dev, "memctl %08x\n", tmp);
#endif
	if (sc->has_dsp) {
		tmp &= 0x0000000f;
		if (tmp < 8)
			memsize = (tmp + 1) * 512;
		else if (tmp < 12)
			memsize = (tmp - 3) * 1024;
		else
			memsize = (tmp - 7) * 2048;
	} else {
		memsize = mem_tab[tmp & 0x07];
	}

	return memsize;
}

static int
mach64_get_max_ramdac(struct mach64_softc *sc)
{
	int i;

	if ((mach64_chip_id == PCI_PRODUCT_ATI_MACH64_VT ||
	     mach64_chip_id == PCI_PRODUCT_ATI_RAGE_II) &&
	     (mach64_chip_rev & 0x07))
		return 170000;

	for (i = 0; i < __arraycount(mach64_info); i++)
		if (mach64_chip_id == mach64_info[i].chip_id)
			return mach64_info[i].ramdac_freq;

	if (sc->bits_per_pixel == 8)
		return 135000;
	else
		return 80000;
}

static int
mach64_ref_freq(void)
{
	/*
	 * There doesn't seem to be any way to calculate the reference
	 * frequency from known values
	 */
	if ((mach64_chip_id == PCI_PRODUCT_ATI_RAGE_XL_PCI) ||
	    ((mach64_chip_id >= PCI_PRODUCT_ATI_RAGE_LT_PRO_PCI) &&
	    (mach64_chip_id <= PCI_PRODUCT_ATI_RAGE_L_MOB_M1_PCI)))
		return 29498;
	else
		return 14318;
}

#ifdef MACHFB_DEBUG
static void
mach64_get_mode(struct mach64_softc *sc, struct videomode *mode)
{
	int htotal, hdisplay, hsync_start, hsync_end;
	int vtotal, vdisplay, vsync_start, vsync_end;
	int clk_ctl, clock;
	int ref_freq, ref_div, vclk_post_div, vclk_fb_div;
	int nhsync, nvsync;
	int post_div, dot_clock, vrefresh, vrefresh2;

	hdisplay = regr(sc, CRTC_H_TOTAL_DISP);
	hsync_end = regr(sc, CRTC_H_SYNC_STRT_WID);
	vdisplay = regr(sc, CRTC_V_TOTAL_DISP);
	vsync_end = regr(sc, CRTC_V_SYNC_STRT_WID);
	clk_ctl = regr(sc, CLOCK_CNTL);
	clock = clk_ctl & 3;
	ref_div = regrb_pll(sc, PLL_REF_DIV);
	vclk_post_div = regrb_pll(sc, VCLK_POST_DIV);
	vclk_fb_div = regrb_pll(sc, VCLK0_FB_DIV + clock);
	ref_freq = mach64_ref_freq();

	htotal = ((hdisplay & 0x01ff) + 1) << 3;
	hdisplay = (((hdisplay & 0x1ff0000) >> 16) + 1) << 3;
	if (hsync_end & CRTC_HSYNC_NEG)
		nhsync = 1;
	else
		nhsync = 0;
	hsync_start = (((hsync_end & 0xff) + 1) << 3) +
	    ((hsync_end & 0x700) >> 8);
	hsync_end = (((hsync_end & 0x1f0000) >> 16) << 3) + hsync_start;

	vtotal = (vdisplay & 0x07ff) + 1;
	vdisplay = ((vdisplay & 0x7ff0000) >> 16) + 1;
	if (vsync_end & CRTC_VSYNC_NEG)
		nvsync = 1;
	else
		nvsync = 0;
	vsync_start = (vsync_end & 0x07ff) + 1;
	vsync_end = ((vsync_end & 0x1f0000) >> 16) + vsync_start;

	switch ((vclk_post_div >> (clock * 2)) & 3) {
		case 3:
			post_div = 8;
			break;
		case 2:
			post_div = 4;
			break;
		case 1:
			post_div = 2;
			break;
		default:
			post_div = 1;
			break;
	}
	dot_clock = (2 * ref_freq * vclk_fb_div) / (ref_div * post_div);
	vrefresh = (dot_clock * 1000) / (htotal * vtotal);
	vrefresh2 = ((dot_clock * 1000) - (vrefresh * htotal * vtotal)) * 100 /
	    (htotal * vtotal);

	aprint_normal_dev(sc->sc_dev, "Video mode:\n");
	aprint_normal("\t%d" "x%d @ %d.%02dHz "
	    "(%d %d %d %d %d %d %d %cH %cV)\n",
	    hdisplay, vdisplay, vrefresh, vrefresh2, dot_clock,
	    hsync_start, hsync_end, htotal, vsync_start, vsync_end, vtotal,
	    nhsync == 1 ? '-' : '+', nvsync == 1 ? '-' : '+');

	if (mode != NULL) {
		mode->dot_clock = dot_clock;
		mode->htotal = htotal;
		mode->hdisplay = hdisplay;
		mode->hsync_start = hsync_start;
		mode->hsync_end = hsync_end;
		mode->vtotal = vtotal;
		mode->vdisplay = vdisplay;
		mode->vsync_start = vsync_start;
		mode->vsync_end = vsync_end;
		mode->flags = 0;
		if (nhsync)
			mode->flags |= VID_NHSYNC;
		if (nvsync)
			mode->flags |= VID_NVSYNC;
	}
}

static void
mach64_print_reg(struct mach64_softc *sc)
{
	struct reglist {
		int offset;
		const char *name;
	};
	static const struct reglist reglist_tab[] = {
		{ 0x0000, "CRTC_H_TOTAL_DISP" },
		{ 0x0004, "CRTC_H_SYNC_STRT_WID" },
		{ 0x0008, "CRTC_V_TOTAL_DISP" },
		{ 0x000C, "CRTC_V_SYNC_STRT_WID" },
		{ 0x0010, "CRTC_VLINE_CRNT_VLINE" },
		{ 0x0014, "CRTC_OFF_PITCH" },
		{ 0x001C, "CRTC_GEN_CNTL" },
		{ 0x0090, "CLOCK_CNTL" },
		{ 0, NULL }
	};
	static const struct reglist plllist_tab[] = {
		{ 0x02, "PLL_REF_DIV" },
		{ 0x03, "PLL_GEN_CNTL" },
		{ 0x04, "MCLK_FB_DIV" },
		{ 0x05, "PLL_VCLK_CNTL" },
		{ 0x06, "VCLK_POST_DIV" },
		{ 0x07, "VCLK0_FB_DIV" },
		{ 0x08, "VCLK1_FB_DIV" },
		{ 0x09, "VCLK2_FB_DIV" },
		{ 0x0A, "VCLK3_FB_DIV" },
		{ 0x0B, "PLL_XCLK_CNTL" },
		{ 0x10, "LVDSPLL_CNTL0" },
		{ 0x11, "LVDSPLL_CNTL0" },
		{ 0x19, "EXT_VPLL_CNTL" },
		{ 0x1A, "EXT_VPLL_REF_DIV" },
		{ 0x1B, "EXT_VPLL_FB_DIV" },
		{ 0x1C, "EXT_VPLL_MSB" },
		{ 0, NULL }
	};
	const struct reglist *r;

	aprint_normal("CRTC registers\n");
	for (r = reglist_tab; r->name != NULL; r++)
		aprint_normal("0x%04x 0x%08x %s\n", r->offset,
		    regr(sc, r->offset), r->name);
	aprint_normal("PLL registers\n");
	for (r = plllist_tab; r->name != NULL; r++)
		aprint_normal("0x%02x 0x%02x %s\n", r->offset,
		    regrb_pll(sc, r->offset), r->name);
}
#endif

static int
mach64_calc_crtcregs(struct mach64_softc *sc, struct mach64_crtcregs *crtc,
    struct videomode *mode)
{

	if (mode->dot_clock > sc->ramdac_freq)
		/* Clock too high. */
		return 1;

	crtc->h_total_disp = (((mode->hdisplay >> 3) - 1) << 16) |
	    ((mode->htotal >> 3) - 1);
	crtc->h_sync_strt_wid =
	    (((mode->hsync_end - mode->hsync_start) >> 3) << 16) |
	    ((mode->hsync_start >> 3) - 1) | ((mode->hsync_start & 7) << 8);

	crtc->v_total_disp = ((mode->vdisplay - 1) << 16) |
	    (mode->vtotal - 1);
	crtc->v_sync_strt_wid =
	    ((mode->vsync_end - mode->vsync_start) << 16) |
	    (mode->vsync_start - 1);

	if (mode->flags & VID_NVSYNC)
		crtc->v_sync_strt_wid |= CRTC_VSYNC_NEG;

	switch (sc->bits_per_pixel) {
	case 8:
		crtc->color_depth = CRTC_PIX_WIDTH_8BPP;
		break;
	case 16:
		crtc->color_depth = CRTC_PIX_WIDTH_16BPP;
		break;
	case 32:
		crtc->color_depth = CRTC_PIX_WIDTH_32BPP;
		break;
	}

	crtc->gen_cntl = 0;
	if (mode->flags & VID_INTERLACE)
		crtc->gen_cntl |= CRTC_INTERLACE_EN;

	if (mode->flags & VID_CSYNC)
		crtc->gen_cntl |= CRTC_CSYNC_EN;

	crtc->dot_clock = mode->dot_clock;

	return 0;
}

static void
mach64_set_crtcregs(struct mach64_softc *sc, struct mach64_crtcregs *crtc)
{

	mach64_set_pll(sc, crtc->dot_clock);

	if (sc->has_dsp)
		mach64_set_dsp(sc);

	DPRINTF("\th total: 0x%08x  h sync: 0x%08x\n",
	    crtc->h_total_disp, crtc->h_sync_strt_wid);
	DPRINTF("\tv total: 0x%08x  v sync: 0x%08x\n",
	    crtc->v_total_disp, crtc->v_sync_strt_wid);
	regw(sc, CRTC_H_TOTAL_DISP, crtc->h_total_disp);
	regw(sc, CRTC_H_SYNC_STRT_WID, crtc->h_sync_strt_wid);
	regw(sc, CRTC_V_TOTAL_DISP, crtc->v_total_disp);
	regw(sc, CRTC_V_SYNC_STRT_WID, crtc->v_sync_strt_wid);

	regw(sc, CRTC_VLINE_CRNT_VLINE, 0);

	regw(sc, CRTC_OFF_PITCH, (sc->stride >> 3) << 22);

	regw(sc, CRTC_GEN_CNTL, crtc->gen_cntl | crtc->color_depth |
	    sc->sc_gen_cntl | CRTC_EXT_DISP_EN | CRTC_EXT_EN);
}

static int
mach64_modeswitch(struct mach64_softc *sc, struct videomode *mode)
{
	struct mach64_crtcregs crtc;

	memset(&crtc, 0, sizeof crtc);	/* XXX gcc */

	if (mach64_calc_crtcregs(sc, &crtc, mode))
		return 1;
	aprint_debug("crtc dot clock: %d\n", crtc.dot_clock);
	if (crtc.dot_clock == 0) {
		aprint_error("%s: preposterous dot clock (%d)\n",
		    device_xname(sc->sc_dev), crtc.dot_clock);
		return 1;
	}
	mach64_set_crtcregs(sc, &crtc);
	return 0;
}

static void
mach64_reset_engine(struct mach64_softc *sc)
{

	/* Reset engine.*/
	regw(sc, GEN_TEST_CNTL, regr(sc, GEN_TEST_CNTL) & ~GUI_ENGINE_ENABLE);

	/* Enable engine. */
	regw(sc, GEN_TEST_CNTL, regr(sc, GEN_TEST_CNTL) | GUI_ENGINE_ENABLE);

	/* Ensure engine is not locked up by clearing any FIFO or
	   host errors. */
	regw(sc, BUS_CNTL, regr(sc, BUS_CNTL) | BUS_HOST_ERR_ACK |
	    BUS_FIFO_ERR_ACK);
}

static void
mach64_init_engine(struct mach64_softc *sc)
{
	uint32_t pitch_value;

	pitch_value = sc->stride;

	if (sc->bits_per_pixel == 24)
		pitch_value *= 3;

	mach64_reset_engine(sc);

	wait_for_fifo(sc, 14);

	regw(sc, CONTEXT_MASK, 0xffffffff);

	regw(sc, DST_OFF_PITCH, (pitch_value >> 3) << 22);

	/* make sure the visible area starts where we're going to draw */
	regw(sc, CRTC_OFF_PITCH, (sc->stride >> 3) << 22);

	regw(sc, DST_Y_X, 0);
	regw(sc, DST_HEIGHT, 0);
	regw(sc, DST_BRES_ERR, 0);
	regw(sc, DST_BRES_INC, 0);
	regw(sc, DST_BRES_DEC, 0);

	regw(sc, DST_CNTL, DST_LAST_PEL | DST_X_LEFT_TO_RIGHT |
	    DST_Y_TOP_TO_BOTTOM);

	regw(sc, SRC_OFF_PITCH, (pitch_value >> 3) << 22);

	regw(sc, SRC_Y_X, 0);
	regw(sc, SRC_HEIGHT1_WIDTH1, 1);
	regw(sc, SRC_Y_X_START, 0);
	regw(sc, SRC_HEIGHT2_WIDTH2, 1);

	regw(sc, SRC_CNTL, SRC_LINE_X_LEFT_TO_RIGHT);

	wait_for_fifo(sc, 13);
	regw(sc, HOST_CNTL, 0);

	regw(sc, PAT_REG0, 0);
	regw(sc, PAT_REG1, 0);
	regw(sc, PAT_CNTL, 0);

	regw(sc, SC_LEFT, 0);
	regw(sc, SC_TOP, 0);
	regw(sc, SC_BOTTOM, 0x3fff);
	regw(sc, SC_RIGHT, pitch_value - 1);

	regw(sc, DP_BKGD_CLR, WS_DEFAULT_BG);
	regw(sc, DP_FRGD_CLR, WS_DEFAULT_FG);
	regw(sc, DP_WRITE_MASK, 0xffffffff);
	regw(sc, DP_MIX, (MIX_SRC << 16) | MIX_DST);

	regw(sc, DP_SRC, FRGD_SRC_FRGD_CLR);

	wait_for_fifo(sc, 3);
	regw(sc, CLR_CMP_CLR, 0);
	regw(sc, CLR_CMP_MASK, 0xffffffff);
	regw(sc, CLR_CMP_CNTL, 0);

	wait_for_fifo(sc, 3);
	switch (sc->bits_per_pixel) {
	case 8:
		regw(sc, DP_PIX_WIDTH, HOST_1BPP | SRC_8BPP | DST_8BPP);
		regw(sc, DP_CHAIN_MASK, DP_CHAIN_8BPP);
		/* We want 8 bit per channel */
		regw(sc, DAC_CNTL, regr(sc, DAC_CNTL) | DAC_8BIT_EN);
		break;
	case 32:
		regw(sc, DP_PIX_WIDTH, HOST_1BPP | SRC_32BPP | DST_32BPP);
		regw(sc, DP_CHAIN_MASK, DP_CHAIN_32BPP);
		regw(sc, DAC_CNTL, regr(sc, DAC_CNTL) | DAC_8BIT_EN);
		break;
	}
	regw(sc, DP_WRITE_MASK, 0xff);

	wait_for_fifo(sc, 5);
	regw(sc, CRTC_INT_CNTL, regr(sc, CRTC_INT_CNTL) & ~0x20);
	regw(sc, GUI_TRAJ_CNTL, DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM);

	wait_for_idle(sc);
}

#if 0
static void
mach64_adjust_frame(struct mach64_softc *sc, int x, int y)
{
	int offset;

	offset = ((x + y * sc->stride) * (sc->bits_per_pixel >> 3)) >> 3;

	regw(sc, CRTC_OFF_PITCH, (regr(sc, CRTC_OFF_PITCH) & 0xfff00000) |
	     offset);
}
#endif

static void
mach64_set_dsp(struct mach64_softc *sc)
{
	uint32_t fifo_depth, page_size, dsp_precision, dsp_loop_latency;
	uint32_t dsp_off, dsp_on, dsp_xclks_per_qw;
	uint32_t xclks_per_qw, xclks_per_qw_m, y;
	uint32_t fifo_off, fifo_on;

	aprint_normal_dev(sc->sc_dev, "initializing the DSP\n");

	if (mach64_chip_id == PCI_PRODUCT_ATI_MACH64_VT ||
	    mach64_chip_id == PCI_PRODUCT_ATI_RAGE_II ||
	    mach64_chip_id == PCI_PRODUCT_ATI_RAGE_IIP ||
	    mach64_chip_id == PCI_PRODUCT_ATI_RAGE_IIC_PCI ||
	    mach64_chip_id == PCI_PRODUCT_ATI_RAGE_IIC_AGP_B ||
	    mach64_chip_id == PCI_PRODUCT_ATI_RAGE_IIC_AGP_P) {
		dsp_loop_latency = 0;
		fifo_depth = 24;
	} else {
		dsp_loop_latency = 2;
		fifo_depth = 32;
	}

	dsp_precision = 0;

	xclks_per_qw = (sc->mclk_fb_div * sc->vclk_post_div * 64 << 11) /
	    (sc->vclk_fb_div * sc->mclk_post_div * sc->bits_per_pixel);

	xclks_per_qw_m = (sc->mem_freq * 64 << 4) /
		       (sc->vclk_freq * sc->bits_per_pixel);

	DPRINTF("xclks_per_qw %d %d\n", xclks_per_qw >> 7, xclks_per_qw_m);
	DPRINTF("mem %dkHz v %dkHz\n", sc->mem_freq, sc->vclk_freq);

	y = (xclks_per_qw * fifo_depth) >> 11;
		       
	while (y) {
		y >>= 1;
		dsp_precision++;
	}
	dsp_precision -= 5;
	fifo_off = ((xclks_per_qw * (fifo_depth - 1)) >> 5) + (3 << 6);

	switch (sc->memtype) {
	case DRAM:
	case EDO_DRAM:
	case PSEUDO_EDO:
		if (sc->memsize > 1024) {
			page_size = 9;
			dsp_loop_latency += 6;
		} else {
			page_size = 10;
			if (sc->memtype == DRAM)
				dsp_loop_latency += 8;
			else
				dsp_loop_latency += 7;
		}
		break;
	case SDRAM:
		if (sc->memsize > 1024) {
			page_size = 8;
			dsp_loop_latency += 8;
		} else {
			page_size = 10;
			dsp_loop_latency += 9;
		}
		break;
	case SGRAM:
		page_size = 8;
		dsp_loop_latency = 8;
		break;
	default:
		page_size = 10;
		dsp_loop_latency += 9;
		break;
	}

	if (xclks_per_qw >= (page_size << 11))
		fifo_on = ((2 * page_size + 1) << 6) + (xclks_per_qw >> 5);
	else
		fifo_on = (3 * page_size + 2) << 6;

	dsp_xclks_per_qw = xclks_per_qw >> dsp_precision;
	dsp_on = fifo_on >> dsp_precision;
	dsp_off = fifo_off >> dsp_precision;

	DPRINTF("dsp_xclks_per_qw = %d, dsp_on = %d, dsp_off = %d,\n"
	    "dsp_precision = %d, dsp_loop_latency = %d,\n"
	    "mclk_fb_div = %d, vclk_fb_div = %d,\n"
	    "mclk_post_div = %d, vclk_post_div = %d\n",
	    dsp_xclks_per_qw, dsp_on, dsp_off, dsp_precision, dsp_loop_latency,
	    sc->mclk_fb_div, sc->vclk_fb_div,
	    sc->mclk_post_div, sc->vclk_post_div);
	DPRINTF("DSP_ON_OFF %08x\n", regr(sc, DSP_ON_OFF));
	DPRINTF("DSP_CONFIG %08x\n", regr(sc, DSP_CONFIG));
	regw(sc, DSP_ON_OFF, ((dsp_on << 16) & DSP_ON) | (dsp_off & DSP_OFF));
	regw(sc, DSP_CONFIG, ((dsp_precision << 20) & DSP_PRECISION) |
	    ((dsp_loop_latency << 16) & DSP_LOOP_LATENCY) |
	    (dsp_xclks_per_qw & DSP_XCLKS_PER_QW));
	DPRINTF("DSP_ON_OFF %08x\n", regr(sc, DSP_ON_OFF));
	DPRINTF("DSP_CONFIG %08x\n", regr(sc, DSP_CONFIG));
}

static void
mach64_set_pll(struct mach64_softc *sc, int clock)
{
	uint32_t q, clockreg;
	int clockshift = sc->sc_clock << 1;
	uint8_t reg, vclk_ctl;

	q = (clock * sc->ref_div * 100) / (2 * sc->ref_freq);
#ifdef MACHFB_DEBUG
	printf("q = %d\n", q);
#endif
	if (q > 25500) {
		aprint_error_dev(sc->sc_dev, "Warning: q > 25500\n");
		q = 25500;
		sc->vclk_post_div = 1;
		sc->log2_vclk_post_div = 0;
	} else if (q > 12750) {
		sc->vclk_post_div = 1;
		sc->log2_vclk_post_div = 0;
	} else if (q > 6350) {
		sc->vclk_post_div = 2;
		sc->log2_vclk_post_div = 1;
	} else if (q > 3150) {
		sc->vclk_post_div = 4;
		sc->log2_vclk_post_div = 2;
	} else if (q >= 1600) {
		sc->vclk_post_div = 8;
		sc->log2_vclk_post_div = 3;
	} else {
		aprint_error_dev(sc->sc_dev, "Warning: q < 1600\n");
		sc->vclk_post_div = 8;
		sc->log2_vclk_post_div = 3;
	}
	sc->vclk_fb_div = q * sc->vclk_post_div / 100;
	DPRINTF("post_div: %d log2_post_div: %d mclk_div: %d\n",
	    sc->vclk_post_div, sc->log2_vclk_post_div, sc->mclk_fb_div);

	vclk_ctl = regrb_pll(sc, PLL_VCLK_CNTL);
	aprint_debug("vclk_ctl: %02x\n", vclk_ctl);
	vclk_ctl |= PLL_VCLK_RESET;
	regwb_pll(sc, PLL_VCLK_CNTL, vclk_ctl);

	DPRINTF("target: %d output: %d\n", clock, 
	    (2 * sc->ref_freq * sc->vclk_fb_div) / 
	    (sc->ref_div * sc->vclk_post_div));
	
	regwb_pll(sc, MCLK_FB_DIV, sc->mclk_fb_div);
	reg = regrb_pll(sc, VCLK_POST_DIV);
	reg &= ~(3 << clockshift);
	reg |= (sc->log2_vclk_post_div << clockshift);
	regwb_pll(sc, VCLK_POST_DIV, reg);
	regwb_pll(sc, VCLK0_FB_DIV + sc->sc_clock, sc->vclk_fb_div);

	vclk_ctl &= ~PLL_VCLK_RESET;
	regwb_pll(sc, PLL_VCLK_CNTL, vclk_ctl);

	clockreg = regr(sc, CLOCK_CNTL);
	clockreg &= ~CLOCK_SEL;
	clockreg |= sc->sc_clock | CLOCK_STROBE;
	regw(sc, CLOCK_CNTL, clockreg);
	sc->vclk_freq = clock;
}

static void
mach64_init_lut(struct mach64_softc *sc)
{
	uint8_t cmap[768];
	int i, idx;

	rasops_get_cmap(&mach64_console_screen.scr_ri, cmap, sizeof(cmap));
	idx = 0;
	for (i = 0; i < 256; i++) {
		mach64_putpalreg(sc, i, cmap[idx], cmap[idx + 1],
		    cmap[idx + 2]);
		idx += 3;
	}
}

static int
mach64_putpalreg(struct mach64_softc *sc, uint8_t index, uint8_t r, uint8_t g,
    uint8_t b)
{
	sc->sc_cmap_red[index] = r;
	sc->sc_cmap_green[index] = g;
	sc->sc_cmap_blue[index] = b;
	/*
	 * writing the dac index takes a while, in theory we can poll some
	 * register to see when it's ready - but we better avoid writing it
	 * unnecessarily
	 */
	if (index != sc->sc_dacw) {
		regwb(sc, DAC_MASK, 0xff);
		regwb(sc, DAC_WINDEX, index);
	}
	sc->sc_dacw = index + 1;
	regwb(sc, DAC_DATA, r);
	regwb(sc, DAC_DATA, g);
	regwb(sc, DAC_DATA, b);
	return 0;
}

static int
mach64_putcmap(struct mach64_softc *sc, struct wsdisplay_cmap *cm)
{
	uint index = cm->index;
	uint count = cm->count;
	int i, error;
	uint8_t rbuf[256], gbuf[256], bbuf[256];
	uint8_t *r, *g, *b;

	if (cm->index >= 256 || cm->count > 256 ||
	    (cm->index + cm->count) > 256)
		return EINVAL;
	error = copyin(cm->red, &rbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->green, &gbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->blue, &bbuf[index], count);
	if (error)
		return error;

	memcpy(&sc->sc_cmap_red[index], &rbuf[index], count);
	memcpy(&sc->sc_cmap_green[index], &gbuf[index], count);
	memcpy(&sc->sc_cmap_blue[index], &bbuf[index], count);

	r = &sc->sc_cmap_red[index];
	g = &sc->sc_cmap_green[index];
	b = &sc->sc_cmap_blue[index];

	for (i = 0; i < count; i++) {
		mach64_putpalreg(sc, index, *r, *g, *b);
		index++;
		r++, g++, b++;
	}
	return 0;
}

static int
mach64_getcmap(struct mach64_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int error;

	if (index >= 255 || count > 256 || index + count > 256)
		return EINVAL;

	error = copyout(&sc->sc_cmap_red[index],   cm->red,   count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_green[index], cm->green, count);
	if (error)
		return error;
	error = copyout(&sc->sc_cmap_blue[index],  cm->blue,  count);
	if (error)
		return error;

	return 0;
}

static int
mach64_is_console(struct mach64_softc *sc)
{
	bool console = 0;

	prop_dictionary_get_bool(device_properties(sc->sc_dev),
	    "is_console", &console);
	return console;
}

/*
 * wsdisplay_emulops
 */

static void
mach64_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mach64_softc *sc = scr->scr_cookie;
	int x, y, wi, he;

	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;

	if ((!sc->sc_locked) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		if (ri->ri_flg & RI_CURSOR) {
			mach64_bitblt(sc, x, y, x, y, wi, he, MIX_NOT_SRC);
			ri->ri_flg &= ~RI_CURSOR;
		}
		ri->ri_crow = row;
		ri->ri_ccol = col;
		if (on) {
			x = ri->ri_ccol * wi + ri->ri_xorigin;
			y = ri->ri_crow * he + ri->ri_yorigin;
			mach64_bitblt(sc, x, y, x, y, wi, he, MIX_NOT_SRC);
			ri->ri_flg |= RI_CURSOR;
		}
	} else {
		scr->scr_ri.ri_crow = row;
		scr->scr_ri.ri_ccol = col;
		scr->scr_ri.ri_flg &= ~RI_CURSOR;
	}
}

#if 0
static int
mach64_mapchar(void *cookie, int uni, u_int *index)
{
	return 0;
}
#endif

static void
mach64_putchar_mono(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct mach64_softc *sc = scr->scr_cookie;

	if (sc->sc_mode == WSDISPLAYIO_MODE_EMUL) {
		int fg, bg, uc;
		uint8_t *data;
		int x, y, wi, he;
		wi = font->fontwidth;
		he = font->fontheight;

		if (!CHAR_IN_FONT(c, font))
			return;
		bg = ri->ri_devcmap[(attr >> 16) & 0x0f];
		fg = ri->ri_devcmap[(attr >> 24) & 0x0f];
		x = ri->ri_xorigin + col * wi;
		y = ri->ri_yorigin + row * he;
		if (c == 0x20) {
			mach64_rectfill(sc, x, y, wi, he, bg);
		} else {
			uc = c - font->firstchar;
			data = (uint8_t *)font->data + uc *
			    ri->ri_fontscale;

			mach64_setup_mono(sc, x, y, wi, he, fg, bg);
			mach64_feed_bytes(sc, ri->ri_fontscale, data);
		}
		if (attr & 1)
			mach64_rectfill(sc, x, y + he - 2, wi, 1, fg);
	}
}

static void
mach64_putchar_aa8(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, c);
	struct vcons_screen *scr = ri->ri_hw;
	struct mach64_softc *sc = scr->scr_cookie;
	uint32_t bg, fg, latch = 0, bg8, fg8, pixel;
	int i, x, y, wi, he, r, g, b, aval;
	int r1, g1, b1, r0, g0, b0, fgo, bgo;
	uint8_t *data8;
	int rv = 0, cnt = 0;

	if (sc->sc_mode != WSDISPLAYIO_MODE_EMUL)
		return;

	if (!CHAR_IN_FONT(c, font))
		return;

	wi = font->fontwidth;
	he = font->fontheight;
	bg = (u_char)ri->ri_devcmap[(attr >> 16) & 0x0f];
	fg = (u_char)ri->ri_devcmap[(attr >> 24) & 0x0f];
	x = ri->ri_xorigin + col * wi;
	y = ri->ri_yorigin + row * he;

	if (c == 0x20) {
		mach64_rectfill(sc, x, y, wi, he, bg);
		if (attr & 1)
			mach64_rectfill(sc, x, y + he - 2, wi, 1, fg);
		return;
	}

	rv = glyphcache_try(&sc->sc_gc, c, x, y, attr);
	if (rv == GC_OK)
		return;

	data8 = WSFONT_GLYPH(c, font);

	wait_for_fifo(sc, 11);
	regw(sc, DP_PIX_WIDTH, DST_8BPP | SRC_8BPP | HOST_8BPP);
	regw(sc, DP_SRC, MONO_SRC_ONE | BKGD_SRC_HOST | FRGD_SRC_HOST);
	regw(sc, DP_MIX, ((MIX_SRC & 0xffff) << 16) | MIX_SRC);
	regw(sc, CLR_CMP_CNTL ,0);	/* no transparency */
	regw(sc, SRC_CNTL, SRC_LINE_X_LEFT_TO_RIGHT);
	regw(sc, DST_CNTL, DST_Y_TOP_TO_BOTTOM | DST_X_LEFT_TO_RIGHT);
	regw(sc, HOST_CNTL, HOST_BYTE_ALIGN);
	regw(sc, SRC_Y_X, 0);
	regw(sc, SRC_WIDTH1, wi);
	regw(sc, DST_Y_X, (x << 16) | y);
	regw(sc, DST_HEIGHT_WIDTH, (wi << 16) | he);

	/*
	 * we need the RGB colours here, so get offsets into rasops_cmap
	 */
	fgo = ((attr >> 24) & 0xf) * 3;
	bgo = ((attr >> 16) & 0xf) * 3;

	r0 = rasops_cmap[bgo];
	r1 = rasops_cmap[fgo];
	g0 = rasops_cmap[bgo + 1];
	g1 = rasops_cmap[fgo + 1];
	b0 = rasops_cmap[bgo + 2];
	b1 = rasops_cmap[fgo + 2];
#define R3G3B2(r, g, b) ((r & 0xe0) | ((g >> 3) & 0x1c) | (b >> 6))
	bg8 = R3G3B2(r0, g0, b0);
	fg8 = R3G3B2(r1, g1, b1);

	wait_for_fifo(sc, 10);

	for (i = 0; i < ri->ri_fontscale; i++) {
		aval = *data8;
		if (aval == 0) {
			pixel = bg8;
		} else if (aval == 255) {
			pixel = fg8;
		} else {
			r = aval * r1 + (255 - aval) * r0;
			g = aval * g1 + (255 - aval) * g0;
			b = aval * b1 + (255 - aval) * b0;
			pixel = ((r & 0xe000) >> 8) |
				((g & 0xe000) >> 11) |
				((b & 0xc000) >> 14);
		}
		latch = (latch << 8) | pixel;
		/* write in 32bit chunks */
		if ((i & 3) == 3) {
			regws(sc, HOST_DATA0, latch);
			/*
			 * not strictly necessary, old data should be shifted
			 * out
			 */
			latch = 0;
			cnt++;
			if (cnt > 8) {
				wait_for_fifo(sc, 10);
				cnt = 0;
			}
		}
		data8++;
	}
	/* if we have pixels left in latch write them out */
	if ((i & 3) != 0) {
		latch = latch << ((4 - (i & 3)) << 3);
		regws(sc, HOST_DATA0, latch);
	}

	if (rv == GC_ADD) {
		glyphcache_add(&sc->sc_gc, c, x, y);
	} else 	if (attr & 1) {
		mach64_rectfill(sc, x, y + he - 2, wi, 1, fg);
	}

}

static void
mach64_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mach64_softc *sc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		mach64_bitblt(sc, xs, y, xd, y, width, height, MIX_SRC);
	}
}

static void
mach64_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mach64_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		mach64_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}

static void
mach64_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mach64_softc *sc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight*nrows;
		mach64_bitblt(sc, x, ys, x, yd, width, height, MIX_SRC);
	}
}

static void
mach64_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct vcons_screen *scr = ri->ri_hw;
	struct mach64_softc *sc = scr->scr_cookie;
	int32_t x, y, width, height, fg, bg, ul;

	if ((sc->sc_locked == 0) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		if ((row == 0) && (nrows == ri->ri_rows)) {
			/* clear full screen */
			x = 0;
			y = 0;
			width = sc->virt_x;
			height = sc->virt_y;
		} else {
			x = ri->ri_xorigin;
			y = ri->ri_yorigin + ri->ri_font->fontheight * row;
			width = ri->ri_emuwidth;
			height = ri->ri_font->fontheight * nrows;
		}
		rasops_unpack_attr(fillattr, &fg, &bg, &ul);

		mach64_rectfill(sc, x, y, width, height, ri->ri_devcmap[bg]);
	}
}

static void
mach64_bitblt(void *cookie, int xs, int ys, int xd, int yd, int width,
    int height, int rop)
{
	struct mach64_softc *sc = cookie;
	uint32_t dest_ctl = 0;
	
#if 0
	wait_for_idle(sc);
#else
	wait_for_fifo(sc, 10);
#endif

	regw(sc, DP_PIX_WIDTH, DST_8BPP | SRC_8BPP | HOST_8BPP);
	regw(sc, DP_SRC, FRGD_SRC_BLIT);
	regw(sc, DP_MIX, (rop & 0xffff) << 16);
	regw(sc, CLR_CMP_CNTL, 0);	/* no transparency */
	if (yd < ys) {
		dest_ctl = DST_Y_TOP_TO_BOTTOM;
	} else {
		ys += height - 1;
		yd += height - 1;
		dest_ctl = DST_Y_BOTTOM_TO_TOP;
	}
	if (xd < xs) {
		dest_ctl |= DST_X_LEFT_TO_RIGHT;
		regw(sc, SRC_CNTL, SRC_LINE_X_LEFT_TO_RIGHT);
	} else {
		dest_ctl |= DST_X_RIGHT_TO_LEFT;
		xs += width - 1;
		xd += width - 1;
		regw(sc, SRC_CNTL, SRC_LINE_X_RIGHT_TO_LEFT);
	}
	regw(sc, DST_CNTL, dest_ctl);

	regw(sc, SRC_Y_X, (xs << 16) | ys);
	regw(sc, SRC_WIDTH1, width);
	regw(sc, DST_Y_X, (xd << 16) | yd);
	regw(sc, DST_HEIGHT_WIDTH, (width << 16) | height);
}

static void
mach64_setup_mono(struct mach64_softc *sc, int xd, int yd, int width,
     int height, uint32_t fg, uint32_t bg)
{
	wait_for_idle(sc);
	regw(sc, DP_WRITE_MASK, 0xff);	/* XXX only good for 8 bit */
	regw(sc, DP_PIX_WIDTH, DST_8BPP | SRC_1BPP | HOST_1BPP);
	regw(sc, DP_SRC, MONO_SRC_HOST | BKGD_SRC_BKGD_CLR | FRGD_SRC_FRGD_CLR);
	regw(sc, DP_MIX, ((MIX_SRC & 0xffff) << 16) | MIX_SRC);
	regw(sc, CLR_CMP_CNTL ,0);	/* no transparency */
	regw(sc, SRC_CNTL, SRC_LINE_X_LEFT_TO_RIGHT);
	regw(sc, DST_CNTL, DST_Y_TOP_TO_BOTTOM | DST_X_LEFT_TO_RIGHT);
	regw(sc, HOST_CNTL, HOST_BYTE_ALIGN);
	regw(sc, DP_BKGD_CLR, bg);
	regw(sc, DP_FRGD_CLR, fg);
	regw(sc, SRC_Y_X, 0);
	regw(sc, SRC_WIDTH1, width);
	regw(sc, DST_Y_X, (xd << 16) | yd);
	regw(sc, DST_HEIGHT_WIDTH, (width << 16) | height);
	/* now feed the data into the chip */
}

static void
mach64_feed_bytes(struct mach64_softc *sc, int count, uint8_t *data)
{
	int i;
	uint32_t latch = 0, bork;
	int shift = 0;
	int reg = 0;

	for (i = 0; i < count; i++) {
		bork = data[i];
		latch |= (bork << shift);
		if (shift == 24) {
			regw(sc, HOST_DATA0 + reg, latch);
			latch = 0;
			shift = 0;
			reg = (reg + 4) & 0x3c;
		} else
			shift += 8;
	}
	if (shift != 0)	/* 24 */
		regw(sc, HOST_DATA0 + reg, latch);
}


static void
mach64_rectfill(struct mach64_softc *sc, int x, int y, int width, int height,
    int colour)
{
	wait_for_fifo(sc, 11);
	regw(sc, DP_FRGD_CLR, colour);
	regw(sc, DP_PIX_WIDTH, DST_8BPP | SRC_8BPP | HOST_8BPP);
	regw(sc, DP_SRC, FRGD_SRC_FRGD_CLR);
	regw(sc, DP_MIX, MIX_SRC << 16);
	regw(sc, CLR_CMP_CNTL, 0);	/* no transparency */
	regw(sc, SRC_CNTL, SRC_LINE_X_LEFT_TO_RIGHT);
	regw(sc, DST_CNTL, DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM);

	regw(sc, SRC_Y_X, (x << 16) | y);
	regw(sc, SRC_WIDTH1, width);
	regw(sc, DST_Y_X, (x << 16) | y);
	regw(sc, DST_HEIGHT_WIDTH, (width << 16) | height);
}

static void
mach64_clearscreen(struct mach64_softc *sc)
{
	mach64_rectfill(sc, 0, 0, sc->virt_x, sc->virt_y, sc->sc_bg);
}


#if 0
static void
mach64_showpal(struct mach64_softc *sc)
{
	int i, x = 0;

	for (i = 0; i < 16; i++) {
		mach64_rectfill(sc, x, 0, 64, 64, i);
		x += 64;
	}
}
#endif

/*
 * wsdisplay_accessops
 */

static int
mach64_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd = v;
	struct mach64_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wdf;
	struct vcons_screen *ms = vd->active;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
		return 0;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->stride * sc->bits_per_pixel / 8;
		return 0;

	case WSDISPLAYIO_GINFO:
		wdf = (void *)data;
		wdf->height = sc->virt_y;
		wdf->width = sc->virt_x;
		wdf->depth = sc->bits_per_pixel;
		wdf->cmsize = 256;
		return 0;

	case WSDISPLAYIO_GETCMAP:
		return mach64_getcmap(sc,
		    (struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return mach64_putcmap(sc,
		    (struct wsdisplay_cmap *)data);

	/* PCI config read/write passthrough. */
	case PCI_IOC_CFGREAD:
	case PCI_IOC_CFGWRITE:
		return pci_devioctl(sc->sc_pc, sc->sc_pcitag,
		    cmd, data, flag, l);

	case WSDISPLAYIO_GET_BUSID:
		return wsdisplayio_busid_pci(sc->sc_dev, sc->sc_pc,
		    sc->sc_pcitag, data);

	case WSDISPLAYIO_SMODE: {
		int new_mode = *(int*)data;
		if (new_mode != sc->sc_mode) {
			sc->sc_mode = new_mode;
			if ((new_mode == WSDISPLAYIO_MODE_EMUL)
			    && (ms != NULL))
			{
				/* restore initial video mode */
				mach64_init(sc);
				mach64_init_engine(sc);
				mach64_init_lut(sc);
				if (sc->sc_setmode)
					mach64_modeswitch(sc, sc->sc_my_mode);
				mach64_clearscreen(sc);
				glyphcache_wipe(&sc->sc_gc);
				vcons_redraw_screen(ms);
			}
		}
		}
		return 0;
	case WSDISPLAYIO_GET_EDID: {
		struct wsdisplayio_edid_info *d = data;
		return wsdisplayio_get_edid(sc->sc_dev, d);
	}

	case WSDISPLAYIO_GET_FBINFO: {
		struct wsdisplayio_fbinfo *fbi = data;
		return wsdisplayio_get_fbinfo(&ms->scr_ri, fbi);
	}
	}
	return EPASSTHROUGH;
}

static paddr_t
mach64_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd = v;
	struct mach64_softc *sc = vd->cookie;
	paddr_t pa;

	if (sc->sc_mode == WSDISPLAYIO_MODE_DUMBFB) {
		/*
		 *'regular' framebuffer mmap()ing
		 */
		if (offset < (sc->memsize * 1024)) {
			pa = bus_space_mmap(sc->sc_memt, sc->sc_aperbase,
			    offset, prot, BUS_SPACE_MAP_LINEAR);
			return pa;
		}
	} else if (sc->sc_mode == WSDISPLAYIO_MODE_MAPPED) {
		/*
		 * restrict all other mappings to processes with superuser
		 * privileges
		 */
		if (kauth_authorize_machdep(kauth_cred_get(),
		    KAUTH_MACHDEP_UNMANAGEDMEM,
		    NULL, NULL, NULL, NULL) != 0) {
			return -1;
		}
		if ((offset >= sc->sc_aperbase) &&
		    (offset < (sc->sc_aperbase + sc->sc_apersize))) {
			pa = bus_space_mmap(sc->sc_memt, offset, 0, prot,
			    BUS_SPACE_MAP_LINEAR);
			return pa;
		}

		if ((offset >= sc->sc_regbase) &&
		    (offset < (sc->sc_regbase + sc->sc_regsize))) {
			pa = bus_space_mmap(sc->sc_regt, offset, 0, prot,
			    BUS_SPACE_MAP_LINEAR);
			return pa;
		}

		if ((offset >= sc->sc_rom.vb_base) &&
		    (offset < (sc->sc_rom.vb_base + sc->sc_rom.vb_size))) {
			pa = bus_space_mmap(sc->sc_memt, offset, 0, prot,
			    BUS_SPACE_MAP_LINEAR);
			return pa;
		}

#ifdef PCI_MAGIC_IO_RANGE
		if ((offset >= PCI_MAGIC_IO_RANGE) &&
		    (offset <= PCI_MAGIC_IO_RANGE + 0x10000)) {
		    	return bus_space_mmap(sc->sc_iot,
		    	   offset - PCI_MAGIC_IO_RANGE, 0, prot, 0);
		}
#endif
	}
	return -1;
}

#if 0
static int
mach64_load_font(void *v, void *cookie, struct wsdisplay_font *data)
{

	return 0;
}
#endif

void
machfb_blank(struct mach64_softc *sc, int blank)
{
	uint32_t reg;

#define MACH64_BLANK (CRTC_DISPLAY_DIS | CRTC_HSYNC_DIS | CRTC_VSYNC_DIS)

	switch (blank)
	{
    		case 0:
			reg = regr(sc, CRTC_GEN_CNTL);
			regw(sc, CRTC_GEN_CNTL, reg & ~(MACH64_BLANK));
			sc->sc_blanked = 0;
			break;
		case 1:
			reg = regr(sc, CRTC_GEN_CNTL);
			regw(sc, CRTC_GEN_CNTL, reg | (MACH64_BLANK));
			sc->sc_blanked = 1;
			break;
		default:
        		break;
	}
}
