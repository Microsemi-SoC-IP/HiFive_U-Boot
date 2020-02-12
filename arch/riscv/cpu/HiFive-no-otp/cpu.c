// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2018 Microsemi Corporation.
 * Copyright (c) 2018 Padmarao Begari <Padmarao.Begari@microsemi.com>
 */

/* CPU specific code */
#include <common.h>
#include <config.h>
#include <command.h>
#include <watchdog.h>
#include <asm/cache.h>
#include <asm/ddrregs.h>
#include <linux/io.h>

struct hifive_prci_regs
{
    volatile uint32_t  HFXOSCCFG;      /* 0x0000 */
    volatile uint32_t  COREPLLCFG0;    /* 0x0004 */
    volatile uint32_t  COREPLLOUT;     /* 0x0008 */
    volatile uint32_t  DDRPLLCFG0;     /* 0x000C */
    volatile uint32_t  DDRPLLCFG1;     /* 0x0010 */
    volatile uint32_t  reserved1;      /* 0x0014 */
    volatile uint32_t  reserved2;      /* 0x0018 */
    volatile uint32_t  GEMGXLPLLCFG0;  /* 0x001C */
    volatile uint32_t  GEMGXLPLLCFG1;  /* 0x0020 */
    volatile uint32_t  CORECLKSEL;     /* 0x0024 */
    volatile uint32_t  DEVICERESETREG; /* 0x0028 */
    volatile uint32_t  CLKMUXSTATUSREG; /* 0x002C */
    volatile uint32_t  RESERVED[48]; /* 0x0028 */
    volatile uint32_t  PROCMONCFG; 	/* 0x00F0 */
};

struct hifive_prci_regs *g_aloe_prci = (struct hifive_prci_regs *)HIFIVE_BASE_PRCI;

static void ddr_writeregmap(const uint32_t *ctlsettings, const uint32_t *physettings);
static void ddr_phy_reset(volatile uint32_t *ddrphyreg,const uint32_t *physettings);
static void ddr_setuprangeprotection(size_t end_addr);
static uint64_t ddr_phy_fixup(void);

#define REG32(p, i) (*(volatile uint32_t *)((p) + (i)))
#define DDR_CTRL_ADDR 0x100B0000UL
#define DDR_SIZE  (8UL * 1024UL * 1024UL * 1024UL)
#define ahbregaddr DDR_CTRL_ADDR

int arch_cpu_init(void)
{

	const uint64_t ddr_end = CONFIG_SYS_SDRAM_BASE + DDR_SIZE;
	uint32_t regdata;
    volatile int ix;

	g_aloe_prci->COREPLLCFG0 = 0x02110EC0u; /* Take PLL out of bypass */
	while((g_aloe_prci->COREPLLCFG0 & 0x80000000u) == 0); /* Wait for lock with PLL bypassed */

	g_aloe_prci->COREPLLOUT  = 0x80000000u;
	g_aloe_prci->CORECLKSEL  = 0x00000000u; /* Switch to PLL as clock source */

	g_aloe_prci->DDRPLLCFG0 = 0x2110DC0u;

	while((g_aloe_prci->DDRPLLCFG0 & 0x80000000u) == 0); /* Wait for lock with PLL bypassed */

	g_aloe_prci->DDRPLLCFG1 = 0x80000000u;

	g_aloe_prci->DEVICERESETREG |= 0x00000001u; /* Release DDR CTRL from reset */

	asm volatile ("fence"); // HACK to get the '1 full controller clock cycle'.
	g_aloe_prci->DEVICERESETREG |= 0x0000000Eu;
	asm volatile ("fence"); /* HACK to get the '1 full controller clock cycle'. */
	/* These take like 16 cycles to actually propogate. We can't go sending stuff before they
	 come out of reset. So wait. (TODO: Add a register to read the current reset states, or DDR Control device?)*/
	for (int i = 0; i < 256; i++)
	{
	    asm volatile ("nop");
	}
	ddr_writeregmap(DENALI_CTL_DATA,DENALI_PHY_DATA);

	REG32(120<<2,ahbregaddr) |= (1<<16);
	REG32(21<<2,ahbregaddr) &= (~(1<<0));
	REG32(170<<2,ahbregaddr) |= ((1<<0) | (1<<24));

	REG32(181<<2,ahbregaddr) |= (1<<24);
	REG32(260<<2,ahbregaddr) |= (1<<16);
	REG32(260<<2,ahbregaddr) |= (1<<24);
	REG32(182<<2,ahbregaddr) |= (1<<0);
	if(((REG32(0,DDR_CTRL_ADDR) >> 8) & 0xF) == 0xA)
	{
		REG32(184<<2,ahbregaddr) |= (1<<24);
	}
	/* Mask off Bit 22 of Interrupt Status
	   Bit [22] The leveling operation has completed */
	REG32(136<<2,ahbregaddr) |= (1<<22);
	/* Mask off Bit 8 of Interrupt Status
	   Bit [8] The MC initialization has been completed */
	REG32(136<<2,ahbregaddr) |= (1<<8);
	/* Mask off Bit 8, Bit 2 and Bit 1 of Interrupt Status
	   Bit [2] Multiple accesses outside the defined PHYSICAL memory space have occured
	   Bit [1] A memory access outside the defined PHYSICAL memory space has occured */
	REG32(136<<2,ahbregaddr) |= ((1<<1) | (1<<2));

	ddr_setuprangeprotection(DDR_SIZE);

	/* Mask off Bit 7 of Interrupt Status
	   Bit [7] An error occured on the port command channel */
	REG32(136<<2,ahbregaddr) |= (1<<7);

	/* START register at ddrctl register base offset 0 */
	regdata = REG32(0<<2,DDR_CTRL_ADDR);
	regdata |= 0x1;
	(*(volatile uint32_t *)(DDR_CTRL_ADDR)) = regdata;
	/* WAIT for initialization complete : bit 8 of INT_STATUS (DENALI_CTL_132) 0x210 */
	while((REG32(132<<2,DDR_CTRL_ADDR) & (1<<8)) == 0) {}
	/* Disable the BusBlocker in front of the controller AXI slave ports */
	volatile uint64_t *filterreg = (volatile uint64_t *)0x100b8000UL;
	filterreg[0] = 0x0f00000000000000UL | (ddr_end >> 2);
	/*                ^^ RWX + TOR */
	ddr_phy_fixup();

	g_aloe_prci->GEMGXLPLLCFG0 = 0x03128EC0u; /* Configure Core Clock PLL */
	while(g_aloe_prci->GEMGXLPLLCFG0 & 0x80000000u) /* Wait for lock with PLL bypassed */
	ix++;

	g_aloe_prci->GEMGXLPLLCFG0 = 0x02128EC0u; /* Take PLL out of bypass */
	g_aloe_prci->GEMGXLPLLCFG1  = 0x80000000u; /* Switch to PLL as clock source */
	g_aloe_prci->DEVICERESETREG |= 0x00000020u; /* Release MAC from reset */
	g_aloe_prci->PROCMONCFG = 0x1 << 24u;
	return 0;

}
static void ddr_setuprangeprotection(size_t end_addr)
{
	REG32(209<<2,ahbregaddr) = 0x0;
	size_t end_addr_16Kblocks = ((end_addr >> 14) & 0x7FFFFF)-1;

	REG32(210<<2,ahbregaddr) = ((uint32_t) end_addr_16Kblocks);
	REG32(212<<2,ahbregaddr) = 0x0;
	REG32(214<<2,ahbregaddr) = 0x0;
	REG32(216<<2,ahbregaddr) = 0x0;
	REG32(224<<2,ahbregaddr) |= (0x3 << 24);
	REG32(225<<2,ahbregaddr) = 0xFFFFFFFF;
	REG32(208<<2,ahbregaddr) |= (1 << 8);
	REG32(208<<2,ahbregaddr) |= (1 << 0);
}

static uint64_t ddr_phy_fixup(void)
{
  size_t ddrphyreg = ahbregaddr + 0x2000;

  uint64_t fails=0;
  uint32_t slicebase = 0;
  uint32_t dq = 0;

  /* check errata condition */
  for(uint32_t slice = 0; slice < 8; slice++) {
    uint32_t regbase = slicebase + 34;
    for(uint32_t reg = 0 ; reg < 4; reg++) {
      uint32_t updownreg = REG32((regbase+reg)<<2,ddrphyreg);
      for(uint32_t bit = 0; bit < 2; bit++) {
        uint32_t phy_rx_cal_dqn_0_offset;

        if(bit==0){
          phy_rx_cal_dqn_0_offset = 0u;
        }else{
          phy_rx_cal_dqn_0_offset = 16u;
        }

        uint32_t down = (updownreg >> phy_rx_cal_dqn_0_offset) & 0x3F;
        uint32_t up = (updownreg >> (phy_rx_cal_dqn_0_offset+6)) & 0x3F;

        uint8_t failc0 = ((down == 0) && (up == 0x3F));
        uint8_t failc1 = ((up == 0) && (down == 0x3F));

        /* error on failure */
        if(failc0 || failc1){
          if(fails==0){
        	/*  "DDR error in fixing up */
          }
          fails |= (1<<dq);
          char slicelsc = '0';
          char slicemsc = '0';
          slicelsc += (dq % 10);
          slicemsc += (dq / 10);
        }
        dq++;
      }
    }
    slicebase+=128;
  }
  return (0);
}


static void ddr_phy_reset(volatile uint32_t *ddrphyreg,const uint32_t *physettings)
{
  unsigned int i;
  for(i=1152;i<=1214;i++)
  {
    uint32_t physet = physettings[i];
    /*if(physet!=0)*/ ddrphyreg[i] = physet;
  }
  for(i=0;i<=1151;i++)
  {
    uint32_t physet = physettings[i];
    /*if(physet!=0)*/ ddrphyreg[i] = physet;
  }
}


static void ddr_writeregmap(const uint32_t *ctlsettings, const uint32_t *physettings)
{

  volatile uint32_t *ddrctlreg = (volatile uint32_t *) ahbregaddr;
  volatile uint32_t *ddrphyreg = ((volatile uint32_t *) ahbregaddr) + (0x2000 / sizeof(uint32_t));

  unsigned int i;
  for(i=0;i<=264;i++){
    uint32_t ctlset = ctlsettings[i];
    /*if(ctlset!=0)*/ ddrctlreg[i] = ctlset;
  }

  ddr_phy_reset(ddrphyreg,physettings);

}

/*
 * cleanup_before_linux() is called just before we call linux
 * it prepares the processor for linux
 *
 * we disable interrupt and caches.
 */
int cleanup_before_linux(void)
{
	disable_interrupts();

	/* turn off I/D-cache */

	return 0;
}

int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	disable_interrupts();
	panic("HiFive-U540 wdt not support yet.\n");
}
