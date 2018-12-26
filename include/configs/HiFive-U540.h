/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2018 Microsemi Corporation.
 * Padmarao Begari, Microsemi Corporation <padmarao.begari@microsemi.com>
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef CONFIG_OF_CONTROL
#undef CONFIG_OF_SEPARATE
#define CONFIG_OF_EMBED
#endif

/*
 * CPU and Board Configuration Options
 */

#define CONFIG_BOOTP_SEND_HOSTNAME
//#define CONFIG_BOOTP_SERVERIP

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_CBSIZE	1024	/* Console I/O Buffer Size */

/*
 * Print Buffer Size
 */
#define CONFIG_SYS_PBSIZE	\
	(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)

/*
 * max number of command args
 */
#define CONFIG_SYS_MAXARGS	16

/*
 * Boot Argument Buffer Size
 */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE

/*
 * Size of malloc() pool
 * 512kB is suggested, (CONFIG_ENV_SIZE + 128 * 1024) was not enough
 */
#define CONFIG_SYS_MALLOC_LEN   (512 << 10)

#define CONFIG_SYS_CLK_FREQ    		1000000000

/*
 * Physical Memory Map
 */
#if 0
/*#define CONFIG_NR_DRAM_BANKS	1*/
#define PHYS_SDRAM_0	0x08000000		/* SDRAM Bank #1 */
#define PHYS_SDRAM_1	\
	(PHYS_SDRAM_0 + PHYS_SDRAM_0_SIZE)	/* SDRAM Bank #2 */
#define PHYS_SDRAM_0_SIZE	0x001E0000	/* 1.92 MB */
#define PHYS_SDRAM_1_SIZE	0x001E0000	/* 1.92 MB */
#define CONFIG_SYS_SDRAM_BASE	PHYS_SDRAM_0
#endif
#if 1
/*#define CONFIG_NR_DRAM_BANKS	1*/
#define PHYS_SDRAM_0	0x80000000		/* SDRAM Bank #1 */
#define PHYS_SDRAM_1	\
	(PHYS_SDRAM_0 + PHYS_SDRAM_0_SIZE)	/* SDRAM Bank #2 */
#define PHYS_SDRAM_0_SIZE	0x80000000	/* 2 GB */
#define PHYS_SDRAM_1_SIZE	0x10000000	/* 256 MB */
#define CONFIG_SYS_SDRAM_BASE	PHYS_SDRAM_0
#endif
/*
 * Serial console configuration
 */
#define HIFIVE_UART_BASE_ADDR	0x10010000
#define HIFIVE_PERIPH_CLK_FREQ	(CONFIG_SYS_CLK_FREQ / 2)
#define CONSOLE_ARG				"console=ttyS0,115200\0"

/* Init Stack Pointer */
#define CONFIG_SYS_INIT_SP_ADDR		(0x08000000 + 0x001D0000 - \
					GENERATED_GBL_DATA_SIZE)

#define CONFIG_SYS_LOAD_ADDR		0x80000000	/* SDRAM */

#define HIFIVE_BASE_CLINT		0x02000000
#define HIFIVE_BASE_MTIME		0x0200bff8
#define HIFIVE_BASE_TIMECMP		0x02004000
#define HIFIVE_BASE_PRCI		0x10000000
#define HIFIVE_BASE_SPI			0x10050000
#define HIFIVE_BASE_GPIO	 	0x10060000
#define HIFIVE_BASE_ETHERNET	0x10090000
#define HIFIVE_BASE_MAC_MGMT	0x100a0000

#define HIFIVE_ETHERNET_CLK_FREQ	(CONFIG_SYS_CLK_FREQ / 2)

#define CONFIG_HIFIVE_SPI
#define CONFIG_MMC_SPI
#define CONFIG_SPI
#define CONFIG_SYS_SPI_BASE			HIFIVE_BASE_SPI
#define CONFIG_SYS_SPI_CLK			(CONFIG_SYS_CLK_FREQ / 2)
#define CONFIG_ENV_SPI_MAX_HZ		25000000

#define CONFIG_BOOTFILE			"bbl.bin"

/* DT blob (fdt) address */
#define HIFIVE_FDT_BASE		0xF0000000
/*
 * Ethernet
 */
#ifdef CONFIG_CMD_NET
#define CONFIG_MACB
#define CONFIG_GMII
#define CONFIG_NET_RETRY_COUNT		20
#define CONFIG_MACB_SEARCH_PHY
#define CONFIG_ARP_TIMEOUT		200UL
#define CONFIG_IP_DEFRAG
#define CONFIG_CMD_MII
#define CONFIG_RESET_PHY_R

#endif

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE

#define	CONFIG_EXTRA_ENV_SETTINGS	"serverip=10.60.132.52\0" \
									"ip_dyn=yes\0"

#define CONFIG_SYS_MAX_FLASH_SECT	0
#define CONFIG_SYS_MAX_FLASH_BANKS 0
/*
 * memtest works on 1.9 MB in DRAM
 */
#define CONFIG_SYS_MEMTEST_START	PHYS_SDRAM_0
#define CONFIG_SYS_MEMTEST_END		(PHYS_SDRAM_0 + PHYS_SDRAM_0_SIZE)

#define CONFIG_ENV_SIZE		0x400   /* Total Size of Environment, 128KB */


#endif /* __CONFIG_H */
