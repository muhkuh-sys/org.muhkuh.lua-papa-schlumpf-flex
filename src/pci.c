/**
 * @file pci.c PCI configuration file.
 *
 * Configures PCI registers.
 *
 */

#include "defines.h"
#include "netx_io_areas.h"
#include "pci.h"
#include "uprintf.h"
#include "systime.h"
#include <string.h>


/* Startaddress of DMA buffer */
extern volatile unsigned long g_ul_PCI_DMA_Buffer_Start[];
/* Endaddress of DMA buffer */
extern volatile unsigned long g_ul_PCI_DMA_Buffer_End[];

/* Pointer to start address of DMA buffer */
volatile unsigned long *g_pul_PCI_DMA_Buffer_Start;
/* Pointer to end address of DMA buffer */
volatile unsigned long *g_pul_PCI_DMA_Buffer_End;

//-------------------------------------

/**
 * Wait for 100us.
 *
 * @param uDelay100Us Waiting period
 *
 */

void delay100US(unsigned int uDelay100Us)
{
	HOSTDEF(ptHostControlledGlobalRegisterBlockArea);
	unsigned long ulValue;


	// set timeout value
	ptHostControlledGlobalRegisterBlockArea->ulCyclic_tmr_reload = uDelay100Us;

	// enable timer
	ptHostControlledGlobalRegisterBlockArea->ulCyclic_tmr_control = 0x8000;

	// wait for timer finish
	do
	{
		ulValue = ptHostControlledGlobalRegisterBlockArea->aulIrq_status__host[0];
		ulValue &= 1<<25;
	} while( ulValue==0 );

	ptHostControlledGlobalRegisterBlockArea->aulIrq_status__host[0] = 1<<25;
}

//-------------------------------------

void pciInitGlobals(void)
{
	g_pul_PCI_DMA_Buffer_Start = &g_ul_PCI_DMA_Buffer_Start[0];
	g_pul_PCI_DMA_Buffer_End = &g_ul_PCI_DMA_Buffer_End[0];
}


/**
 * Enable PCI mode in netX.
 */

void pciModeEnable(void)
{
	HOSTDEF(ptAsicCtrlArea);
	unsigned long ulValue;


	/* Enable PCI mode in the io_config register. */
	ulValue  = ptAsicCtrlArea->ulIo_config;
	ulValue |= MSK_NX500_io_config_if_select_n;
	/* Unlock write access to the io_config register. */
	ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;  /* @suppress("Assignment to itself") */
	ptAsicCtrlArea->ulIo_config = ulValue;
}


/**
 * Disable PCI Mode in netX.
 */

void pciModeDisable(void)
{
	HOSTDEF(ptAsicCtrlArea);
	unsigned long ulKey, ulValue;

	// TODO: no IRQs

	// enable PCI mode in io config
	ulValue = ptAsicCtrlArea->ulIo_config;

	// enable writing io config
	ulKey = ptAsicCtrlArea->ulAsic_ctrl_access_key;

	ptAsicCtrlArea->ulAsic_ctrl_access_key = ulKey;

	// write new value to io config
	ptAsicCtrlArea->ulIo_config = ulValue&(~MSK_NX500_io_config_if_select_n);

	// TODO: reenable IRQs
}

//-------------------------------------

int pciSetupNetx(void)
{
	unsigned long ulPciWindowStart;
	int iResult;
	int iDevConfigResult;


	/* Be optimistic. */
	iResult = 0;

	/* configure arbiter */
	pciArbConfig();

	ulPciWindowStart = (unsigned long) g_pul_PCI_DMA_Buffer_Start;
	pciSetMemoryWindow0(ulPciWindowStart, 0x008000);

	iDevConfigResult = pciNetXDeviceConfigRead();
	if( iDevConfigResult==0 )
	{
		iResult = -1;
	}

	return iResult;
}


int pciResetAndInit(unsigned int uRstActiveToClock, unsigned int uRstActiveDelayAfterClock, unsigned int uBusIdleDelay)
{
	unsigned long ulPciWindowStart;
	int iResult;
	int iDevConfigResult;


	/* Be optimistic. */
	iResult = 0;

	pciReset(uRstActiveToClock, uRstActiveDelayAfterClock, uBusIdleDelay);

	/* configure arbiter */
	pciArbConfig();

	ulPciWindowStart = (unsigned long) g_pul_PCI_DMA_Buffer_Start;
	pciSetMemoryWindow0(ulPciWindowStart, 0x008000);

	iDevConfigResult = pciNetXDeviceConfigRead();
	if( iDevConfigResult==0 )
	{
		iResult = -1;
	}

	return iResult;
}


/**
 * Reset PCI.
 *
 * @param uRstActiveToClock
 * @param uRstActiveDelayAfterClock
 * @param uBusIdleDelay
 */

void pciReset(unsigned int uRstActiveToClock, unsigned int uRstActiveDelayAfterClock, unsigned int uBusIdleDelay)
{
	HOSTDEF(ptNetxControlledGlobalRegisterBlock2Area);
	HOSTDEF(ptAsicCtrlArea);

	unsigned long ulKey, ulValue;


	// switch clock off but enable driver
	ptNetxControlledGlobalRegisterBlock2Area->ulClk_reg = 0x80000000;

	// activate reset (to low)
	// TODO: no IRQs
	// get old value
	ulValue = ptAsicCtrlArea->ulReset_ctrl;

	ulValue |= reset_ctrl_RSTOUTn|reset_ctrl_EN_RSTOUTn;

	// enable writing io config
	ulKey = ptAsicCtrlArea->ulAsic_ctrl_access_key;

	ptAsicCtrlArea->ulAsic_ctrl_access_key = ulKey;

	// write value
	ptAsicCtrlArea->ulReset_ctrl = ulValue;

	// TODO: reenable IRQs

	// delay 50ms (==500 *100us)
	delay100US(uRstActiveToClock);

	// activate clock
	//ptNetxControlledGlobalRegisterBlock2Area->ulClk_reg =  0x80000000|uFreqValue;

	// delay 100us
	delay100US(uRstActiveDelayAfterClock);

	// deactivate reset
	// activate reset (to low)
	// TODO: no IRQs
	// get old value
	ulValue = ptAsicCtrlArea->ulReset_ctrl;

	ulValue &= (~(unsigned)reset_ctrl_RSTOUTn);
	ulValue |= reset_ctrl_EN_RSTOUTn;
	// enable writing io config
	ulKey = ptAsicCtrlArea->ulAsic_ctrl_access_key;

	ptAsicCtrlArea->ulAsic_ctrl_access_key = ulKey;

	// write value
	ptAsicCtrlArea->ulReset_ctrl = ulValue;


	// TODO: reenable IRQs

	// delay 1s (==10000 * 100us)
	delay100US(uBusIdleDelay);
}



/**
 * Reset PCI.
 *
 * @param uRstActiveToClock
 * @param uRstActiveDelayAfterClock
 * @param uBusIdleDelay
 */

void pciSetPciReset(unsigned long ulResetState)
{
	HOSTDEF(ptNetxControlledGlobalRegisterBlock2Area);
	HOSTDEF(ptAsicCtrlArea);
	unsigned long ulValue;


	ulValue  = ptAsicCtrlArea->ulReset_ctrl;
	if( ulResetState!=0 )
	{
		/* Switch clock off but enable driver. */
		ptNetxControlledGlobalRegisterBlock2Area->ulClk_reg = 0x80000000;

		/* Assert the PCI reset. */
		ulValue |= HOSTMSK(reset_ctrl_RES_REQ_OUT);
		ulValue |= HOSTMSK(reset_ctrl_EN_RES_REQ_OUT);
	}
	else
	{
		/* De-assert the PCI reset. */
		ulValue &= ~(HOSTMSK(reset_ctrl_RES_REQ_OUT));
		ulValue |= HOSTMSK(reset_ctrl_EN_RES_REQ_OUT);
	}
	ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;
	ptAsicCtrlArea->ulReset_ctrl = ulValue;
}


//-------------------------------------

/**
* Read specific netX device configuration.
*
* Writes PCI command register.
* Writes latency timer.
* Sets target tready timeout.
* Sets DMA burst length.
*/

int pciNetXDeviceConfigRead(void)
{
	HOSTDEF(ptNetxControlledGlobalRegisterBlock1Area);

	int iResult;

	// write pci command register
	iResult = pciWriteReq(0x00030004, 0x00000147);
	if( iResult==0 ) {
		return iResult;
	}

	// write latency timer, maximum set to 80 dwords
	iResult = pciWriteReq(0x0002000c, 0x00008000);
	if( iResult==0 ) {
		return iResult;
	}

	// set target tready timeout to 32 clocks
	iResult = pciWriteReq(0x000100ec, 0x000000a0);
	if( iResult==0 ) {
		return iResult;
	}

	// set dma burst length in dword to 16 dwords
	iResult = pciWriteReq(0x000100e8, 0x00000010);
	if( iResult==0 ) {
		return iResult;
	}

	ptNetxControlledGlobalRegisterBlock1Area->ulPci_config = 0x00010001;

	return iResult;
}

//-------------------------------------

/**
 *  Configure arbiter.
 */


void pciArbConfig(void)
{
	HOSTDEF(ptNetxControlledGlobalRegisterBlock1Area);

	ptNetxControlledGlobalRegisterBlock1Area->ulArb_ctrl = 0x00000080;

}

//-------------------------------------

/**
 * Set PCI memory window.
 *
 * @param ulStartAdr 	Start address
 * @param ulLength		Length of address
 */

void pciSetMemoryWindow0(unsigned long ulStartAdr, unsigned long ulLength)
{
	HOSTDEF(ptNetxControlledGlobalRegisterBlock1Area);

	// restrict values
	ulStartAdr &= 0xffffff00;
	ulLength |= 0xff;

	// reserved area start and end
	ptNetxControlledGlobalRegisterBlock1Area->ulPci_window_low0 = ulStartAdr;

	ptNetxControlledGlobalRegisterBlock1Area->ulPci_window_high0 = (ulStartAdr+ulLength)|1;

}

//-------------------------------------

/**
 * Swap bit 0 and bit 30.
 *
 * @param ulDeviceAdr	32 Bit where bits should getting swapped
 */

unsigned long swapBit0_Bit30(unsigned long ulDeviceAdr)
{
	unsigned int val0  = (ulDeviceAdr & MSK_BIT0) >> SRT_BIT0; // check and shift Bit 0 on position 0
	unsigned int val30 = (ulDeviceAdr & MSK_BIT30) >> SRT_BIT30; // check and shift Bit 30 on position 0

	// Clear bit 0 and bit 30.
	ulDeviceAdr &= ~(MSK_BIT0|MSK_BIT30);

	// Combine the bits in other order.
	ulDeviceAdr |= val0 << SRT_BIT30;
	ulDeviceAdr |= val30 << SRT_BIT0;

	return ulDeviceAdr;
}

void swapBi0_Bit30_array(volatile unsigned long *aulData, unsigned long ulCount)
{
	for(unsigned int i = 0; i < ulCount; i++)
	{
		aulData[i] = swapBit0_Bit30(aulData[i]);
	}
}

//-------------------------------------

/**
 * Set PCI start address.
 * Set netX start address.
 * Set DMA control.
 *
 * @param uPciStartAdr         Fix start address
 * @param *pulNetxMemStartAdr  Start address of netX memory
 * @param uDmaCtrl             DMA control bits
 */

int pciDma_Ch0(unsigned int uPciStartAdr, volatile unsigned long *pulNetxMemStartAdr, unsigned int uDmaCtrl)
{
	HOSTDEF(ptNetxControlledDmaRegisterBlockArea);
	unsigned long ulResult;
	unsigned long ulStatus;
	int iResult;
	unsigned long ulTimer;
	int iIsElapsed;


	/* Be optimistic. */
	iResult = RESULT_OK;

	// set pci start address
	uPciStartAdr = swapBit0_Bit30(uPciStartAdr); // swap start address
	//uprintf("Address: 0x%08x\n", uPciStartAdr);
	ptNetxControlledDmaRegisterBlockArea->asDpmas_ch[0].ulHost_start = uPciStartAdr;

	// set netx start address
	ptNetxControlledDmaRegisterBlockArea->asDpmas_ch[0].ulNetx_start = (unsigned long)pulNetxMemStartAdr;

	// set dma ctrl
	ptNetxControlledDmaRegisterBlockArea->asDpmas_ch[0].ulDma_ctrl = uDmaCtrl;

	ulTimer = systime_get_ms();
	do
	{
		iIsElapsed = systime_elapsed(ulTimer, 1000);
		if( iIsElapsed!=0 )
		{
			uprintf("Timeout waiting for the DMA to finish.\n");
			iResult = RESULT_DMA_ERROR;
			break;
		}
		else
		{
			ulResult = ptNetxControlledDmaRegisterBlockArea->asDpmas_ch[0].ulDma_ctrl;
		}
	} while( (ulResult&MSK_DPMAS_NETX_DMA_CTRL_DONE)==0 ); // wait until "done" gets set

	if( iResult==RESULT_OK )
	{
		/* Extract the status field. */
		ulStatus = (ulResult & MSK_DPMAS_NETX_DMA_CTRL_STATUS) >> SRT_DPMAS_NETX_DMA_CTRL_STATUS;

		if( ulStatus==0 )
		{
			iResult = RESULT_OK;
		}
		else if( ulStatus==VAL_DPMAS_NETX_DMA_CTRL_STATUS_DMA_transfer_parity_error )
		{
			iResult = RESULT_DMA_PARITY_ERROR;
		}
		else
		{
			iResult = RESULT_DMA_ERROR;
		}
	}

	return iResult;
}

//-------------------------------------

/**
 * Do a PCI read request.
 *
 * @param uWrCtrl	Write control
 * @param *uData 	Pointer to the data.
 *
 * @return uDelay 0 if read request failed, !=0 if read request succeeded
 */

int pciReadReq(unsigned int uWrCtrl, unsigned int *uData)
{
	unsigned int uResult = 0;
	unsigned int uDelay;


	// RD_REQ, PCI_Cfg_Adr 4
	POKE(0x00103700, (1<<20)|uWrCtrl);

	uDelay = 8;
	do {
		if( (PEEK(0x00103700)&(1<<21))!=0 ) {
		uResult = PEEK(0x00103704);
		if( uData!=NULL ) {
			*uData = uResult;
		}
		break;
    }
  } while( --uDelay!=0 );

  return (uDelay!=0);
}

//-------------------------------------

/**
 * Do a PCI write request.
 *
 * @param uWrCtrl  Write control
 * @param uData    Data to write
 *
 * @return uDelay  0 if write request failed, !=0 if write request succeeded
 */

int pciWriteReq(unsigned int uWrCtrl, unsigned int uData)
{
		HOSTDEF(ptNetxControlledPciConfigurationShadowRegisterBlockArea);
	unsigned int uDelay;
	unsigned long ulValue;

	ptNetxControlledPciConfigurationShadowRegisterBlockArea->ulDpmas_pci_conf_wr_data = uData;
	ptNetxControlledPciConfigurationShadowRegisterBlockArea->ulDpmas_pci_conf_wr_ctrl = (1<<20)|uWrCtrl;// write request

	uDelay = 8;
	do
	{
//		if( (PEEK(0x00103708)&(1<<21))!=0 ) {
//			break;
//		}
		ulValue  = ptNetxControlledPciConfigurationShadowRegisterBlockArea->ulDpmas_pci_conf_wr_ctrl;
		ulValue &= 1<<21;
		if( ulValue!=0 )
		{
			break;
		}
	} while( --uDelay!=0 );

	return (uDelay!=0);
}

//-------------------------------------

/** Read PCI configuration register over a DMA transfer.
 *
 * Do this using a configuration cycle.
 *
 * @param uDeviceAdr    Device address according to the PCI Device Scan
 * @param pulNetxAdr    Destination data pointer
 * @param uDwords       Data length
 *
 * @return iResult      0 if OK, !=0 on error
 */

int pciDma_CfgRead(unsigned int uDeviceAdr, volatile unsigned long *pulNetxAdr, unsigned int uDwords)
{
	unsigned int uDmaCtrl;
	int iResult;


	/* Initiate DMA Buffer (START) */
	uDmaCtrl = MSK_DPMAS_NETX_DMA_CTRL_START;
	/* Transfer Type: Configuration Cycle (DMA_TYPE) */
	uDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Configuration_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;

	iResult = pciDma_Ch0(uDeviceAdr, pulNetxAdr, uDmaCtrl|(uDwords<<SRT_DPMAS_NETX_DMA_CTRL_TRANSFER_LENGTH));

	/* Swap bits after reading. */
	swapBi0_Bit30_array(pulNetxAdr, uDwords);

	return iResult;
}

/** Write PCI configuration register over a DMA transfer.
 *
 * Do this using a configuration cycle in direction from netX to host.
 *
 * @param uDeviceAdr    Device address according to the PCI Device Scan
 * @param pulNetxAdr    Destination data pointer
 * @param uDwords       Data length
 *
 * @return iResult      0 if OK, !=0 on error
 */

int pciDma_CfgWrite(unsigned int uDeviceAdr, volatile unsigned long *pulNetxAdr, unsigned int uDwords)
{
	unsigned int uDmaCtrl;
	int iResult;


	/* Initiate DMA Buffer (START) */
	uDmaCtrl  = MSK_DPMAS_NETX_DMA_CTRL_START;
	/* Transfer Type: Configuration Cycle (DMA_TYPE) */
	uDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Configuration_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;
	/* Transfer direction 1: netX to Host (DIRECTION) */
	uDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DIRECTION_netx_to_host << SRT_DPMAS_NETX_DMA_CTRL_DIRECTION;

	/* Swap before writing. */
	swapBi0_Bit30_array(pulNetxAdr, uDwords);

	iResult = pciDma_Ch0(uDeviceAdr, pulNetxAdr, uDmaCtrl|(uDwords << SRT_DPMAS_NETX_DMA_CTRL_TRANSFER_LENGTH));
	return iResult;
}

/** Read PCI configuration register over a DMA transfer.
 *
 * Do this using a memory cycle.
 *
 * @param uDeviceAdr    Device address according to the PCI Device Scan
 * @param *pulNetxAdr   Destination data pointer
 * @param uDwords       Data length
 *
 * @return iResult      0 if OK, !=0 on error
 */

int pciDma_MemRead(unsigned int uDeviceAdr, volatile unsigned long *pulNetxAdr, unsigned int uDwords)
{
	unsigned int uDmaCtrl;
	int iResult;


	/* Initiate DMA Buffer (START) */
	uDmaCtrl  = MSK_DPMAS_NETX_DMA_CTRL_START;
	/* Transfer Type: Memory Cycle (DMA_TYPE) */
	uDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Memory_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;

	iResult = pciDma_Ch0(uDeviceAdr, pulNetxAdr, uDmaCtrl|(uDwords<<SRT_DPMAS_NETX_DMA_CTRL_TRANSFER_LENGTH));

	/* Swap bits after reading. */
	swapBi0_Bit30_array(pulNetxAdr, uDwords);

	return iResult;
}

/** Write PCI DMA memory.
 *
 * Do this using a memory cycle in direction from netX to host.
 *
 * @param uDeviceAdr    Device address according to the PCI Device Scan
 * @param *pulNetxAdr   Destination data pointer
 * @param uDwords       Data length
 *
 * @return iResult      0 if OK, !=0 on error
 */

int pciDma_MemWrite(unsigned int uDeviceAdr, volatile unsigned long *pulNetxAdr, unsigned int uDwords)
{
	unsigned int uDmaCtrl;
	int iResult;


	/* Initiate DMA Buffer (START) */
	uDmaCtrl = MSK_DPMAS_NETX_DMA_CTRL_START;
	/* Transfer Type: Memory Cycle (DMA_TYPE) */
	uDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Memory_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;
	/* Transfer direction 1: netX to Host (DIRECTION) */
	uDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DIRECTION_netx_to_host << SRT_DPMAS_NETX_DMA_CTRL_DIRECTION;

	/* Swap bits before writing. */
	swapBi0_Bit30_array(pulNetxAdr, uDwords);

	iResult = pciDma_Ch0(uDeviceAdr, pulNetxAdr, uDmaCtrl|(uDwords<<SRT_DPMAS_NETX_DMA_CTRL_TRANSFER_LENGTH));

	return iResult;
}

/** Initiate a Type 1 configuration read cycle.
 *
 * @param uDeviceAdr     Device address according to the PCI Device Scan
 * @param *pulNetxAdr    Destination data pointer
 * @param uDwords        Data length
 *
 * @return iResult       0 if OK, !=0 on error
 */
int pciDma_CfgRead_Type1(unsigned int uDeviceAdr, volatile unsigned long *pulNetxAdr, unsigned int uDwords)
{
	unsigned int uDmaCtrl;
	int iResult;


	/* Initiate DMA Buffer (START) */
	uDmaCtrl = MSK_DPMAS_NETX_DMA_CTRL_START;
	/* Transfer Type: Configuration Cycle (DMA_TYPE) */
	uDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Configuration_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;

	/* Set the header type to 1. */
	uDeviceAdr |= (1 << SRT_PCI_HEADER_TYPE);

	iResult = pciDma_Ch0(uDeviceAdr, pulNetxAdr, uDmaCtrl|(uDwords<<SRT_DPMAS_NETX_DMA_CTRL_TRANSFER_LENGTH));

	/* Swap data after reading. */
	swapBi0_Bit30_array(pulNetxAdr, uDwords);

	return iResult;
}

/** Initiate a Type 1 configuration write cycle.
 *
 * @param uDeviceAdr     Device address according to the PCI Device Scan
 * @param uDeviceOffs    Source data pointer
 * @param *pulNetxAdr    Destination data pointer
 * @param uDwords        Data length
 *
 * @return iResult       0 if OK, !=0 on error
 */
int pciDma_CfgWrite_Type1(unsigned int uDeviceAdr, volatile unsigned long *pulNetxAdr, unsigned int uDwords)
{
	unsigned int uDmaCtrl;
	int iResult;


	/* Initiate DMA Buffer (START) */
	uDmaCtrl  = MSK_DPMAS_NETX_DMA_CTRL_START;
	/* Transfer Type: Configuration Cycle (DMA_TYPE) */
	uDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Configuration_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;
	/* Transfer direction 1: netX to Host (DIRECTION) */
	uDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DIRECTION_netx_to_host << SRT_DPMAS_NETX_DMA_CTRL_DIRECTION;

	/* Set the header type to 1. */
	uDeviceAdr |= (1 << SRT_PCI_HEADER_TYPE);

	/* Swap data before writing. */
	swapBi0_Bit30_array(pulNetxAdr, uDwords);

	iResult = pciDma_Ch0(uDeviceAdr, pulNetxAdr, uDmaCtrl|(uDwords << SRT_DPMAS_NETX_DMA_CTRL_TRANSFER_LENGTH));
	return iResult;
}


//-------------------------------------


int dmalist_process(const DMALIST_T *ptDmaList, unsigned int sizDmaList, const char *pcName)
{
	int iResult;
	const DMALIST_T *ptCnt;
	unsigned int uiCnt;
	DMAACTION_T tAct;
	unsigned long ulDmaCtrl;
	unsigned long ulAddress;
	unsigned long ulDataReceived;
	unsigned long ulDataExpected;
	unsigned long ulParity;


	/* Be optimistic.
	 * An empty list is OK.
	 */
	iResult = 0;

	/* Loop over all entries in the DMA list. */
	uiCnt = 0;
	while( uiCnt<sizDmaList )
	{
		/* Get a pointer to the DMA list entry. */
		ptCnt = ptDmaList + uiCnt;

		ulDmaCtrl  = MSK_DPMAS_NETX_DMA_CTRL_START;
		/* The number of DWORDS is always 1 in a DMA list. */
		ulDmaCtrl |= 1U << SRT_DPMAS_NETX_DMA_CTRL_TRANSFER_LENGTH;

		ulAddress  = ptCnt->ulAddress;

		iResult = -1;
		tAct = ptCnt->tAct;
		switch( tAct )
		{
		case DMAACTION_Write:
			/* Transfer direction: netX to host */
			ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DIRECTION_netx_to_host << SRT_DPMAS_NETX_DMA_CTRL_DIRECTION;

			/* Swap bit 0 and 30 of the data and write it to the DMA buffer. */
			g_ul_PCI_DMA_Buffer_Start[0] = swapBit0_Bit30(ptCnt->ulData);

			iResult = 0;
			break;

		case DMAACTION_Compare:
			/* Transfer direction: host to netX */
			ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DIRECTION_host_to_netx << SRT_DPMAS_NETX_DMA_CTRL_DIRECTION;

			iResult = 0;
			break;

		case DMAACTION_Dump:
			/* Transfer direction: host to netX */
			ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DIRECTION_host_to_netx << SRT_DPMAS_NETX_DMA_CTRL_DIRECTION;

			iResult = 0;
			break;
		}
		if( iResult!=0 )
		{
			uprintf("[%s:%d] Invalid action: %d\n", pcName, uiCnt, tAct);
		}
		else
		{
			iResult = -1;
			switch( ptCnt->tTyp )
			{
			case DMATYPE_IO:
				/* Transfer type: I/O cycle */
				ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Input_Output_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;

				iResult = 0;
				break;

			case DMATYPE_Mem:
				/* Transfer type: memory cycle */
				ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Memory_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;

				iResult = 0;
				break;

			case DMATYPE_Cfg0:
				/* Transfer type: configuration cycle */
				ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Configuration_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;

				iResult = 0;
				break;

			case DMATYPE_Cfg1:
				/* Transfer type: configuration cycle */
				ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Configuration_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;

				/* Set type 1 in the header. */
				ulAddress |= 1U << SRT_PCI_HEADER_TYPE;

				iResult = 0;
				break;
			}
			if( iResult!=0 )
			{
				uprintf("[%s:%d] Invalid type: %d\n", pcName, uiCnt, ptCnt->tTyp);
			}
			else
			{
				if( tAct==DMAACTION_Write )
				{
					/* Do a simple parity check. */
					ulParity = ulAddress ^ ptCnt->ulData;
					ulParity = ulParity ^ (ulParity >> 16U);
					ulParity = ulParity ^ (ulParity >>  8U);
					ulParity = ulParity ^ (ulParity >>  4U);
					ulParity = ulParity ^ (ulParity >>  2U);
					ulParity = ulParity ^ (ulParity >>  1U);
					ulParity &= 1U;
					if( ulParity==0 )
					{
						uprintf("!!!PARITY ERROR!!!\n");
					}
				}

				iResult = pciDma_Ch0(ulAddress, g_ul_PCI_DMA_Buffer_Start, ulDmaCtrl);
				if( iResult!=0 )
				{
					uprintf("[%s:%d] The transfer failed: %d\n", pcName, uiCnt, iResult);
				}
				else
				{
					switch( tAct )
					{
					case DMAACTION_Write:
						/* Nothing to do after a write. */
						break;

					case DMAACTION_Compare:
						/* Compare the received value with the ulData field. */
						ulDataReceived = swapBit0_Bit30(g_ul_PCI_DMA_Buffer_Start[0]);
						ulDataExpected = ptCnt->ulData;
						if( ulDataReceived!=ulDataExpected )
						{
							uprintf("[%s:%d] The received data does not match the expected data: 0x%08x / 0x%08x\n", pcName, uiCnt, ulDataReceived, ulDataExpected);
							iResult = -1;
						}
						break;

					case DMAACTION_Dump:
						/* Just print the received data. */
						ulDataReceived = swapBit0_Bit30(g_ul_PCI_DMA_Buffer_Start[0]);
						uprintf("[%s:%d] Received 0x%08x\n", pcName, uiCnt, ulDataReceived);
						break;
					}
				}
			}
		}

		/* Do not process the rest of the list if the current entry failed. */
		if( iResult!=0 )
		{
			break;
		}

		++uiCnt;
	}

	return iResult;
}



int io_register_read_08(unsigned int ulAddress, unsigned char *pucData)
{
	int iResult;
	unsigned long ulDmaCtrl;
	unsigned long ulByteOffset;
	unsigned long ulDataReceived;


	/* Split the address into a DWORD part with a byte offset. */
	ulByteOffset = ulAddress & 3U;
	ulAddress &= 0xfffffffcU;

	ulDmaCtrl  = MSK_DPMAS_NETX_DMA_CTRL_START;
	/* The number of DWORDS is 1. */
	ulDmaCtrl |= 1U << SRT_DPMAS_NETX_DMA_CTRL_TRANSFER_LENGTH;
	ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DIRECTION_host_to_netx << SRT_DPMAS_NETX_DMA_CTRL_DIRECTION;
	ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Input_Output_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;

	iResult = pciDma_Ch0(ulAddress, g_ul_PCI_DMA_Buffer_Start, ulDmaCtrl);
	if( iResult!=0 )
	{
		/* The transfer failed. */
		iResult = -1;
	}
	else
	{
		if( pucData!=NULL )
		{
			/* Get the complete DWORD. */
			ulDataReceived = swapBit0_Bit30(g_ul_PCI_DMA_Buffer_Start[0]);
			/* Extract the data. */
			ulDataReceived >>= 8U * ulByteOffset;
			*pucData = (unsigned char)(ulDataReceived & 0x000000ffU);
		}
		iResult = 0;
	}

	return iResult;
}



int io_register_read_32(unsigned int ulAddress, unsigned long *pulData)
{
	int iResult;
	unsigned long ulDmaCtrl;
	unsigned long ulDataReceived;


	ulDmaCtrl  = MSK_DPMAS_NETX_DMA_CTRL_START;
	/* The number of DWORDS is 1. */
	ulDmaCtrl |= 1U << SRT_DPMAS_NETX_DMA_CTRL_TRANSFER_LENGTH;
	ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DIRECTION_host_to_netx << SRT_DPMAS_NETX_DMA_CTRL_DIRECTION;
	ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Input_Output_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;

	iResult = pciDma_Ch0(ulAddress, g_ul_PCI_DMA_Buffer_Start, ulDmaCtrl);
	if( iResult!=0 )
	{
		/* The transfer failed. */
		iResult = -1;
	}
	else
	{
		if( pulData!=NULL )
		{
			/* Get the complete DWORD. */
			ulDataReceived = swapBit0_Bit30(g_ul_PCI_DMA_Buffer_Start[0]);
			*pulData = ulDataReceived;
		}
		iResult = 0;
	}

	return iResult;
}



int io_register_write_32(unsigned int ulAddress, unsigned long ulData)
{
	int iResult;
	unsigned long ulDmaCtrl;


	ulDmaCtrl  = MSK_DPMAS_NETX_DMA_CTRL_START;
	/* The number of DWORDS is 1. */
	ulDmaCtrl |= 1U << SRT_DPMAS_NETX_DMA_CTRL_TRANSFER_LENGTH;
	ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DIRECTION_netx_to_host << SRT_DPMAS_NETX_DMA_CTRL_DIRECTION;
	ulDmaCtrl |= VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Input_Output_Cycle << SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE;

	/* Swap bit 0 and 30 of the data and write it to the DMA buffer. */
	g_ul_PCI_DMA_Buffer_Start[0] = swapBit0_Bit30(ulData);

	iResult = pciDma_Ch0(ulAddress, g_ul_PCI_DMA_Buffer_Start, ulDmaCtrl);

	return iResult;
}
