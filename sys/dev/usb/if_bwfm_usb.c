/* $NetBSD: if_bwfm_usb.c,v 1.15 2020/07/22 17:18:10 riastradh Exp $ */
/* $OpenBSD: if_bwfm_usb.c,v 1.2 2017/10/15 14:55:13 patrick Exp $ */
/*
 * Copyright (c) 2010-2016 Broadcom Corporation
 * Copyright (c) 2016,2017 Patrick Wildt <patrick@blueri.se>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_bwfm_usb.c,v 1.15 2020/07/22 17:18:10 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/buf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/workqueue.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <netinet/in.h>

#include <net80211/ieee80211_netbsd.h>
#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_regdomain.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>

#include <dev/ic/bwfmreg.h>
#include <dev/ic/bwfmvar.h>

static const struct bwfm_firmware_selector bwfm_usb_fwtab[] = {
	BWFM_FW_ENTRY(BRCM_CC_43143_CHIP_ID,
		      BWFM_FWSEL_ALLREVS, "brcmfmac43143"),

	BWFM_FW_ENTRY(BRCM_CC_43235_CHIP_ID,
		      BWFM_FWSEL_REV_EQ(3), "brcmfmac43236b"),
	BWFM_FW_ENTRY(BRCM_CC_43236_CHIP_ID,
		      BWFM_FWSEL_REV_EQ(3), "brcmfmac43236b"),
	BWFM_FW_ENTRY(BRCM_CC_43238_CHIP_ID,
		      BWFM_FWSEL_REV_EQ(3), "brcmfmac43236b"),

	BWFM_FW_ENTRY(BRCM_CC_43242_CHIP_ID,
		      BWFM_FWSEL_ALLREVS, "brcmfmac43242a"),

	BWFM_FW_ENTRY(BRCM_CC_43566_CHIP_ID,
		      BWFM_FWSEL_ALLREVS, "brcmfmac43569"),
	BWFM_FW_ENTRY(BRCM_CC_43569_CHIP_ID,
		      BWFM_FWSEL_ALLREVS, "brcmfmac43569"),

	BWFM_FW_ENTRY(CY_CC_4373_CHIP_ID,
		      BWFM_FWSEL_ALLREVS, "brcmfmac4373"),

	BWFM_FW_ENTRY_END
};

/*
 * Various supported device vendors/products.
 */
static const struct usb_devno bwfm_usbdevs[] = {
	{ USB_VENDOR_BROADCOM,	USB_PRODUCT_BROADCOM_BCM43143 },
	{ USB_VENDOR_BROADCOM,	USB_PRODUCT_BROADCOM_BCM43236 },
	{ USB_VENDOR_BROADCOM,	USB_PRODUCT_BROADCOM_BCM43242 },
	{ USB_VENDOR_BROADCOM,	USB_PRODUCT_BROADCOM_BCM43569 },
	{ USB_VENDOR_BROADCOM,	USB_PRODUCT_BROADCOM_BCMFW },
};

#ifdef BWFM_DEBUG
#define DPRINTF(x)	do { if (bwfm_debug > 0) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (bwfm_debug >= (n)) printf x; } while (0)
static int bwfm_debug = 2;
#else
#define DPRINTF(x)	do { ; } while (0)
#define DPRINTFN(n, x)	do { ; } while (0)
#endif

#define DEVNAME(sc)	device_xname((sc)->sc_sc.sc_dev)

#define BRCMF_POSTBOOT_ID	0xA123	/* ID to detect if dongle
					 * has boot up
					 */

#define TRX_MAGIC		0x30524448	/* "HDR0" */
#define TRX_MAX_OFFSET		3		/* Max number of file offsets */
#define TRX_UNCOMP_IMAGE	0x20		/* Trx holds uncompressed img */
#define TRX_RDL_CHUNK		1500		/* size of each dl transfer */
#define TRX_OFFSETS_DLFWLEN_IDX	0

/* Control messages: bRequest values */
#define DL_GETSTATE	0	/* returns the rdl_state_t struct */
#define DL_CHECK_CRC	1	/* currently unused */
#define DL_GO		2	/* execute downloaded image */
#define DL_START	3	/* initialize dl state */
#define DL_REBOOT	4	/* reboot the device in 2 seconds */
#define DL_GETVER	5	/* returns the bootrom_id_t struct */
#define DL_GO_PROTECTED	6	/* execute the downloaded code and set reset
				 * event to occur in 2 seconds.  It is the
				 * responsibility of the downloaded code to
				 * clear this event
				 */
#define DL_EXEC		7	/* jump to a supplied address */
#define DL_RESETCFG	8	/* To support single enum on dongle
				 * - Not used by bootloader
				 */
#define DL_DEFER_RESP_OK 9	/* Potentially defer the response to setup
				 * if resp unavailable
				 */

/* states */
#define DL_WAITING	0	/* waiting to rx first pkt */
#define DL_READY	1	/* hdr was good, waiting for more of the
				 * compressed image
				 */
#define DL_BAD_HDR	2	/* hdr was corrupted */
#define DL_BAD_CRC	3	/* compressed image was corrupted */
#define DL_RUNNABLE	4	/* download was successful,waiting for go cmd */
#define DL_START_FAIL	5	/* failed to initialize correctly */
#define DL_NVRAM_TOOBIG	6	/* host specified nvram data exceeds DL_NVRAM
				 * value
				 */
#define DL_IMAGE_TOOBIG	7	/* firmware image too big */


struct trx_header {
	uint32_t	magic;			/* "HDR0" */
	uint32_t	len;			/* Length of file including header */
	uint32_t	crc32;			/* CRC from flag_version to end of file */
	uint32_t	flag_version;		/* 0:15 flags, 16:31 version */
	uint32_t	offsets[TRX_MAX_OFFSET];/* Offsets of partitions from start of
						 * header
						 */
};

struct rdl_state {
	uint32_t	state;
	uint32_t	bytes;
};

struct bootrom_id {
	uint32_t	chip;		/* Chip id */
	uint32_t	chiprev;	/* Chip rev */
	uint32_t	ramsize;	/* Size of  RAM */
	uint32_t	remapbase;	/* Current remap base address */
	uint32_t	boardtype;	/* Type of board */
	uint32_t	boardrev;	/* Board revision */
};

struct bwfm_usb_rx_data {
	struct bwfm_usb_softc		*sc;
	struct usbd_xfer		*xfer;
	uint8_t				*buf;
};

struct bwfm_usb_tx_data {
	struct bwfm_usb_softc		*sc;
	struct usbd_xfer		*xfer;
	uint8_t				*buf;
	struct mbuf			*mbuf;
	TAILQ_ENTRY(bwfm_usb_tx_data)	 next;
};

#define BWFM_RX_LIST_COUNT		50
#define BWFM_TX_LIST_COUNT		50
#define BWFM_RXBUFSZ			1600
#define BWFM_TXBUFSZ			1600
struct bwfm_usb_softc {
	struct bwfm_softc	 sc_sc;
	struct usbd_device	*sc_udev;
	struct usbd_interface	*sc_iface;
	uint8_t			 sc_ifaceno;

	uint16_t		 sc_vendor;
	uint16_t		 sc_product;

	uint32_t		 sc_chip;
	uint32_t		 sc_chiprev;

	int			 sc_rx_no;
	int			 sc_tx_no;

	struct usbd_pipe	*sc_rx_pipeh;
	struct usbd_pipe	*sc_tx_pipeh;

	struct bwfm_usb_rx_data	 sc_rx_data[BWFM_RX_LIST_COUNT];
	struct bwfm_usb_tx_data	 sc_tx_data[BWFM_TX_LIST_COUNT];
	TAILQ_HEAD(, bwfm_usb_tx_data) sc_tx_free_list;

	kmutex_t		 sc_rx_lock;
	kmutex_t		 sc_tx_lock;
};

int		 bwfm_usb_match(device_t, cfdata_t, void *);
void		 bwfm_usb_attachhook(device_t);
void		 bwfm_usb_attach(device_t, device_t, void *);
int		 bwfm_usb_detach(device_t, int);

int		 bwfm_usb_dl_cmd(struct bwfm_usb_softc *, uint8_t, void *, int);
int		 bwfm_usb_load_microcode(struct bwfm_usb_softc *, const u_char *,
		     size_t);

int		 bwfm_usb_alloc_rx_list(struct bwfm_usb_softc *);
void		 bwfm_usb_free_rx_list(struct bwfm_usb_softc *);
int		 bwfm_usb_alloc_tx_list(struct bwfm_usb_softc *);
void		 bwfm_usb_free_tx_list(struct bwfm_usb_softc *);

int		 bwfm_usb_txcheck(struct bwfm_softc *);
int		 bwfm_usb_txdata(struct bwfm_softc *, struct mbuf **);
int		 bwfm_usb_txctl(struct bwfm_softc *, char *, size_t);
int		 bwfm_usb_rxctl(struct bwfm_softc *, char *, size_t *);

struct mbuf *	 bwfm_usb_newbuf(void);
void		 bwfm_usb_rxeof(struct usbd_xfer *, void *, usbd_status);
void		 bwfm_usb_txeof(struct usbd_xfer *, void *, usbd_status);

static const struct bwfm_bus_ops bwfm_usb_bus_ops = {
	.bs_init = NULL,
	.bs_stop = NULL,
	.bs_txcheck = bwfm_usb_txcheck,
	.bs_txdata = bwfm_usb_txdata,
	.bs_txctl = bwfm_usb_txctl,
	.bs_rxctl = bwfm_usb_rxctl,
};

CFATTACH_DECL_NEW(bwfm_usb, sizeof(struct bwfm_usb_softc),
    bwfm_usb_match, bwfm_usb_attach, bwfm_usb_detach, NULL);

int
bwfm_usb_match(device_t parent, cfdata_t match, void *aux)
{
	struct usb_attach_arg *uaa = aux;

	return (usb_lookup(bwfm_usbdevs, uaa->uaa_vendor, uaa->uaa_product) != NULL) ?
	    UMATCH_VENDOR_PRODUCT_CONF_IFACE : UMATCH_NONE;
}

void
bwfm_usb_attach(device_t parent, device_t self, void *aux)
{
	struct bwfm_usb_softc *sc = device_private(self);
	struct usb_attach_arg *uaa = aux;
	usb_device_descriptor_t *dd;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	char *devinfop;
	int i;

	sc->sc_sc.sc_dev = self;
	sc->sc_udev = uaa->uaa_device;
	mutex_init(&sc->sc_rx_lock, MUTEX_DEFAULT, IPL_NET);
	mutex_init(&sc->sc_tx_lock, MUTEX_DEFAULT, IPL_NET);

	aprint_naive("\n");

	devinfop = usbd_devinfo_alloc(sc->sc_udev, 0);
	aprint_normal(": %s\n", devinfop);
	usbd_devinfo_free(devinfop);

	if (usbd_set_config_no(sc->sc_udev, 1, 1) != 0) {
		aprint_error_dev(self, "failed to set configuration\n");
		return;
	}
	if (usbd_device2interface_handle(sc->sc_udev, 0, &sc->sc_iface) != 0) {
		aprint_error_dev(self, "failed to get interface handle\n");
		return;
	}

	sc->sc_ifaceno = 0;
	sc->sc_vendor = uaa->uaa_vendor;
	sc->sc_product = uaa->uaa_product;
	sc->sc_sc.sc_bus_ops = &bwfm_usb_bus_ops;
	sc->sc_sc.sc_proto_ops = &bwfm_proto_bcdc_ops;

	/* Check number of configurations. */
	dd = usbd_get_device_descriptor(sc->sc_udev);
	if (dd->bNumConfigurations != 1) {
		printf("%s: number of configurations not supported\n",
		    DEVNAME(sc));
		return;
	}

	/* Get endpoints. */
	id = usbd_get_interface_descriptor(sc->sc_iface);

	sc->sc_rx_no = sc->sc_tx_no = -1;
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(sc->sc_iface, i);
		if (ed == NULL) {
			printf("%s: no endpoint descriptor for iface %d\n",
			    DEVNAME(sc), i);
			return;
		}

		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK &&
		    sc->sc_rx_no == -1)
			sc->sc_rx_no = ed->bEndpointAddress;
		else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
		    UE_GET_XFERTYPE(ed->bmAttributes) == UE_BULK &&
		    sc->sc_tx_no == -1)
			sc->sc_tx_no = ed->bEndpointAddress;
	}
	if (sc->sc_rx_no == -1 || sc->sc_tx_no == -1) {
		printf("%s: missing endpoint\n", DEVNAME(sc));
		return;
	}

	config_mountroot(self, bwfm_usb_attachhook);
}

void
bwfm_usb_attachhook(device_t self)
{
	struct bwfm_usb_softc *sc = device_private(self);
	struct bwfm_softc *bwfm = &sc->sc_sc;
	struct bwfm_usb_rx_data *data;
	struct bootrom_id brom;
	struct bwfm_firmware_context fwctx;
	usbd_status error;
	u_char *ucode;
	size_t ucsize;
	int i;

	/* Read chip id and chip rev to check the firmware. */
	memset(&brom, 0, sizeof(brom));
	bwfm_usb_dl_cmd(sc, DL_GETVER, &brom, sizeof(brom));
	sc->sc_chip = le32toh(brom.chip);
	sc->sc_chiprev = le32toh(brom.chiprev);

	/* Setup data pipes */
	error = usbd_open_pipe(sc->sc_iface, sc->sc_rx_no, USBD_EXCLUSIVE_USE,
	    &sc->sc_rx_pipeh);
	if (error != 0) {
		aprint_error_dev(bwfm->sc_dev, "could not open rx pipe: %s\n",
		    usbd_errstr(error));
		return;
	}
	error = usbd_open_pipe(sc->sc_iface, sc->sc_tx_no, USBD_EXCLUSIVE_USE,
	    &sc->sc_tx_pipeh);
	if (error != 0) {
		aprint_error_dev(bwfm->sc_dev, "could not open tx pipe: %s\n",
		    usbd_errstr(error));
		return;
	}

	/* Firmware not yet loaded? */
	if (sc->sc_chip != BRCMF_POSTBOOT_ID) {
		bwfm_firmware_context_init(&fwctx,
		    sc->sc_chip, sc->sc_chiprev, NULL,
		    BWFM_FWREQ(BWFM_FILETYPE_UCODE));

		if (!bwfm_firmware_open(bwfm, bwfm_usb_fwtab, &fwctx)) {
			/* Error message already displayed. */
			return;
		}

		ucode = bwfm_firmware_data(&fwctx, BWFM_FILETYPE_UCODE,
		    &ucsize);
		KASSERT(ucode != NULL);

		if (bwfm_usb_load_microcode(sc, ucode, ucsize) != 0) {
			aprint_error_dev(bwfm->sc_dev,
			    "could not load microcode\n");
			bwfm_firmware_close(&fwctx);
			return;
		}

		bwfm_firmware_close(&fwctx);

		for (i = 0; i < 10; i++) {
			delay(100 * 1000);
			memset(&brom, 0, sizeof(brom));
			bwfm_usb_dl_cmd(sc, DL_GETVER, &brom, sizeof(brom));
			if (le32toh(brom.chip) == BRCMF_POSTBOOT_ID)
				break;
		}

		if (le32toh(brom.chip) != BRCMF_POSTBOOT_ID) {
			aprint_error_dev(bwfm->sc_dev,
			    "firmware did not start up\n");
			return;
		}

		sc->sc_chip = le32toh(brom.chip);
		sc->sc_chiprev = le32toh(brom.chiprev);
	}

	bwfm_usb_dl_cmd(sc, DL_RESETCFG, &brom, sizeof(brom));

	if (bwfm_usb_alloc_rx_list(sc) || bwfm_usb_alloc_tx_list(sc)) {
		printf("%s: cannot allocate rx/tx lists\n", DEVNAME(sc));
		return;
	}

	bwfm_attach(&sc->sc_sc);

	for (i = 0; i < BWFM_RX_LIST_COUNT; i++) {
		data = &sc->sc_rx_data[i];

		usbd_setup_xfer(data->xfer, data, data->buf,
		    BWFM_RXBUFSZ, USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT,
		    bwfm_usb_rxeof);
		error = usbd_transfer(data->xfer);
		if (error != 0 && error != USBD_IN_PROGRESS)
			aprint_error_dev(bwfm->sc_dev,
			    "could not set up new transfer: %s\n",
			    usbd_errstr(error));
	}
}

struct mbuf *
bwfm_usb_newbuf(void)
{
	struct mbuf *m;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (NULL);

	MCLGET(m, M_DONTWAIT);
	if (!(m->m_flags & M_EXT)) {
		m_freem(m);
		return (NULL);
	}

	m->m_len = m->m_pkthdr.len = MCLBYTES;

	return (m);
}

void
bwfm_usb_rxeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct bwfm_usb_rx_data *data = priv;
	struct bwfm_usb_softc *sc = data->sc;
	struct bwfm_proto_bcdc_hdr *hdr;
	usbd_status error;
	uint32_t len, off;
	struct mbuf *m;

	DPRINTFN(2, ("%s: %s status %s\n", DEVNAME(sc), __func__,
	    usbd_errstr(status)));

	if (__predict_false(status != USBD_NORMAL_COMPLETION)) {
		usbd_clear_endpoint_stall_async(sc->sc_rx_pipeh);
		if (status != USBD_CANCELLED)
			goto resubmit;
		return;
	}
	usbd_get_xfer_status(xfer, NULL, NULL, &len, NULL);

	off = 0;
	hdr = (void *)data->buf;
	if (len < sizeof(*hdr))
		goto resubmit;
	len -= sizeof(*hdr);
	off += sizeof(*hdr);
	if (len <= hdr->data_offset << 2)
		goto resubmit;
	len -= hdr->data_offset << 2;
	off += hdr->data_offset << 2;

	m = bwfm_usb_newbuf();
	if (m == NULL)
		goto resubmit;

	memcpy(mtod(m, char *), data->buf + off, len);
	m->m_len = m->m_pkthdr.len = len;
	mutex_enter(&sc->sc_rx_lock); /* XXX */
	bwfm_rx(&sc->sc_sc, m);
	mutex_exit(&sc->sc_rx_lock);

resubmit:
	usbd_setup_xfer(data->xfer, data, data->buf,
	    BWFM_RXBUFSZ, USBD_SHORT_XFER_OK, USBD_NO_TIMEOUT,
	    bwfm_usb_rxeof);
	error = usbd_transfer(data->xfer);
	if (error != 0 && error != USBD_IN_PROGRESS)
		printf("%s: could not set up new transfer: %s\n",
		    DEVNAME(sc), usbd_errstr(error));
}

int
bwfm_usb_alloc_rx_list(struct bwfm_usb_softc *sc)
{
	struct bwfm_usb_rx_data *data;
	int i, error = 0;

	for (i = 0; i < BWFM_RX_LIST_COUNT; i++) {
		data = &sc->sc_rx_data[i];

		data->sc = sc; /* Backpointer for callbacks. */

		if (usbd_create_xfer(sc->sc_rx_pipeh, BWFM_RXBUFSZ,
		    0, 0, &data->xfer) != 0) {
			printf("%s: could not create xfer\n",
			    DEVNAME(sc));
			error = ENOMEM;
			break;
		}
		data->buf = usbd_get_buffer(data->xfer);
	}
	if (error != 0)
		bwfm_usb_free_rx_list(sc);
	return (error);
}

void
bwfm_usb_free_rx_list(struct bwfm_usb_softc *sc)
{
	int i;

	/* NB: Caller must abort pipe first. */
	for (i = 0; i < BWFM_RX_LIST_COUNT; i++) {
		if (sc->sc_rx_data[i].xfer != NULL)
			usbd_destroy_xfer(sc->sc_rx_data[i].xfer);
		sc->sc_rx_data[i].xfer = NULL;
	}
}

int
bwfm_usb_alloc_tx_list(struct bwfm_usb_softc *sc)
{
	struct bwfm_usb_tx_data *data;
	int i, error = 0;

	TAILQ_INIT(&sc->sc_tx_free_list);
	for (i = 0; i < BWFM_TX_LIST_COUNT; i++) {
		data = &sc->sc_tx_data[i];

		data->sc = sc; /* Backpointer for callbacks. */

		if (usbd_create_xfer(sc->sc_tx_pipeh, BWFM_TXBUFSZ,
		    USBD_FORCE_SHORT_XFER, 0, &data->xfer) != 0) {
			printf("%s: could not create xfer\n",
			    DEVNAME(sc));
			error = ENOMEM;
			break;
		}
		data->buf = usbd_get_buffer(data->xfer);

		/* Append this Tx buffer to our free list. */
		TAILQ_INSERT_TAIL(&sc->sc_tx_free_list, data, next);
	}
	if (error != 0)
		bwfm_usb_free_tx_list(sc);
	return (error);
}

void
bwfm_usb_free_tx_list(struct bwfm_usb_softc *sc)
{
	int i;

	/* NB: Caller must abort pipe first. */
	for (i = 0; i < BWFM_TX_LIST_COUNT; i++) {
		if (sc->sc_tx_data[i].xfer != NULL)
			usbd_destroy_xfer(sc->sc_tx_data[i].xfer);
		sc->sc_tx_data[i].xfer = NULL;
	}
}

void
bwfm_usb_txeof(struct usbd_xfer *xfer, void *priv, usbd_status status)
{
	struct bwfm_usb_tx_data *data = priv;
	struct bwfm_usb_softc *sc = data->sc;
	struct ifnet *ifp = sc->sc_sc.sc_ic.ic_ifp;
	int s;

	DPRINTFN(2, ("%s: %s status %s\n", DEVNAME(sc), __func__,
	    usbd_errstr(status)));

	m_freem(data->mbuf);
	data->mbuf = NULL;

	mutex_enter(&sc->sc_tx_lock);
	/* Put this Tx buffer back to our free list. */
	TAILQ_INSERT_TAIL(&sc->sc_tx_free_list, data, next);
	mutex_exit(&sc->sc_tx_lock);

	s = splnet();

	if (__predict_false(status != USBD_NORMAL_COMPLETION)) {
		if (status == USBD_CANCELLED)
			usbd_clear_endpoint_stall_async(sc->sc_tx_pipeh);
		if_statinc(ifp, if_oerrors);
		splx(s);
		return;
	}

	if_statinc(ifp, if_opackets);

	/* We just released a Tx buffer, notify Tx. */
	if ((ifp->if_flags & IFF_OACTIVE) != 0) {
		ifp->if_flags &= ~IFF_OACTIVE;
		if_schedule_deferred_start(ifp);
	}
	splx(s);
}

int
bwfm_usb_detach(device_t self, int flags)
{
	struct bwfm_usb_softc *sc = device_private(self);

	bwfm_detach(&sc->sc_sc, flags);

	if (sc->sc_rx_pipeh != NULL) {
		usbd_abort_pipe(sc->sc_rx_pipeh);
		usbd_close_pipe(sc->sc_rx_pipeh);
	}
	if (sc->sc_tx_pipeh != NULL) {
		usbd_abort_pipe(sc->sc_tx_pipeh);
		usbd_close_pipe(sc->sc_tx_pipeh);
	}

	bwfm_usb_free_rx_list(sc);
	bwfm_usb_free_tx_list(sc);

	mutex_destroy(&sc->sc_rx_lock);
	mutex_destroy(&sc->sc_tx_lock);

	return 0;
}

int
bwfm_usb_dl_cmd(struct bwfm_usb_softc *sc, uByte cmd, void *buf, int len)
{
	usb_device_request_t req;
	usbd_status error;

	req.bmRequestType = UT_READ_VENDOR_INTERFACE;
	req.bRequest = cmd;

	USETW(req.wValue, 0);
	USETW(req.wIndex, sc->sc_ifaceno);
	USETW(req.wLength, len);

	error = usbd_do_request(sc->sc_udev, &req, buf);
	if (error != 0) {
		printf("%s: could not read register: %s\n",
		    DEVNAME(sc), usbd_errstr(error));
	}
	return error;
}

int
bwfm_usb_load_microcode(struct bwfm_usb_softc *sc, const u_char *ucode, size_t size)
{
	const struct trx_header *trx = (const struct trx_header *)ucode;
	struct rdl_state state;
	uint32_t rdlstate, rdlbytes, sent = 0, sendlen = 0;
	struct usbd_xfer *xfer;
	usbd_status error;
	char *buf;

	if (le32toh(trx->magic) != TRX_MAGIC ||
	    (le32toh(trx->flag_version) & TRX_UNCOMP_IMAGE) == 0) {
		printf("%s: invalid firmware\n", DEVNAME(sc));
		return 1;
	}

	bwfm_usb_dl_cmd(sc, DL_START, &state, sizeof(state));
	rdlstate = le32toh(state.state);
	rdlbytes = le32toh(state.bytes);

	if (rdlstate != DL_WAITING) {
		printf("%s: cannot start fw download\n", DEVNAME(sc));
		return 1;
	}

	error = usbd_create_xfer(sc->sc_tx_pipeh, TRX_RDL_CHUNK,
	    0, 0, &xfer);
	if (error != 0) {
		printf("%s: cannot create xfer\n", DEVNAME(sc));
		goto err;
	}

	buf = usbd_get_buffer(xfer);

	while (rdlbytes != size) {
		sendlen = MIN(size - sent, TRX_RDL_CHUNK);
		memcpy(buf, ucode + sent, sendlen);

		usbd_setup_xfer(xfer, NULL, buf, sendlen,
		    USBD_SYNCHRONOUS, USBD_NO_TIMEOUT, NULL);
		error = usbd_transfer(xfer);
		if (error != 0 && error != USBD_IN_PROGRESS) {
			printf("%s: transfer error\n", DEVNAME(sc));
			goto err;
		}
		sent += sendlen;

		bwfm_usb_dl_cmd(sc, DL_GETSTATE, &state, sizeof(state));
		rdlstate = le32toh(state.state);
		rdlbytes = le32toh(state.bytes);

		if (rdlbytes != sent) {
			printf("%s: device reported different size\n",
			    DEVNAME(sc));
			goto err;
		}

		if (rdlstate == DL_BAD_HDR || rdlstate == DL_BAD_CRC) {
			printf("%s: device reported bad hdr/crc\n",
			    DEVNAME(sc));
			goto err;
		}
	}

	bwfm_usb_dl_cmd(sc, DL_GETSTATE, &state, sizeof(state));
	rdlstate = le32toh(state.state);
	rdlbytes = le32toh(state.bytes);

	if (rdlstate != DL_RUNNABLE) {
		printf("%s: dongle not runnable\n", DEVNAME(sc));
		goto err;
	}

	bwfm_usb_dl_cmd(sc, DL_GO, &state, sizeof(state));

	usbd_destroy_xfer(xfer);

	return 0;
err:
	if (sc->sc_tx_pipeh != NULL) {
		usbd_abort_pipe(sc->sc_tx_pipeh);
		usbd_close_pipe(sc->sc_tx_pipeh);
		sc->sc_tx_pipeh = NULL;
	}
	if (xfer != NULL)
		usbd_destroy_xfer(xfer);
	return 1;
}

int
bwfm_usb_txcheck(struct bwfm_softc *bwfm)
{
	struct bwfm_usb_softc *sc = (void *)bwfm;

	mutex_enter(&sc->sc_tx_lock);

	if (TAILQ_EMPTY(&sc->sc_tx_free_list)) {
		mutex_exit(&sc->sc_tx_lock);
		return ENOBUFS;
	}

	mutex_exit(&sc->sc_tx_lock);
	return 0;
}


int
bwfm_usb_txdata(struct bwfm_softc *bwfm, struct mbuf **mp)
{
	struct bwfm_usb_softc *sc = (void *)bwfm;
	struct mbuf *m = *mp;
	struct bwfm_proto_bcdc_hdr *hdr;
	struct bwfm_usb_tx_data *data;
	struct ether_header *eh;
	uint32_t len = 0;
	int error, ac;

	DPRINTFN(2, ("%s: %s\n", DEVNAME(sc), __func__));

	mutex_enter(&sc->sc_tx_lock);

	if (TAILQ_EMPTY(&sc->sc_tx_free_list)) {
		mutex_exit(&sc->sc_tx_lock);
		return ENOBUFS;
	}

	/* No QoS for EAPOL frames. */
	eh = mtod(m, struct ether_header *);
	ac = (eh->ether_type != htons(ETHERTYPE_PAE)) ?
	    M_WME_GETAC(m) : WME_AC_BE;

	/* Grab a Tx buffer from our free list. */
	data = TAILQ_FIRST(&sc->sc_tx_free_list);
	TAILQ_REMOVE(&sc->sc_tx_free_list, data, next);

	mutex_exit(&sc->sc_tx_lock);

	hdr = (void *)&data->buf[len];
	hdr->data_offset = 0;
	hdr->priority = ac;
	hdr->flags = BWFM_BCDC_FLAG_VER(BWFM_BCDC_FLAG_PROTO_VER);
	hdr->flags2 = 0;
	len += sizeof(*hdr);

	m_copydata(m, 0, m->m_pkthdr.len, &data->buf[len]);
	len += m->m_pkthdr.len;

	data->mbuf = m;

	usbd_setup_xfer(data->xfer, data, data->buf,
	    len, USBD_FORCE_SHORT_XFER, USBD_NO_TIMEOUT,
	    bwfm_usb_txeof);
	error = usbd_transfer(data->xfer);
	if (error != 0 && error != USBD_IN_PROGRESS)
		printf("%s: could not set up new transfer: %s\n",
		    DEVNAME(sc), usbd_errstr(error));
	return 0;
}

int
bwfm_usb_txctl(struct bwfm_softc *bwfm, char *buf, size_t len)
{
	struct bwfm_usb_softc *sc = (void *)bwfm;
	usb_device_request_t req;
	usbd_status error;
	int ret = 1;

	DPRINTFN(2, ("%s: %s\n", DEVNAME(sc), __func__));

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = 0;

	USETW(req.wValue, 0);
	USETW(req.wIndex, sc->sc_ifaceno);
	USETW(req.wLength, len);

	error = usbd_do_request(sc->sc_udev, &req, buf);
	if (error != 0) {
		printf("%s: could not read ctl packet: %s\n",
		    DEVNAME(sc), usbd_errstr(error));
		goto err;
	}

	ret = 0;
err:
	return ret;
}

int
bwfm_usb_rxctl(struct bwfm_softc *bwfm, char *buf, size_t *len)
{
	struct bwfm_usb_softc *sc = (void *)bwfm;
	usb_device_request_t req;
	usbd_status error;
	uint32_t len32;
	int ret = 1;

	DPRINTFN(2, ("%s: %s\n", DEVNAME(sc), __func__));

	req.bmRequestType = UT_READ_CLASS_INTERFACE;
	req.bRequest = 1;

	USETW(req.wValue, 0);
	USETW(req.wIndex, sc->sc_ifaceno);
	USETW(req.wLength, *len);

	error = usbd_do_request_flags(sc->sc_udev, &req, buf, 0,
	    &len32, USBD_DEFAULT_TIMEOUT);
	if (error != 0) {
		printf("%s: could not read ctl packet: %s\n",
		    DEVNAME(sc), usbd_errstr(error));
		goto err;
	}

	if (len32 > *len) {
		printf("%s: broken length\n", DEVNAME(sc));
		goto err;
	}

	*len = len32;
	ret = 0;
err:
	return ret;
}
