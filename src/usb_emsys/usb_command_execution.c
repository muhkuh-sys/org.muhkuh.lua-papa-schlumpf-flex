/*
 * command_execution.cpp
 *
 *  Created on: 18.05.2016
 *      Author: DennisKaiser
 */

#include <string.h>

#include "usb_command_execution.h"
#include "pci.h"
#include "uprintf.h"
#include "version.h"


extern volatile unsigned long *g_pul_PCI_DMA_Buffer_Start;
extern volatile unsigned long *g_pul_PCI_DMA_Buffer_End;


static void execute_command_get_firmware_version(void)
{
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_GETFIRMWAREVERSION_T tPacket;


	tPacket.ulStatus = USB_COMMAND_STATUS_Ok;
	tPacket.ulMajor = VERSION_MAJOR;
	tPacket.ulMinor = VERSION_MINOR;
	tPacket.ulSub = VERSION_MICRO;
	memset(tPacket.acVcsVersion, 0, sizeof(tPacket.acVcsVersion));
	strncpy(tPacket.acVcsVersion, VERSION_VCS, sizeof(tPacket.acVcsVersion));
	usb_send_packet((unsigned char*)(&tPacket), sizeof(tPacket));
}



static void execute_command_reset_pci(PAPA_SCHLUMPF_USB_COMMAND_RESET_PCI_T *ptCommand)
{
	int iResult;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_STATUS_T tPacket;


	iResult = pciResetAndInit(ptCommand->ulResetActiveToClock, ptCommand->ulResetActiveDelayAfterClock, ptCommand->ulBusIdleDelay);
	if( iResult==0 )
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_Ok;
	}
	else
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_PciInitFailed;
	}

	usb_send_packet((unsigned char*)(&tPacket), sizeof(tPacket));
}



static void execute_command_dma_io_read(PAPA_SCHLUMPF_USB_COMMAND_DMA_IO_READ_T *ptCommand)
{
	int iResult;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_DMA_IO_READ_T tPacket;


	iResult = io_register_read_32(ptCommand->ulDeviceAddress, &(tPacket.ulData));
	if( iResult==0 )
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_Ok;
	}
	else
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_PciTransferFailed;
	}
	usb_send_packet((unsigned char*)(&tPacket), sizeof(tPacket));
}



static void execute_command_dma_mem_read(PAPA_SCHLUMPF_USB_COMMAND_DMA_MEM_READ_T *ptCommand)
{
	int iResult;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_DMA_MEM_READ_T tPacket;


	iResult = pciDma_MemRead(ptCommand->ulDeviceAddress, g_pul_PCI_DMA_Buffer_Start, 1);
	if( iResult==0 )
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_Ok;
		tPacket.ulData = *g_pul_PCI_DMA_Buffer_Start;
	}
	else
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_PciTransferFailed;
	}
	usb_send_packet((unsigned char*)(&tPacket), sizeof(tPacket));
}



static PAPA_SCHLUMPF_USB_COMMAND_RESULT_DMA_MEM_READ_AREA_T tReadAreaResponse;
static void execute_command_dma_mem_read_area(PAPA_SCHLUMPF_USB_COMMAND_DMA_MEM_READ_AREA_T *ptCommand)
{
	int iResult;
	unsigned long ulSize;
	unsigned int uiSizeDw;
	size_t sizUsbPacket;

	/* Round the size of the read request up to the next DWORD and convert the byte size to a DWORD size. */
	ulSize = ptCommand->ulSize;
	if( (ulSize&3U)!=0 )
	{
		tReadAreaResponse.ulStatus = USB_COMMAND_STATUS_InvalidSize;
		sizUsbPacket = sizeof(uint32_t);
	}
	else
	{
		uiSizeDw = ulSize / sizeof(uint32_t);

		/* Does the requested size exceed the PCI DMA buffer? */
		if( ulSize>(unsigned int)(g_pul_PCI_DMA_Buffer_End - g_pul_PCI_DMA_Buffer_Start) )
		{
			tReadAreaResponse.ulStatus = USB_COMMAND_STATUS_InvalidSize;
			sizUsbPacket = sizeof(uint32_t);
		}
		/* Does the request exceed the USB buffer? */
		else if( ulSize>sizeof(tReadAreaResponse.aucData) )
		{
			tReadAreaResponse.ulStatus = USB_COMMAND_STATUS_InvalidSize;
			sizUsbPacket = sizeof(uint32_t);
		}
		else
		{
			iResult = pciDma_MemRead(ptCommand->ulDeviceAddress, g_pul_PCI_DMA_Buffer_Start, uiSizeDw);
			if( iResult==0 )
			{
				tReadAreaResponse.ulStatus = USB_COMMAND_STATUS_Ok;
				memcpy(tReadAreaResponse.aucData, g_pul_PCI_DMA_Buffer_Start, ulSize);
				sizUsbPacket = sizeof(uint32_t) + ulSize;
			}
			else
			{
				tReadAreaResponse.ulStatus = USB_COMMAND_STATUS_PciTransferFailed;
				sizUsbPacket = sizeof(uint32_t);
			}
		}
		usb_send_packet((unsigned char*)(&tReadAreaResponse), sizUsbPacket);
	}
}



static void execute_command_dma_cfg0_read(PAPA_SCHLUMPF_USB_COMMAND_DMA_CFG0_READ_T *ptCommand)
{
	int iResult;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_DMA_CFG0_READ_T tPacket;


	iResult = pciDma_CfgRead(ptCommand->ulDeviceAddress, g_pul_PCI_DMA_Buffer_Start, 1);
	if( iResult==0 )
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_Ok;
		tPacket.ulData = *g_pul_PCI_DMA_Buffer_Start;
	}
	else
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_PciTransferFailed;
	}
	usb_send_packet((unsigned char*)(&tPacket), sizeof(tPacket));
}



static void execute_command_dma_cfg1_read(PAPA_SCHLUMPF_USB_COMMAND_DMA_CFG1_READ_T *ptCommand)
{
	int iResult;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_DMA_CFG1_READ_T tPacket;


	iResult = pciDma_CfgRead_Type1(ptCommand->ulDeviceAddress, g_pul_PCI_DMA_Buffer_Start, 1);
	if( iResult==0 )
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_Ok;
		tPacket.ulData = *g_pul_PCI_DMA_Buffer_Start;
	}
	else
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_PciTransferFailed;
	}
	usb_send_packet((unsigned char*)(&tPacket), sizeof(tPacket));
}



static void execute_command_dma_io_write(PAPA_SCHLUMPF_USB_COMMAND_DMA_IO_WRITE_T *ptCommand)
{
	int iResult;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_STATUS_T tPacket;


	iResult = io_register_write_32(ptCommand->ulDeviceAddress, ptCommand->ulData);
	if( iResult==0 )
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_Ok;
	}
	else
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_PciTransferFailed;
	}
	usb_send_packet((unsigned char*)(&tPacket), sizeof(tPacket));
}



static void execute_command_dma_mem_write(PAPA_SCHLUMPF_USB_COMMAND_DMA_MEM_WRITE_T *ptCommand)
{
	int iResult;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_STATUS_T tPacket;


	*g_pul_PCI_DMA_Buffer_Start = ptCommand->ulData;
	iResult = pciDma_MemWrite(ptCommand->ulDeviceAddress, g_pul_PCI_DMA_Buffer_Start, 1);
	if( iResult==0 )
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_Ok;
	}
	else
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_PciTransferFailed;
	}
	usb_send_packet((unsigned char*)(&tPacket), sizeof(tPacket));
}



static void execute_command_dma_mem_write_area(PAPA_SCHLUMPF_USB_COMMAND_DMA_MEM_WRITE_AREA_T *ptCommand)
{
	int iResult;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_STATUS_T tPacket;
	unsigned long ulSize;
	unsigned int uiSizeDw;


	ulSize = ptCommand->ulSize;
	uiSizeDw = ulSize / sizeof(uint32_t);

	/* Is the size a multiple of 4? */
	if( (ulSize&3U)!=0 )
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_InvalidSize;
	}
	/* Does the requested size exceed the PCI DMA buffer? */
	else if( ulSize>(unsigned int)(g_pul_PCI_DMA_Buffer_End - g_pul_PCI_DMA_Buffer_Start) )
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_InvalidSize;
	}
	else
	{
		/* Copy the data from the packet to the DMA buffer. */
		memcpy(g_pul_PCI_DMA_Buffer_Start, ptCommand->aucData, ulSize);

		iResult = pciDma_MemWrite(ptCommand->ulDeviceAddress, g_pul_PCI_DMA_Buffer_Start, uiSizeDw);
		if( iResult==0 )
		{
			tPacket.ulStatus = USB_COMMAND_STATUS_Ok;
		}
		else
		{
			tPacket.ulStatus = USB_COMMAND_STATUS_PciTransferFailed;
		}
	}
	usb_send_packet((unsigned char*)(&tPacket), sizeof(tPacket));
}



static void execute_command_dma_cfg0_write(PAPA_SCHLUMPF_USB_COMMAND_DMA_CFG0_WRITE_T *ptCommand)
{
	int iResult;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_STATUS_T tPacket;


	*g_pul_PCI_DMA_Buffer_Start = ptCommand->ulData;
	iResult = pciDma_CfgWrite(ptCommand->ulDeviceAddress, g_pul_PCI_DMA_Buffer_Start, 1);
	if( iResult==0 )
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_Ok;
	}
	else
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_PciTransferFailed;
	}
	usb_send_packet((unsigned char*)(&tPacket), sizeof(tPacket));
}



static void execute_command_dma_cfg1_write(PAPA_SCHLUMPF_USB_COMMAND_DMA_CFG1_WRITE_T *ptCommand)
{
	int iResult;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_STATUS_T tPacket;


	*g_pul_PCI_DMA_Buffer_Start = ptCommand->ulData;
	iResult = pciDma_CfgWrite_Type1(ptCommand->ulDeviceAddress, g_pul_PCI_DMA_Buffer_Start, 1);
	if( iResult==0 )
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_Ok;
	}
	else
	{
		tPacket.ulStatus = USB_COMMAND_STATUS_PciTransferFailed;
	}
	usb_send_packet((unsigned char*)(&tPacket), sizeof(tPacket));
}



void execute_command(PAPA_SCHLUMPF_USB_COMMAND_T *ptCommand)
{
	PAPA_SCHLUMPF_USB_COMMANDS_T tCommand;
	int iResult;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_STATUS_T tPacketStatus;


	tCommand = ptCommand->ulCommand;
	iResult = -1;
	switch(tCommand)
	{
	case PAPA_SCHLUMPF_USB_COMMAND_GetFirmwareVersion:
	case PAPA_SCHLUMPF_USB_COMMAND_ResetPCI:
	case PAPA_SCHLUMPF_USB_COMMAND_DMAIoRead:
	case PAPA_SCHLUMPF_USB_COMMAND_DMAMemRead:
	case PAPA_SCHLUMPF_USB_COMMAND_DMACfg0Read:
	case PAPA_SCHLUMPF_USB_COMMAND_DMACfg1Read:
	case PAPA_SCHLUMPF_USB_COMMAND_DMAIoWrite:
	case PAPA_SCHLUMPF_USB_COMMAND_DMAMemWrite:
	case PAPA_SCHLUMPF_USB_COMMAND_DMACfg0Write:
	case PAPA_SCHLUMPF_USB_COMMAND_DMACfg1Write:
	case PAPA_SCHLUMPF_USB_COMMAND_DMAMemReadArea:
	case PAPA_SCHLUMPF_USB_COMMAND_DMAMemWriteArea:
		iResult = 0;
		break;
	}
	if( iResult!=0 )
	{
		/*Unknown command*/
		tPacketStatus.ulStatus = USB_COMMAND_STATUS_UnknownCommand;
		usb_send_packet((unsigned char*)(&tPacketStatus), sizeof(tPacketStatus));
	}
	else
	{
		switch(tCommand)
		{
		case PAPA_SCHLUMPF_USB_COMMAND_GetFirmwareVersion:
			execute_command_get_firmware_version();
			break;

		case PAPA_SCHLUMPF_USB_COMMAND_ResetPCI:
			execute_command_reset_pci((PAPA_SCHLUMPF_USB_COMMAND_RESET_PCI_T*)ptCommand);
			break;

		case PAPA_SCHLUMPF_USB_COMMAND_DMAIoRead:
			execute_command_dma_io_read((PAPA_SCHLUMPF_USB_COMMAND_DMA_IO_READ_T*)ptCommand);
			break;

		case PAPA_SCHLUMPF_USB_COMMAND_DMAMemRead:
			execute_command_dma_mem_read((PAPA_SCHLUMPF_USB_COMMAND_DMA_MEM_READ_T*)ptCommand);
			break;

		case PAPA_SCHLUMPF_USB_COMMAND_DMAMemReadArea:
			execute_command_dma_mem_read_area((PAPA_SCHLUMPF_USB_COMMAND_DMA_MEM_READ_AREA_T*)ptCommand);
			break;

		case PAPA_SCHLUMPF_USB_COMMAND_DMACfg0Read:
			execute_command_dma_cfg0_read((PAPA_SCHLUMPF_USB_COMMAND_DMA_CFG0_READ_T*)ptCommand);
			break;

		case PAPA_SCHLUMPF_USB_COMMAND_DMACfg1Read:
			execute_command_dma_cfg1_read((PAPA_SCHLUMPF_USB_COMMAND_DMA_CFG1_READ_T*)ptCommand);
			break;

		case PAPA_SCHLUMPF_USB_COMMAND_DMAIoWrite:
			execute_command_dma_io_write((PAPA_SCHLUMPF_USB_COMMAND_DMA_IO_WRITE_T*)ptCommand);
			break;

		case PAPA_SCHLUMPF_USB_COMMAND_DMAMemWrite:
			execute_command_dma_mem_write((PAPA_SCHLUMPF_USB_COMMAND_DMA_MEM_WRITE_T*)ptCommand);
			break;

		case PAPA_SCHLUMPF_USB_COMMAND_DMAMemWriteArea:
			execute_command_dma_mem_write_area((PAPA_SCHLUMPF_USB_COMMAND_DMA_MEM_WRITE_AREA_T*)ptCommand);
			break;

		case PAPA_SCHLUMPF_USB_COMMAND_DMACfg0Write:
			execute_command_dma_cfg0_write((PAPA_SCHLUMPF_USB_COMMAND_DMA_CFG0_WRITE_T*)ptCommand);
			break;

		case PAPA_SCHLUMPF_USB_COMMAND_DMACfg1Write:
			execute_command_dma_cfg1_write((PAPA_SCHLUMPF_USB_COMMAND_DMA_CFG1_WRITE_T*)ptCommand);
			break;
		}
	}
}

