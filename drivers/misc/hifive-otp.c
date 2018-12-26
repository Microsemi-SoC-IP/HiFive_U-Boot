// SPDX-License-Identifier: GPL-2.0
/*
 * This is a driver for the eMemory EG004K32TQ028XW01 NeoFuse
 * One-Time-Programmable (OTP) memory used within the SiFive FU540.
 * It is documented in the FU540 manual here:
 * https://www.sifive.com/documentation/chips/freedom-u540-c000-manual/
 *
 * Copyright (C) 2018 Philipp Hug <philipp@hug.cx>
 * Copyright (C) 2018 Joey Hewitt <joey@joeyhewitt.com>
 */

/*
 * The FU540 stores 4096x32 bit (16KiB) values.
 * Index 0x00-0xff are reserved for SiFive internal use. (first 1KiB)
 */

#include <common.h>
#include <dm/device.h>
#include <dm/read.h>
#include <linux/io.h>
#include <misc.h>

struct hifive_otp_regs {
	u32 pa;     /* Address input */
	u32 paio;   /* Program address input */
	u32 pas;    /* Program redundancy cell selection input */
	u32 pce;    /* OTP Macro enable input */
	u32 pclk;   /* Clock input */
	u32 pdin;   /* Write data input */
	u32 pdout;  /* Read data output */
	u32 pdstb;  /* Deep standby mode enable input (active low) */
	u32 pprog;  /* Program mode enable input */
	u32 ptc;    /* Test column enable input */
	u32 ptm;    /* Test mode enable input */
	u32 ptm_rep;/* Repair function test mode enable input */
	u32 ptr;    /* Test row enable input */
	u32 ptrim;  /* Repair function enable input */
	u32 pwe;    /* Write enable input (defines program cycle) */
} __packed;

struct hifive_otp_platdata {
	struct hifive_otp_regs __iomem *regs;
};

typedef u32 fuse_value_t;
#define BYTES_PER_FUSE     4

#define NUM_FUSES          0x1000

/*
 * offset and size are assumed aligned to the size of the fuses (32bit).
 */
static int hifive_otp_read(struct udevice *dev, int offset,
			       void *buf, int size)
{
	struct hifive_otp_platdata *plat = dev_get_platdata(dev);
	struct hifive_otp_regs *regs = (struct hifive_otp_regs *)plat->regs;

	int fuseidx = offset / BYTES_PER_FUSE;
	int fusecount = size / BYTES_PER_FUSE;
	fuse_value_t fusebuf[fusecount];

	// check bounds
	if (offset < 0 || size < 0)
		return -EINVAL;
	if (fuseidx >= NUM_FUSES)
		return -EINVAL;
	if ((fuseidx + fusecount) > NUM_FUSES)
		return -EINVAL;

	// init OTP
	iowrite32(0x01, &regs->pdstb); // wake up from stand-by
	iowrite32(0x01, &regs->ptrim); // enable repair function
	iowrite32(0x01, &regs->pce);   // enable input

	// read all requested fuses
	for (unsigned int i = 0; i < fusecount; i++, fuseidx++) {
		iowrite32(fuseidx, &regs->pa);

		// cycle clock to read
		iowrite32(0x01, &regs->pclk);
		mdelay(1);
		iowrite32(0x00, &regs->pclk);
		mdelay(1);

		// read the value
		fusebuf[i] = ioread32(&regs->pdout);
	}

	// shut down
	iowrite32(0, &regs->pce);
	iowrite32(0, &regs->ptrim);
	iowrite32(0, &regs->pdstb);

	// copy out
	memcpy(buf, fusebuf, size);

	return 0;
}

static int hifive_otp_ofdata_to_platdata(struct udevice *dev)
{
	struct hifive_otp_platdata *plat = dev_get_platdata(dev);

	plat->regs = dev_read_addr_ptr(dev);
	return 0;
}

static const struct misc_ops hifive_otp_ops = {
	.read = hifive_otp_read,
};

static const struct udevice_id hifive_otp_ids[] = {
	{ .compatible = "sifive,ememoryotp0" },
	{}
};

U_BOOT_DRIVER(hifive_otp) = {
	.name = "hifive_otp",
	.id = UCLASS_MISC,
	.of_match = hifive_otp_ids,
	.ofdata_to_platdata = hifive_otp_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct hifive_otp_platdata),
	.ops = &hifive_otp_ops,
};
