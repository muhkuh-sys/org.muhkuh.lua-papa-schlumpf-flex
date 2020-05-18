/**
 *   @file Full PCI Express test procedure
 *
 *  Contains full PCI Express test.
 */

#include <string.h>
#include "pcie_test.h"
#include "serial_vectors.h"
#include "systime.h"
#include "uprintf.h"
#include "uprintf_buffer.h"
#include "rdy_run.h"
#include "pci.h"
#include "pci_plx.h"
#include "netx_io_areas.h"


/* Start address of DMA buffer */
extern volatile unsigned long g_ul_PCI_DMA_Buffer_Start;
/* End address of DMA buffer */
extern volatile unsigned long g_ul_PCI_DMA_Buffer_End;



/* List all supported devices. */
typedef enum NETX_DEVICE_ENUM
{
	NETX_DEVICE_Invalid   = 0,
	NETX_DEVICE_CIFX50    = 1,
	NETX_DEVICE_CIFX4000  = 2
} NETX_DEVICE_T;




/*-------------------------------------------------------------------------*/

static const DMALIST_T atDmaList_SetupOnboardPlxBridge[] =
{
	/* 00: Write 0xffffffff to the memory bar 0 register. */
	{ DMAACTION_Write,   DMA_CFG0(DEVICEADDR_ONBOARD_PLX_BRIDGE, PCI_ADR_MemBar0),                    0xffffffff },

	/* 01: The memory bar 0 register must report a size of 64K. */
	{ DMAACTION_Compare, DMA_CFG0(DEVICEADDR_ONBOARD_PLX_BRIDGE, PCI_ADR_MemBar0),                    0xffff000c },

	/* 02: Clear the "Master Data Parity Error" bit in the status register.
	 *     Activate i/o access, memory space, bus master.
	 */
	{ DMAACTION_Write,   DMA_CFG0(DEVICEADDR_ONBOARD_PLX_BRIDGE, PCI_ADR_Status_Cmd),                 0x01000117 },

	/* 03: Set the cache line size and the bus latency timer.
	 *
	 *     The cache line size is at offset 0x0c, bits 0-7. Bit 8 seems to be reserved.
	 *     Set the cache line size to 0 DWORDs.
	 *
	 *     Set a bus latency of 64.
	 */
	{ DMAACTION_Write,   DMA_CFG0(DEVICEADDR_ONBOARD_PLX_BRIDGE, PCI_ADR_HdrType_LatTimer_CacheSize), 0x00004080 },

	/* 04: Set the base address of the bridge window. */
	{ DMAACTION_Write,   DMA_CFG0(DEVICEADDR_ONBOARD_PLX_BRIDGE, PCI_ADR_Bar0),                       DEVICEADDR_ONBOARD_PLX_BRIDGE_BAR0|0x0000000c },

	/* 05: Set the bus numbers.
	 *     Set the primary bus to 1.
	 *     Set the secondary bus to 2.
	 *     Set the number of the highest bus behind the PEX to 7.
	 */
	{ DMAACTION_Write,   DMA_CFG0(DEVICEADDR_ONBOARD_PLX_BRIDGE, PCI_ADR_SecLatTimer_BusNumbers),     0x40070201 },

	/* 06: Set the IO base and limit. Clear the SecStatus.
	 *     Set the IO base and limit to 0xd to enable I/O access in the area 0xd000-0xdfff.
	 */
	{ DMAACTION_Write,   DMA_CFG0(DEVICEADDR_ONBOARD_PLX_BRIDGE, PCI_ADR_SecStatus_IOLimit_IOBase),   (0xf900U<<16U)|(DEVICEADDR_CIFXM2_IO_BAR0&0xf000U)|((DEVICEADDR_CIFXM2_IO_BAR0&0xf000U)>>8U)},

	/* 07: Set the memory base and limit.
	 *
	 *     Set the memory base to 0x37600000 and the limit to 0x376FFFFF.
	 *     This forwards all accesses in the configured area to the secondary bus.
	 */
	{ DMAACTION_Write,   DMA_CFG0(DEVICEADDR_ONBOARD_PLX_BRIDGE, PCI_ADR_MemLimit_MemBase),           0x37603760 },

	/* 08: Set the prefetchable memory base and limit.
	 *
	 *     Set the prefetchable memory base to 0x37600000 and the limit to 0x376FFFFF.
	 *     This forwards all prefetchable accesses in the configured area to the secondary bus.
	 */
	{ DMAACTION_Write,   DMA_CFG0(DEVICEADDR_ONBOARD_PLX_BRIDGE, PCI_ADR_PreMemLimit_PreMemBase),     0x37603760 },

	/* 09: Set the interrupt lines and the bridge control.
	 *
	 *     Set the interrupt line to 16.
	 *     Set the interrupt pin to 1.
	 *
	 *     Enable SERR, but this has no effect according to the manual.
	 */
	{ DMAACTION_Write,   DMA_CFG0(DEVICEADDR_ONBOARD_PLX_BRIDGE, PCI_ADR_BrideCtrl_IrqPin_Irq_Line),  0x00020110 }
};



static int setup_onboard_PLX_bridge(void)
{
	return dmalist_process(atDmaList_SetupOnboardPlxBridge, sizeof(atDmaList_SetupOnboardPlxBridge)/sizeof(DMALIST_T), "Setup on-board PLX bridge");
}



static const DMALIST_T atDmaList_SetupCifxM2[] =
{
	/* 00: Check the subdevice and subvendor IDs. */
	{ DMAACTION_Compare, DMA_CFG1(BUS_CIFXM2,0,0,PCI_ADR_SubDevId_SubVendId),          0x100215cf },

	/* 01: The M2 card should have a function #3. */
	{ DMAACTION_Compare, DMA_CFG1(BUS_CIFXM2,0,3,PCI_ADR_DevId_VendId),                PCI_ID_CIFXM2 },
	/* 02: Check the subdevice and subvendor IDs of function #3. */
	{ DMAACTION_Compare, DMA_CFG1(BUS_CIFXM2,0,3,PCI_ADR_SubDevId_SubVendId),          0x600115cf },

	/* 03: Write 0xffffffff to the memory bar 0 register. */
	{ DMAACTION_Write,   DMA_CFG1(BUS_CIFXM2,0,3,PCI_ADR_MemBar0),                     0xffffffff },
	/* 04: The bar should report a size of 128 bytes. */
	{ DMAACTION_Compare, DMA_CFG1(BUS_CIFXM2,0,3,PCI_ADR_MemBar0),                     0xffffff81 },
	/* 05: Assign an address to bar 0. */
	{ DMAACTION_Write,   DMA_CFG1(BUS_CIFXM2,0,3,PCI_ADR_Bar0),                        DEVICEADDR_CIFXM2_IO_BAR0|1 },
	/* 06: Show the register contents. */
	{ DMAACTION_Compare, DMA_CFG1(BUS_CIFXM2,0,3,PCI_ADR_MemBar0),                     DEVICEADDR_CIFXM2_IO_BAR0|1 },

	/* Write 0x */
//	{ DMAACTION_Write,   DMA_CFG1(BUS_CIFXM2,0,3,PCI_ADR_MemBar1)|0x01000000,          0xffffffff },
//	{ DMAACTION_Dump,    DMA_CFG1(BUS_CIFXM2,0,3,PCI_ADR_MemBar1),                     0 },

	/* 07: Activate i/o access, memory space and bus master.
	 *     NOTE: we need a parity fix here.
	 */
	{ DMAACTION_Write,   DMA_CFG1(BUS_CIFXM2,0,3,PCI_ADR_Status_Cmd)|0x01000000,       0x00000147 },

	/* 08: try to access the IO space. */
	{ DMAACTION_Dump,    DMATYPE_IO, DEVICEADDR_CIFXM2_IO_BAR0, 0 }

#if 0
	{ DMAACTION_Write,   DMATYPE_IO, DEVICEADDR_CIFXM2_IO_BAR0+0x04, 0x01234567 },
	{ DMAACTION_Dump,    DMATYPE_IO, DEVICEADDR_CIFXM2_IO_BAR0+0x04, 0 },
	{ DMAACTION_Write,   DMATYPE_IO, DEVICEADDR_CIFXM2_IO_BAR0+0x04, 0x11234567 },
	{ DMAACTION_Dump,    DMATYPE_IO, DEVICEADDR_CIFXM2_IO_BAR0+0x04, 0 },
#endif
};



static int setup_cifxm2(void)
{
	return dmalist_process(atDmaList_SetupCifxM2, sizeof(atDmaList_SetupCifxM2)/sizeof(DMALIST_T), "Setup CIFXM2");
}



/* I/O Bar registers - SPI */

/* SPI Master Controller Registers */
#define ASIX_SPI_SPICMR     0x00 /* Control Master Register */
#define ASIX_SPI_SPICSS     0x01 /* Clock Source Select */

#define ASIX_SPI_SPIBRR     0x04 /* Baud Rate Register */
#define ASIX_SPI_SPIDS      0x05 /* Delay before SCLK */
#define ASIX_SPI_SPIDT      0x06 /* Delay transmission */
#define ASIX_SPI_SDAOF      0x07 /* Delay after OPCode field */
#define ASIX_SPI_STOF0      0x08 /* OPCode fields 0-7 */
#define ASIX_SPI_STOF1      0x09
#define ASIX_SPI_STOF2      0x0A
#define ASIX_SPI_STOF3      0x0B
#define ASIX_SPI_STOF4      0x0C
#define ASIX_SPI_STOF5      0x0D
#define ASIX_SPI_STOF6      0x0E
#define ASIX_SPI_STOF7      0x0F
#define ASIX_SPI_SDFL0      0x10 /* DMA Fragment length 0-1 */
#define ASIX_SPI_SDFL1      0x11
#define ASIX_SPI_SPISSOL    0x12 /* Slave Select Register and OPCode length */
#define ASIX_SPI_SDCR       0x13 /* DMA Command Register*/
#define ASIX_SPI_SPIMISR    0x14 /* Master interrupt status register */

/* SPICMR */
#define ASIX_SPI_SPICMR_SSP     0x01
#define ASIX_SPI_SPICMR_CPHA    0x02
#define ASIX_SPI_SPICMR_CPOL    0x04
#define ASIX_SPI_SPICMR_LSB     0x08
#define ASIX_SPI_SPICMR_SPIMEN  0x10
#define ASIX_SPI_SPICMR_ASS     0x20
#define ASIX_SPI_SPICMR_SWE     0x40
#define ASIX_SPI_SPICMR_SSOE    0x80

/* SPICSS */
#define ASIX_SPI_SPICSS_125MHZ     0x00
#define ASIX_SPI_SPICSS_100MHZ     0x01
#define ASIX_SPI_SPICSS_DIVEN      0x04
#define ASIX_SPI_SPICSS_SPICKS_MSK 0x03

/* SPISSOL */
#define ASIX_SPI_SPISSOL_SS_MSK    0x07
#define ASIX_SPI_SPISSOL_EDE       0x08
#define ASIX_SPI_SPISSOL_STOL_MSK  0x70

/* Slave Select settings without encoder (EDE == 0) */
#define SPI_SS_DEVICE0 (0x06)
#define SPI_SS_DEVICE1 (0x05)
#define SPI_SS_DEVICE2 (0x03)

/* SPICMR */
#define ETDMA   0x01 /* Transmit DMA */
#define ERDMA   0x02 /* Receive DMA */
#define OPCFE   0x04 /* OPCode enable field */
#define DMA_GO  0x08
#define FBT     0x10
#define CSS     0x20
#define STCIE   0x40 /* Interrupt enable */
#define STERRIE 0x80 /* Interrupt enable on error */

/* SPIMISR */
#define ASIX_SPI_SPIMISR_STC    0x01 /* SPI Tansceiver Complete Flag */
#define ASIX_SPI_SPIMISR_STERR  0x02 /* SPI Tansceiver Error Indication */


static TEST_RESULT_T spm_access(unsigned long *pulData)
{
	TEST_RESULT_T tResult;
	int iResult;
	unsigned long ulValue;
	unsigned long ulCombined;
	unsigned char ucData;
	unsigned int uiRetries;


	tResult = TEST_RESULT_ERROR;

	/* Set the SPI control master register. */
	ulValue  = ASIX_SPI_SPICMR_CPHA;   /* Set mode 3. */
	ulValue |= ASIX_SPI_SPICMR_CPOL;   /* Set mode 3. */
	ulValue |= ASIX_SPI_SPICMR_SPIMEN; /* Enable the master controller. */
	ulValue |= ASIX_SPI_SPICMR_SSOE;   /* Drive the chip select pins. */
	ulCombined = ulValue;
	/* Set the clock source select register. */
	ulValue  = ASIX_SPI_SPICSS_100MHZ; /* Use 100MHz. */
	ulCombined |= ulValue << 8U;
	iResult = io_register_write_32(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_SPICMR, ulCombined);

	/* Set the clock divider to 1MHz (100MHz / 100). */
	ulCombined  = 100;
	/* No delay after clip select activation. */
	ulCombined |= 0 << 8U;
	/* No delay time between chip selections. */
	ulCombined |= 0 << 16U;
	/* No delay after the opcode. */
	ulCombined |= 0 << 24U;
	iResult = io_register_write_32(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_SPIBRR, ulCombined);

	/* Set the SPI control master register. */
	ulValue  = ASIX_SPI_SPICMR_CPHA;   /* Set mode 3. */
	ulValue |= ASIX_SPI_SPICMR_CPOL;   /* Set mode 3. */
	ulValue |= ASIX_SPI_SPICMR_SPIMEN; /* Enable the master controller. */
	ulValue |= ASIX_SPI_SPICMR_SSOE;   /* Drive the chip select pins. */
	ulCombined = ulValue;
	/* Set the clock source select register. */
	ulValue  = ASIX_SPI_SPICSS_100MHZ; /* Use 100MHz. */
	ulValue |= ASIX_SPI_SPICSS_DIVEN;  /* Activate the clock divider. */
	ulCombined |= ulValue << 8U;
	iResult = io_register_write_32(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_SPICMR, ulCombined);

	/* Set the data to transfer. */
	ulValue = 0x00000180U;
	iResult = io_register_write_32(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_STOF0, ulValue);
	ulValue = 0x00000000U;
	iResult = io_register_write_32(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_STOF4, ulValue);

	/* Activate the chip select and set the transfer length. */
	ulValue  = 6U; /* Select slave 0. */
	ulValue |= 7U << 4U;  /* Transfer 8 bytes. */
	ulCombined = ulValue << 16U;
	iResult = io_register_write_32(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_SDFL0, ulCombined);

	/* Start the transfer.
	 * NOTE: this uses the old ulCombined value. */
	ulValue  = OPCFE;
	ulValue |= DMA_GO;
	ulCombined |= ulValue << 24U;
	iResult = io_register_write_32(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_SDFL0, ulCombined);

	/* Wait for transfer finish. */
	uiRetries = 65536;
	do
	{
		iResult = io_register_read_08(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_SPIMISR, &ucData);
		if( iResult!=0 )
		{
			--uiRetries;
			if( uiRetries==0 )
			{
				break;
			}
			else
			{
				ucData = 0;
			}
		}
	} while( (ucData & ASIX_SPI_SPIMISR_STC)==0 );

	if( iResult==0 )
	{
		/* Read the data. */
		iResult = io_register_read_32(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_STOF0, pulData);
		iResult = io_register_read_32(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_STOF4, pulData+1);

		/* Confirm the IRQ. */
		ulValue = ASIX_SPI_SPIMISR_STC;
		io_register_write_32(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_SPIMISR, ulValue);

		tResult = TEST_RESULT_OK;
	}

	/* Deactivate the chip select. */
	ulValue  = 7U; /* Select no slave. */
	ulCombined = ulValue << 16U;
	iResult = io_register_write_32(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_SDFL0, ulCombined);

	return tResult;
}


#if 0
static const DMALIST_T atTestList[] =
{
	/* 00: Check the VID/PID of the on-board PLX bridge. */
	{ DMAACTION_Compare, DMATYPE_Cfg0, DEVICEADDR_ONBOARD_PLX_BRIDGE,      PCI_ADR_DevId_VendId, 0,            PCI_ID_PLX_BRIDGE },

	/* 01: Access the configuration space of the CIFX 4000 (02:00.0) . */
	{ DMAACTION_Write,   DMATYPE_Mem,  DEVICEADDR_ONBOARD_PLX_BRIDGE_BAR0, 0x1064,               0,            0x80200000 },
	/* 02: Check the VID/PID of the CIFX4000 through the bridge tunnel. */
	{ DMAACTION_Compare, DMATYPE_Mem,  DEVICEADDR_ONBOARD_PLX_BRIDGE_BAR0, 0x2000 | PCI_ADR_DevId_VendId, 0,   PCI_ID_CIFX4000 },

	/* 03: Clear all errors. */
	{ DMAACTION_Write,   DMATYPE_Mem,  DEVICEADDR_ONBOARD_PLX_BRIDGE_BAR0, 0x2000|0x0104,        0,            0xffffffff },
	/* 04: Show the clean status register. */
	{ DMAACTION_Dump,    DMATYPE_Mem,  DEVICEADDR_ONBOARD_PLX_BRIDGE_BAR0, 0x2000|0x0104,        0,            0 },

	/* 05: working write. */
	{ DMAACTION_Write,   DMATYPE_Mem,  DEVICEADDR_CIFX4000_BAR0,           0x0704,               0,            0x00000003 },

	/* 06: Show the status register after the working write. */
	{ DMAACTION_Compare, DMATYPE_Mem,  DEVICEADDR_ONBOARD_PLX_BRIDGE_BAR0, 0x2000|0x0104,        0,            0x00000000 },

	/* 07: Read. */
	{ DMAACTION_Compare, DMATYPE_Mem,  DEVICEADDR_CIFX4000_BAR0,           0x0704,               0,            0x00000003 },
};



static int pci_test_cifx4000(void)
{
	return dmalist_process(atTestList, sizeof(atTestList)/sizeof(DMALIST_T), "Test");
}


static TEST_RESULT_T probe_netx(NETX_DEVICE_T tNetxDevice)
{
	TEST_RESULT_T tResult;
	int iResult;
	unsigned long ulDeviceAdr;
	unsigned long ulValue;


	/* Be pessimistic. */
	tResult = TEST_RESULT_ERROR;

	ulDeviceAdr = 0x376F0000;

	switch(tNetxDevice)
	{
	case NETX_DEVICE_Invalid:
		break;

	case NETX_DEVICE_CIFX50:
		/* check netX System Status register NETX_SYS_STAT */
		*g_pul_PCI_DMA_Buffer_Start = 0x00000000;
		iResult = pciDma_MemRead(ulDeviceAdr, PCI_ADR_NetX_Sys_Stat, g_pul_PCI_DMA_Buffer_Start, 1);
		if( iResult!=RESULT_OK )
		{
			uprintf("error: There is no cookie :( , Result: %d\n", iResult);
		}
		else
		{
			ulValue = *g_pul_PCI_DMA_Buffer_Start;
			uprintf("netX SYS_STA = 0x%08x\n", ulValue);

			/* mask bit 16 and 17 */
			ulValue &= 0xfffcffff; // (LED input kann durch externe Kapazitaeten einen Spannungspegel erhalten)
			if( ulValue!=NETX_SYS_STAT_CONTENT )
			{
				/* no cookie found */
				uprintf("error: No Cookie found :(\n");
			}
			else
			{
				uprintf("Cookie found OK!\n\n");
				tResult = TEST_RESULT_OK;
			}
		}
		break;

	case NETX_DEVICE_CIFX4000:
		/* Read the netX4000 version register. It is at offset 0xfc. */
		*g_pul_PCI_DMA_Buffer_Start = 0x00000000;
		iResult = pciDma_MemRead(ulDeviceAdr, 0xfc, g_pul_PCI_DMA_Buffer_Start, 1);
		if( iResult!=RESULT_OK )
		{
			uprintf("Failed to read the netX4000 version register: %d\n", iResult);
		}
		else
		{
			ulValue = *g_pul_PCI_DMA_Buffer_Start;
			uprintf("netX4000 version register = 0x%08x\n", ulValue);

			/* Be pessimistic. */
			iResult = -1;
			if( ulValue==HBOOT_NETX4000RELAXED_DPM_VERSION_COOKIE )
			{
				iResult = 0;
				uprintf("Found netX4000 RELAXED\n");
			}
			else if( ulValue==HBOOT_NETX4000FULL_DPM_VERSION_COOKIE )
			{
				iResult = 0;
				uprintf("Found netX4000 FULL\n");
			}
			else if( ulValue==HBOOT_NETX4000SMALL_DPM_VERSION_COOKIE )
			{
				iResult = 0;
				uprintf("Found netX4000 SMALL\n");
			}
			else
			{
				uprintf("Unknown magic found.\n");
			}

			if( iResult==0 )
			{
				*g_pul_PCI_DMA_Buffer_Start = 0x00000000;
				iResult = pciDma_MemRead(ulDeviceAdr, 0x100, g_pul_PCI_DMA_Buffer_Start, 1);
				if( iResult!=RESULT_OK )
				{
					uprintf("Failed to read the netX4000 DPM cookie: %d\n", iResult);
				}
				else
				{
					ulValue = *g_pul_PCI_DMA_Buffer_Start;
					uprintf("netX4000 DPM ID = 0x%08x\n", ulValue);
					if( ulValue!=HBOOT_DPM_ID_LISTENING )
					{
						uprintf("The netX DPM is not listening.\n");
					}
					else
					{
						uprintf("Found a valid netX4000 DPM.\n");
						tResult = TEST_RESULT_OK;
					}
				}
			}
		}
		break;
	}

	return tResult;
}
#endif


#if 0
/* PCI configuration and configuration of the slave. */
static TEST_RESULT_T test_pci_setup(void)
{
	unsigned long aulDevData[16];
	unsigned long ulDeviceAdr;
	int iResult;
	unsigned long ulPciId;
	TEST_RESULT_T tResult;


	/* Device configuration */
	iResult = pciNetXDeviceConfigRead();

	if (iResult == 0)
	{
		uprintf("Device config failed!\n");
		return TEST_RESULT_ERROR;
	}

	uprintf("\nDevice config OK!\n");

	/* Scan for devices on bus */
	*g_pul_PCI_DMA_Buffer_Start = 0x00000000;
	iResult = pciDeviceScan(aulDevData, g_pul_PCI_DMA_Buffer_Start);

	if (iResult != RESULT_OK)
	{
		uprintf("Device scan failed!\n");
		return TEST_RESULT_ERROR;
	}

	uprintf("Device scan OK!\n");

	/* look for PLX bridge */
	ulDeviceAdr = pciGetDeviceAddr(PCI_ID_PLX_BRIDGE, aulDevData);

	if (ulDeviceAdr == 0)
	{
		/* error: no bridge found! */
		uprintf("error: no bridge found!\n");
		return TEST_RESULT_ERROR;
	}

	uprintf("PLX Bridge found at 0x%08x.\n", ulDeviceAdr);
	if( ulDeviceAdr!=DEVICEADDR_ONBOARD_PLX_BRIDGE )
	{
		/* error: the PLX bridge appeared at an unexpected address */
		uprintf("error: the PLX bridge appeared at an unexpected address!\n");
		return TEST_RESULT_ERROR;
	}

	tResult = setup_onboard_PLX_bridge();
	if( tResult!=TEST_RESULT_OK )
	{
		return tResult;
	}


	/* ***** Bus 0x02 ***** */

	/* At this point we are leaving the Papa Schlumpf board.
	 * Look for the device connected to the PLX bridge.
	 */

	uprintf("Looking for the device connected to the Papa Schlumpf.\n");
#if 0
	/* Loop over all devices as the ASIX card has 2 instances. */
	ulDeviceAdr = 0x00000000;
	unsigned int uiCnt = 0;
	do
	{
		unsigned long ulID;
		unsigned long ulSubId;

		/* Clear the DMA buffer. */
		*g_pul_PCI_DMA_Buffer_Start = 0x00000000;

		/* Try to read the vendor and device ID of the connected device. */
		iResult = pciDma_CfgRead_Type1(ulDeviceAdr, PCI_ADR_DevId_VendId, g_pul_PCI_DMA_Buffer_Start, 2, 1);
		if (iResult != RESULT_OK)
		{
//			uprintf("Error reading at 0x%08x.", ulDeviceAdr);
		}
		else
		{
			ulID = *g_pul_PCI_DMA_Buffer_Start;

			/* Clear the DMA buffer. */
			*g_pul_PCI_DMA_Buffer_Start = 0x00000000;

			/* Try to read the vendor and device ID of the connected device. */
			iResult = pciDma_CfgRead_Type1(ulDeviceAdr, PCI_ADR_SubDevId_SubVendId, g_pul_PCI_DMA_Buffer_Start, 2, 1);
			if (iResult != RESULT_OK)
			{
//				uprintf("Error reading at 0x%08x.", ulDeviceAdr);
			}
			else
			{
				ulSubId = *g_pul_PCI_DMA_Buffer_Start;
				if( ulSubId!=0x100215cf && ulSubId!=0 && ulSubId!=0xffffffff )
				{
					uprintf("At 0x%08x: %08x/%08x\n", ulDeviceAdr, ulID, ulSubId);
				}
			}
		}

		++uiCnt;
		ulDeviceAdr += 0x00000100;
	} while( ulDeviceAdr!=0 );
	uprintf("checked %d\n", uiCnt);
#endif
#if 1
	/* start type 1 transaction */
	*g_pul_PCI_DMA_Buffer_Start = 0x00000000;

	/* Try to read the vendor and device ID of the connected device. */
	iResult = pciDma_CfgRead_Type1(CFG1ADR(BUS_CIFXM2,0,0,PCI_ADR_DevId_VendId), g_pul_PCI_DMA_Buffer_Start, 1);
	if (iResult != RESULT_OK)
	{
		uprintf("Error: failed to read the ID of the device connected to the PLX bridge!\n");
		return TEST_RESULT_ERROR;
	}

	ulPciId = *g_pul_PCI_DMA_Buffer_Start;
	uprintf("Scan after PLX bridge returned ID: 0x%08x\n", ulPciId);
	if( ulPciId==PCI_ID_CIFXM2 )
	{
		uprintf("Found a cifXM2 card with already programmed EEPROM.\n");

		/* The DUT is a CIFXM2 card. */
		tResult = setup_cifxm2();
		if( tResult==TEST_RESULT_OK )
		{
			union {
				unsigned char auc[8];
				unsigned long aul[2];
			} uData;

			/* Test the SPM connection to the netX90. */

			tResult = spm_access(uData.aul);
			tResult = spm_access(uData.aul);
			tResult = spm_access(uData.aul);
			tResult = spm_access(uData.aul);
#if 0
			tResult = probe_netx(NETX_DEVICE_CIFXM2);
			if( tResult==TEST_RESULT_OK )
			{
				iResult = pci_test_cifxm2();
				if( iResult!=RESULT_OK )
				{
					uprintf("error: test procedure failed!\n");
					tResult = TEST_RESULT_ERROR;
				}
				else
				{
					uprintf("\n\nTest complete!\n\n");
				}
			}
#endif
		}
	}
#if 0
	else if( ulPciId==PCI_ID_ASIX )
	{
		uprintf("Found a blank ASIX device. Assume that this is a cifXM2 card with an empty EEPROM.\n");

		// TODO: Setup the device...

		// TODO: Program the EEPROM...
	}
#endif
	else
	{
		uprintf("Unknown ID!\n");
		tResult = TEST_RESULT_ERROR;
	}
#endif

	return tResult;
}


/*-------------------------------------------------------------------------*/
/*                                                                         */
/*-------------------------------------------------------------------------*/

/* Set master part on the netX, reset slave and start test */
static TEST_RESULT_T test_pci(void)
{
	TEST_RESULT_T tResult;
	unsigned long ulPciWindowStart;


	/* Set PCI mode on netX. */
	pciModeEnable();

	/* reset PCI core */
	pciReset(500, 1, 10000);

	/* configure arbiter */
	pciArbConfig();

	ulPciWindowStart = (unsigned long) g_pul_PCI_DMA_Buffer_Start;
	pciSetMemoryWindow0(ulPciWindowStart, 0x008000);

	tResult = test_pci_setup();

	return tResult;
}

/*-------------------------------------------------------------------------*/

/* Test function. */

TEST_RESULT_T test(void)
{
	TEST_RESULT_T tTestResult;
	HOSTDEF(ptPioArea);

	/* Set PIOs 0-3 to their default state */
	ptPioArea->ulPio_oe &= ~(15U);

	/* Run the test cases. */
	tTestResult = test_pci();

	if (tTestResult != TEST_RESULT_OK)
	{
		/* Test failed. */
		ptPioArea->ulPio_out &= ~(15U);
		ptPioArea->ulPio_oe  |= ~((1U<<0U)|(1U<<2U));
	}
	else
	{
		/* Test OK. */
		ptPioArea->ulPio_out &= ~(15U);
		ptPioArea->ulPio_oe  |= ~((1U<<1U)|(1U<<3U));
	}

	return tTestResult;
}
#endif
