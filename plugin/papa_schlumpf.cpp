#include "papa_schlumpf.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "papa_schlumpf_firmware_interface.h"


PapaSchlumpfFlex::PapaSchlumpfFlex(void)
 : m_ptLibUsbContext(NULL)
 , m_ptDevHandlePapaSchlumpf(NULL)
 , m_pcPluginId(NULL)
{
	const struct libusb_version *ptLibUsbVersion;


	m_pcPluginId = strdup("papa_schlumpf");

	/* Show the libusb version. */
	ptLibUsbVersion = libusb_get_version();
	printf("%s: Using libusb %d.%d.%d.%d%s\n", m_pcPluginId, ptLibUsbVersion->major, ptLibUsbVersion->minor, ptLibUsbVersion->micro, ptLibUsbVersion->nano, ptLibUsbVersion->rc);
}



PapaSchlumpfFlex::~PapaSchlumpfFlex(void)
{
	__disconnect();

	if( m_pcPluginId!=NULL )
	{
		free(m_pcPluginId);
		m_pcPluginId = NULL;
	}
}


RESULT_INT_TRUE_OR_NIL_WITH_ERR PapaSchlumpfFlex::connect(void)
{
	int iResult;
	PAPA_SCHLUMPF_RESULT_T tResult;


	/* Close any existing connection first. */
	__disconnect();

	/* Initialize the libusb context. */
	iResult = libusb_init(&m_ptLibUsbContext);
	if( iResult!=0 )
	{
		printf("Error initializing libusb: %d\n", iResult);
		tResult = PAPA_SCHLUMPF_RESULT_USBError;
	}
	else
	{
		/* Set the debug level to a bit more verbose. */
		libusb_set_option(m_ptLibUsbContext, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);

		printf("libusb is open!\n");

		/* Search for exactly one papa schlumpf hardware and open it. */
		tResult = __scan_for_papa_schlumpf_hardware();
		if( tResult==PAPA_SCHLUMPF_RESULT_Ok )
		{
			printf("Found HW!\n");
		}
	}

	return tResult;
}



RESULT_INT_NOTHING_OR_NIL_WITH_ERR PapaSchlumpfFlex::getFirmwareVersion(PUL_ARGUMENT_OUT pulVersionMajor, PUL_ARGUMENT_OUT pulVersionMinor, PUL_ARGUMENT_OUT pulVersionSub, PPC_ARGUMENT_OUT ppcVcsVersion)
{
	PAPA_SCHLUMPF_RESULT_T tResult;
	int iResult;
	int iTransfered;
	PAPA_SCHLUMPF_USB_COMMAND_T tCommand;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_GETFIRMWAREVERSION_T tResponse;
	char *pcVcsVersion;


	if( m_ptDevHandlePapaSchlumpf==NULL )
	{
		tResult = PAPA_SCHLUMPF_RESULT_NotConnected;
	}
	else
	{
		tCommand.ulCommand = PAPA_SCHLUMPF_USB_COMMAND_GetFirmwareVersion;
		iResult = __send_packet((const unsigned char *)&tCommand, sizeof(tCommand), 100);
		if( iResult!=0 )
		{
			fprintf(stderr, "%s: failed to send packet: %d\n", m_pcPluginId, iResult);
			tResult = PAPA_SCHLUMPF_RESULT_USBError;
		}
		else
		{
			iResult = __receivePacket((unsigned char *)&tResponse, sizeof(tResponse), &iTransfered, 100);
			if( iResult!=0 )
			{
				fprintf(stderr, "%s: failed to receive packet: %d\n", m_pcPluginId, iResult);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( iTransfered!=sizeof(tResponse) )
			{
				fprintf(stderr, "%s: received an unexpected amount of data. wanted %zd bytes, but got %d.\n", m_pcPluginId, sizeof(tResponse), iTransfered);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( tResponse.ulStatus!=USB_COMMAND_STATUS_Ok )
			{
				fprintf(stderr, "%s: received an error: %d.\n", m_pcPluginId, tResponse.ulStatus);
				tResult = PAPA_SCHLUMPF_RESULT_CommandFailed;
			}
			else
			{
				*pulVersionMajor = tResponse.ulMajor;
				*pulVersionMinor = tResponse.ulMinor;
				*pulVersionSub = tResponse.ulSub;
				pcVcsVersion = (char*)calloc(1, sizeof(tResponse.acVcsVersion)+1);
				if( pcVcsVersion!=NULL )
				{
					strncpy(pcVcsVersion, tResponse.acVcsVersion, sizeof(tResponse.acVcsVersion));
				}
				*ppcVcsVersion = pcVcsVersion;
				tResult = PAPA_SCHLUMPF_RESULT_Ok;
			}
		}
	}

	return tResult;
}



RESULT_INT_TRUE_OR_NIL_WITH_ERR PapaSchlumpfFlex::resetPCI(uint32_t ulResetActiveToClock, uint32_t ulResetActiveDelayAfterClock, uint32_t ulBusIdleDelay)
{
	PAPA_SCHLUMPF_RESULT_T tResult;
	int iResult;
	int iTransfered;
	unsigned long ulExpectedDelay;
	PAPA_SCHLUMPF_USB_COMMAND_RESET_PCI_T tCommand;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_STATUS_T tResponse;


	if( m_ptDevHandlePapaSchlumpf==NULL )
	{
		tResult = PAPA_SCHLUMPF_RESULT_NotConnected;
	}
	else
	{
		tCommand.ulCommand = PAPA_SCHLUMPF_USB_COMMAND_ResetPCI;
		tCommand.ulResetActiveToClock = ulResetActiveToClock;
		tCommand.ulResetActiveDelayAfterClock = ulResetActiveDelayAfterClock;
		tCommand.ulBusIdleDelay = ulBusIdleDelay;
		iResult = __send_packet((const unsigned char *)&tCommand, sizeof(tCommand), 100);
		if( iResult!=0 )
		{
			fprintf(stderr, "%s: failed to send packet: %d\n", m_pcPluginId, iResult);
			tResult = PAPA_SCHLUMPF_RESULT_USBError;
		}
		else
		{
			/* Sum up all delays to get the expected command execution time.
			 * All values are in units of 100us, convert them to ms with *10 .
			 * Add 100ms for the command execution.
			 */
			ulExpectedDelay  = (ulResetActiveToClock + ulResetActiveDelayAfterClock + ulBusIdleDelay) * 10U;
			ulExpectedDelay += 100U;

			iResult = __receivePacket((unsigned char *)&tResponse, sizeof(tResponse), &iTransfered, ulExpectedDelay);
			if( iResult!=0 )
			{
				fprintf(stderr, "%s: failed to receive packet: %d\n", m_pcPluginId, iResult);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( iTransfered!=sizeof(tResponse) )
			{
				fprintf(stderr, "%s: received an unexpected amount of data. wanted %zd bytes, but got %d.\n", m_pcPluginId, sizeof(tResponse), iTransfered);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( tResponse.ulStatus!=USB_COMMAND_STATUS_Ok )
			{
				fprintf(stderr, "%s: received an error: %d.\n", m_pcPluginId, tResponse.ulStatus);
				tResult = PAPA_SCHLUMPF_RESULT_CommandFailed;
			}
			else
			{
				tResult = PAPA_SCHLUMPF_RESULT_Ok;
			}
		}
	}

	return tResult;
}



RESULT_INT_NOTHING_OR_NIL_WITH_ERR PapaSchlumpfFlex::ioRead(uint32_t ulAddress, PUL_ARGUMENT_OUT pulData)
{
	PAPA_SCHLUMPF_RESULT_T tResult;
	int iResult;
	int iTransfered;
	PAPA_SCHLUMPF_USB_COMMAND_DMA_IO_READ_T tCommand;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_DMA_IO_READ_T tResponse;


	if( m_ptDevHandlePapaSchlumpf==NULL )
	{
		tResult = PAPA_SCHLUMPF_RESULT_NotConnected;
	}
	else
	{
		tCommand.ulCommand = PAPA_SCHLUMPF_USB_COMMAND_DMAIoRead;
		tCommand.ulDeviceAddress = ulAddress;
		iResult = __send_packet((const unsigned char *)&tCommand, sizeof(tCommand), 100);
		if( iResult!=0 )
		{
			fprintf(stderr, "%s: failed to send packet: %d\n", m_pcPluginId, iResult);
			tResult = PAPA_SCHLUMPF_RESULT_USBError;
		}
		else
		{
			iResult = __receivePacket((unsigned char *)&tResponse, sizeof(tResponse), &iTransfered, 500);
			if( iResult!=0 )
			{
				fprintf(stderr, "%s: failed to receive packet: %d\n", m_pcPluginId, iResult);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( iTransfered!=sizeof(tResponse) )
			{
				fprintf(stderr, "%s: received an unexpected amount of data. wanted %zd bytes, but got %d.\n", m_pcPluginId, sizeof(tResponse), iTransfered);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( tResponse.ulStatus!=USB_COMMAND_STATUS_Ok )
			{
				fprintf(stderr, "%s: received an error: %d.\n", m_pcPluginId, tResponse.ulStatus);
				tResult = PAPA_SCHLUMPF_RESULT_CommandFailed;
			}
			else
			{
				*pulData = tResponse.ulData;
				tResult = PAPA_SCHLUMPF_RESULT_Ok;
			}
		}
	}

	return tResult;
}



RESULT_INT_NOTHING_OR_NIL_WITH_ERR PapaSchlumpfFlex::memRead(uint32_t ulAddress, PUL_ARGUMENT_OUT pulData)
{
	PAPA_SCHLUMPF_RESULT_T tResult;
	int iResult;
	int iTransfered;
	PAPA_SCHLUMPF_USB_COMMAND_DMA_MEM_READ_T tCommand;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_DMA_MEM_READ_T tResponse;


	if( m_ptDevHandlePapaSchlumpf==NULL )
	{
		tResult = PAPA_SCHLUMPF_RESULT_NotConnected;
	}
	else
	{
		tCommand.ulCommand = PAPA_SCHLUMPF_USB_COMMAND_DMAMemRead;
		tCommand.ulDeviceAddress = ulAddress;
		iResult = __send_packet((const unsigned char *)&tCommand, sizeof(tCommand), 100);
		if( iResult!=0 )
		{
			fprintf(stderr, "%s: failed to send packet: %d\n", m_pcPluginId, iResult);
			tResult = PAPA_SCHLUMPF_RESULT_USBError;
		}
		else
		{
			iResult = __receivePacket((unsigned char *)&tResponse, sizeof(tResponse), &iTransfered, 500);
			if( iResult!=0 )
			{
				fprintf(stderr, "%s: failed to receive packet: %d\n", m_pcPluginId, iResult);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( iTransfered!=sizeof(tResponse) )
			{
				fprintf(stderr, "%s: received an unexpected amount of data. wanted %zd bytes, but got %d.\n", m_pcPluginId, sizeof(tResponse), iTransfered);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( tResponse.ulStatus!=USB_COMMAND_STATUS_Ok )
			{
				fprintf(stderr, "%s: received an error: %d.\n", m_pcPluginId, tResponse.ulStatus);
				tResult = PAPA_SCHLUMPF_RESULT_CommandFailed;
			}
			else
			{
				*pulData = tResponse.ulData;
				tResult = PAPA_SCHLUMPF_RESULT_Ok;
			}
		}
	}

	return tResult;
}



RESULT_INT_NOTHING_OR_NIL_WITH_ERR PapaSchlumpfFlex::cfg0Read(uint32_t ulAddress, PUL_ARGUMENT_OUT pulData)
{
	PAPA_SCHLUMPF_RESULT_T tResult;
	int iResult;
	int iTransfered;
	PAPA_SCHLUMPF_USB_COMMAND_DMA_CFG0_READ_T tCommand;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_DMA_CFG0_READ_T tResponse;


	if( m_ptDevHandlePapaSchlumpf==NULL )
	{
		tResult = PAPA_SCHLUMPF_RESULT_NotConnected;
	}
	else
	{
		tCommand.ulCommand = PAPA_SCHLUMPF_USB_COMMAND_DMACfg0Read;
		tCommand.ulDeviceAddress = ulAddress;
		iResult = __send_packet((const unsigned char *)&tCommand, sizeof(tCommand), 100);
		if( iResult!=0 )
		{
			fprintf(stderr, "%s: failed to send packet: %d\n", m_pcPluginId, iResult);
			tResult = PAPA_SCHLUMPF_RESULT_USBError;
		}
		else
		{
			iResult = __receivePacket((unsigned char *)&tResponse, sizeof(tResponse), &iTransfered, 500);
			if( iResult!=0 )
			{
				fprintf(stderr, "%s: failed to receive packet: %d\n", m_pcPluginId, iResult);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( iTransfered!=sizeof(tResponse) )
			{
				fprintf(stderr, "%s: received an unexpected amount of data. wanted %zd bytes, but got %d.\n", m_pcPluginId, sizeof(tResponse), iTransfered);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( tResponse.ulStatus!=USB_COMMAND_STATUS_Ok )
			{
				fprintf(stderr, "%s: received an error: %d.\n", m_pcPluginId, tResponse.ulStatus);
				tResult = PAPA_SCHLUMPF_RESULT_CommandFailed;
			}
			else
			{
				*pulData = tResponse.ulData;
				tResult = PAPA_SCHLUMPF_RESULT_Ok;
			}
		}
	}

	return tResult;
}



RESULT_INT_NOTHING_OR_NIL_WITH_ERR PapaSchlumpfFlex::cfg1Read(uint32_t ulAddress, PUL_ARGUMENT_OUT pulData)
{
	PAPA_SCHLUMPF_RESULT_T tResult;
	int iResult;
	int iTransfered;
	PAPA_SCHLUMPF_USB_COMMAND_DMA_CFG1_READ_T tCommand;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_DMA_CFG1_READ_T tResponse;


	if( m_ptDevHandlePapaSchlumpf==NULL )
	{
		tResult = PAPA_SCHLUMPF_RESULT_NotConnected;
	}
	else
	{
		tCommand.ulCommand = PAPA_SCHLUMPF_USB_COMMAND_DMACfg1Read;
		tCommand.ulDeviceAddress = ulAddress;
		iResult = __send_packet((const unsigned char *)&tCommand, sizeof(tCommand), 100);
		if( iResult!=0 )
		{
			fprintf(stderr, "%s: failed to send packet: %d\n", m_pcPluginId, iResult);
			tResult = PAPA_SCHLUMPF_RESULT_USBError;
		}
		else
		{
			iResult = __receivePacket((unsigned char *)&tResponse, sizeof(tResponse), &iTransfered, 500);
			if( iResult!=0 )
			{
				fprintf(stderr, "%s: failed to receive packet: %d\n", m_pcPluginId, iResult);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( iTransfered!=sizeof(tResponse) )
			{
				fprintf(stderr, "%s: received an unexpected amount of data. wanted %zd bytes, but got %d.\n", m_pcPluginId, sizeof(tResponse), iTransfered);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( tResponse.ulStatus!=USB_COMMAND_STATUS_Ok )
			{
				fprintf(stderr, "%s: received an error: %d.\n", m_pcPluginId, tResponse.ulStatus);
				tResult = PAPA_SCHLUMPF_RESULT_CommandFailed;
			}
			else
			{
				*pulData = tResponse.ulData;
				tResult = PAPA_SCHLUMPF_RESULT_Ok;
			}
		}
	}

	return tResult;
}



RESULT_INT_TRUE_OR_NIL_WITH_ERR PapaSchlumpfFlex::ioWrite(uint32_t ulAddress, uint32_t ulData)
{
	PAPA_SCHLUMPF_RESULT_T tResult;
	int iResult;
	int iTransfered;
	PAPA_SCHLUMPF_USB_COMMAND_DMA_IO_WRITE_T tCommand;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_STATUS_T tResponse;


	if( m_ptDevHandlePapaSchlumpf==NULL )
	{
		tResult = PAPA_SCHLUMPF_RESULT_NotConnected;
	}
	else
	{
		tCommand.ulCommand = PAPA_SCHLUMPF_USB_COMMAND_DMAIoWrite;
		tCommand.ulDeviceAddress = ulAddress;
		tCommand.ulData = ulData;
		iResult = __send_packet((const unsigned char *)&tCommand, sizeof(tCommand), 100);
		if( iResult!=0 )
		{
			fprintf(stderr, "%s: failed to send packet: %d\n", m_pcPluginId, iResult);
			tResult = PAPA_SCHLUMPF_RESULT_USBError;
		}
		else
		{
			iResult = __receivePacket((unsigned char *)&tResponse, sizeof(tResponse), &iTransfered, 500);
			if( iResult!=0 )
			{
				fprintf(stderr, "%s: failed to receive packet: %d\n", m_pcPluginId, iResult);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( iTransfered!=sizeof(tResponse) )
			{
				fprintf(stderr, "%s: received an unexpected amount of data. wanted %zd bytes, but got %d.\n", m_pcPluginId, sizeof(tResponse), iTransfered);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( tResponse.ulStatus!=USB_COMMAND_STATUS_Ok )
			{
				fprintf(stderr, "%s: received an error: %d.\n", m_pcPluginId, tResponse.ulStatus);
				tResult = PAPA_SCHLUMPF_RESULT_CommandFailed;
			}
			else
			{
				tResult = PAPA_SCHLUMPF_RESULT_Ok;
			}
		}
	}

	return tResult;
}



RESULT_INT_TRUE_OR_NIL_WITH_ERR PapaSchlumpfFlex::memWrite(uint32_t ulAddress, uint32_t ulData)
{
	PAPA_SCHLUMPF_RESULT_T tResult;
	int iResult;
	int iTransfered;
	PAPA_SCHLUMPF_USB_COMMAND_DMA_MEM_WRITE_T tCommand;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_STATUS_T tResponse;


	if( m_ptDevHandlePapaSchlumpf==NULL )
	{
		tResult = PAPA_SCHLUMPF_RESULT_NotConnected;
	}
	else
	{
		tCommand.ulCommand = PAPA_SCHLUMPF_USB_COMMAND_DMAMemWrite;
		tCommand.ulDeviceAddress = ulAddress;
		tCommand.ulData = ulData;
		iResult = __send_packet((const unsigned char *)&tCommand, sizeof(tCommand), 100);
		if( iResult!=0 )
		{
			fprintf(stderr, "%s: failed to send packet: %d\n", m_pcPluginId, iResult);
			tResult = PAPA_SCHLUMPF_RESULT_USBError;
		}
		else
		{
			iResult = __receivePacket((unsigned char *)&tResponse, sizeof(tResponse), &iTransfered, 500);
			if( iResult!=0 )
			{
				fprintf(stderr, "%s: failed to receive packet: %d\n", m_pcPluginId, iResult);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( iTransfered!=sizeof(tResponse) )
			{
				fprintf(stderr, "%s: received an unexpected amount of data. wanted %zd bytes, but got %d.\n", m_pcPluginId, sizeof(tResponse), iTransfered);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( tResponse.ulStatus!=USB_COMMAND_STATUS_Ok )
			{
				fprintf(stderr, "%s: received an error: %d.\n", m_pcPluginId, tResponse.ulStatus);
				tResult = PAPA_SCHLUMPF_RESULT_CommandFailed;
			}
			else
			{
				tResult = PAPA_SCHLUMPF_RESULT_Ok;
			}
		}
	}

	return tResult;
}



RESULT_INT_TRUE_OR_NIL_WITH_ERR PapaSchlumpfFlex::cfg0Write(uint32_t ulAddress, uint32_t ulData)
{
	PAPA_SCHLUMPF_RESULT_T tResult;
	int iResult;
	int iTransfered;
	PAPA_SCHLUMPF_USB_COMMAND_DMA_CFG0_WRITE_T tCommand;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_STATUS_T tResponse;


	if( m_ptDevHandlePapaSchlumpf==NULL )
	{
		tResult = PAPA_SCHLUMPF_RESULT_NotConnected;
	}
	else
	{
		tCommand.ulCommand = PAPA_SCHLUMPF_USB_COMMAND_DMACfg0Write;
		tCommand.ulDeviceAddress = ulAddress;
		tCommand.ulData = ulData;
		iResult = __send_packet((const unsigned char *)&tCommand, sizeof(tCommand), 100);
		if( iResult!=0 )
		{
			fprintf(stderr, "%s: failed to send packet: %d\n", m_pcPluginId, iResult);
			tResult = PAPA_SCHLUMPF_RESULT_USBError;
		}
		else
		{
			iResult = __receivePacket((unsigned char *)&tResponse, sizeof(tResponse), &iTransfered, 500);
			if( iResult!=0 )
			{
				fprintf(stderr, "%s: failed to receive packet: %d\n", m_pcPluginId, iResult);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( iTransfered!=sizeof(tResponse) )
			{
				fprintf(stderr, "%s: received an unexpected amount of data. wanted %zd bytes, but got %d.\n", m_pcPluginId, sizeof(tResponse), iTransfered);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( tResponse.ulStatus!=USB_COMMAND_STATUS_Ok )
			{
				fprintf(stderr, "%s: received an error: %d.\n", m_pcPluginId, tResponse.ulStatus);
				tResult = PAPA_SCHLUMPF_RESULT_CommandFailed;
			}
			else
			{
				tResult = PAPA_SCHLUMPF_RESULT_Ok;
			}
		}
	}

	return tResult;
}



RESULT_INT_TRUE_OR_NIL_WITH_ERR PapaSchlumpfFlex::cfg1Write(uint32_t ulAddress, uint32_t ulData)
{
	PAPA_SCHLUMPF_RESULT_T tResult;
	int iResult;
	int iTransfered;
	PAPA_SCHLUMPF_USB_COMMAND_DMA_CFG1_WRITE_T tCommand;
	PAPA_SCHLUMPF_USB_COMMAND_RESULT_STATUS_T tResponse;


	if( m_ptDevHandlePapaSchlumpf==NULL )
	{
		tResult = PAPA_SCHLUMPF_RESULT_NotConnected;
	}
	else
	{
		tCommand.ulCommand = PAPA_SCHLUMPF_USB_COMMAND_DMACfg1Write;
		tCommand.ulDeviceAddress = ulAddress;
		tCommand.ulData = ulData;
		iResult = __send_packet((const unsigned char *)&tCommand, sizeof(tCommand), 100);
		if( iResult!=0 )
		{
			fprintf(stderr, "%s: failed to send packet: %d\n", m_pcPluginId, iResult);
			tResult = PAPA_SCHLUMPF_RESULT_USBError;
		}
		else
		{
			iResult = __receivePacket((unsigned char *)&tResponse, sizeof(tResponse), &iTransfered, 500);
			if( iResult!=0 )
			{
				fprintf(stderr, "%s: failed to receive packet: %d\n", m_pcPluginId, iResult);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( iTransfered!=sizeof(tResponse) )
			{
				fprintf(stderr, "%s: received an unexpected amount of data. wanted %zd bytes, but got %d.\n", m_pcPluginId, sizeof(tResponse), iTransfered);
				tResult = PAPA_SCHLUMPF_RESULT_USBError;
			}
			else if( tResponse.ulStatus!=USB_COMMAND_STATUS_Ok )
			{
				fprintf(stderr, "%s: received an error: %d.\n", m_pcPluginId, tResponse.ulStatus);
				tResult = PAPA_SCHLUMPF_RESULT_CommandFailed;
			}
			else
			{
				tResult = PAPA_SCHLUMPF_RESULT_Ok;
			}
		}
	}

	return tResult;
}



RESULT_INT_TRUE_OR_NIL_WITH_ERR PapaSchlumpfFlex::disconnect(void)
{
	__disconnect();
	return PAPA_SCHLUMPF_RESULT_Ok;
}



/* Look through all USB devices. If one Papa Schlumpf device was found, open it.
 * 0 or more than one Papa Schlumpf device results in an error.
 */
PAPA_SCHLUMPF_RESULT_T PapaSchlumpfFlex::__scan_for_papa_schlumpf_hardware(void)
{
	PAPA_SCHLUMPF_RESULT_T tResult;
	int iLibUsbResult;
	ssize_t ssizDevList;
	libusb_device **ptDeviceList;
	libusb_device **ptDevCnt, **ptDevEnd;
	libusb_device *ptDevice;
	libusb_device *ptDevPapaSchlumpf;
	libusb_device_handle *ptDevHandle;
	libusb_device_handle *ptDevHandlePapaSchlumpf;
	struct libusb_device_descriptor sDevDesc;


	/* Expect success. */
	tResult = PAPA_SCHLUMPF_RESULT_Ok;

	/* No device open yet. */
	ptDevHandlePapaSchlumpf = NULL;

	/* Get the list of all connected USB devices. */
	ptDeviceList = NULL;
	ssizDevList = libusb_get_device_list(m_ptLibUsbContext, &ptDeviceList);
	if( ssizDevList<0 )
	{
		/* Failed to get the list of all USB devices. */
		fprintf(stderr, "%s: failed to detect USB devices: %ld:%s\n", m_pcPluginId, ssizDevList, libusb_strerror(libusb_error(ssizDevList)));
		tResult = PAPA_SCHLUMPF_RESULT_USBError;
	}
	else
	{
		/* No Papa Schlumpf device found yet. */
		ptDevPapaSchlumpf = NULL;

		/* Loop over all devices. */
		ptDevCnt = ptDeviceList;
		ptDevEnd = ptDevCnt + ssizDevList;
		while( ptDevCnt<ptDevEnd )
		{
			ptDevice = *ptDevCnt;

			/* Try to open the device. */
			iLibUsbResult = libusb_open(ptDevice, &ptDevHandle);
			if( iLibUsbResult==LIBUSB_SUCCESS )
			{
				/* Get the device descriptor. */
				iLibUsbResult = libusb_get_descriptor(ptDevHandle, LIBUSB_DT_DEVICE, 0, (unsigned char*)&sDevDesc, sizeof(struct libusb_device_descriptor));
				if( iLibUsbResult==sizeof(struct libusb_device_descriptor) )
				{
					printf("Looking at USB device VID=0x%04x, PID=0x%04x\n", sDevDesc.idVendor, sDevDesc.idProduct);

					/* Is this a "Papa Schlumpf" device? */
					if( sDevDesc.idVendor==PAPA_SCHLUMPF_USB_VENDOR_ID && sDevDesc.idProduct==PAPA_SCHLUMPF_USB_PRODUCT_ID )
					{
						printf("Found a Papa Schlumpf device!\n");

						if( ptDevPapaSchlumpf == NULL)
						{
							/* Check if this device is the first which gets found*/
							ptDevPapaSchlumpf = ptDevice;
						}
						else
						{
							printf("Error! More than one Papa Schlumpf device connected!\n");
							tResult = PAPA_SCHLUMPF_RESULT_MoreThanOneDeviceFound;
							break;
						}
					}
				}

				/* Close the device. */
				libusb_close(ptDevHandle);
			}

			/* Next device in the list... */
			++ptDevCnt;
		}

		/* Did no error occur and found a device? */
		if( tResult==PAPA_SCHLUMPF_RESULT_Ok )
		{
			if( ptDevPapaSchlumpf==NULL )
			{
				fprintf(stderr, "%s: no hardware found!\n", m_pcPluginId);
				tResult = PAPA_SCHLUMPF_RESULT_NoDeviceFound;
			}
			else
			{
				/* Open the device. */
				iLibUsbResult = libusb_open(ptDevPapaSchlumpf, &ptDevHandlePapaSchlumpf);
				if( iLibUsbResult==LIBUSB_SUCCESS )
				{
					/* Claim interface #0. */
					iLibUsbResult = libusb_claim_interface(ptDevHandlePapaSchlumpf, 0);
					if( iLibUsbResult==LIBUSB_SUCCESS )
					{
						/* The handle to the device is now open. */
						m_ptDevHandlePapaSchlumpf = ptDevHandlePapaSchlumpf;
					}
					else
					{
						fprintf(stderr, "%s: failed to claim the interface: %d:%s\n", m_pcPluginId, iLibUsbResult, libusb_strerror(libusb_error(iLibUsbResult)));
						libusb_close(ptDevHandlePapaSchlumpf);
						tResult = PAPA_SCHLUMPF_RESULT_USBError;
					}
				}
				else
				{
					fprintf(stderr, "%s: failed to open the usb devices: %d:%s\n", m_pcPluginId, iLibUsbResult, libusb_strerror(libusb_error(iLibUsbResult)));
					tResult = PAPA_SCHLUMPF_RESULT_USBError;
				}
			}
		}


		/* Free the device list. */
		if( ptDeviceList!=NULL )
		{
			libusb_free_device_list(ptDeviceList, 1);
		}
	}

	return tResult;
}



int PapaSchlumpfFlex::__send_packet(const unsigned char *pucOutBuf, int sizOutBuf, unsigned int uiTimeoutMs)
{
	int iResult;
	int iTransfered = 0;
	/* libusb_bulk_transfer expects a non-const pointer, but this is an out transfer. */
	unsigned char *pucBuf = (unsigned char*)pucOutBuf;


	iResult = libusb_bulk_transfer(m_ptDevHandlePapaSchlumpf, 0x01, pucBuf, sizOutBuf, &iTransfered, uiTimeoutMs);
	return iResult;
}



int PapaSchlumpfFlex::__receivePacket(unsigned char *pucInBuf, int sizInBufMax, int *psizInBuf, unsigned int uiTimeoutMs)
{
	int iResult;


	iResult = libusb_bulk_transfer(m_ptDevHandlePapaSchlumpf, 0x81, pucInBuf, sizInBufMax, psizInBuf, uiTimeoutMs);
	return iResult;
}



void PapaSchlumpfFlex::__disconnect(void)
{
	if( m_ptLibUsbContext!=NULL )
	{
		if( m_ptDevHandlePapaSchlumpf!=NULL )
		{
			/* Release and close the handle. */
			libusb_release_interface(m_ptDevHandlePapaSchlumpf, 0);
			libusb_close(m_ptDevHandlePapaSchlumpf);

			m_ptDevHandlePapaSchlumpf = NULL;
		}

		/* Close the libusb context. */
		libusb_exit(m_ptLibUsbContext);
		m_ptLibUsbContext = NULL;
	}
}


#if 0
static PAPA_SCHLUMPF_ERROR_T send_start_packet()
{
	PAPA_SCHLUMPF_ERROR_T tResult;
	int iLibUsbResult;
	union
	{
		unsigned char auc[sizeof(PAPA_SCHLUMPF_PARAMETER_T)];
		PAPA_SCHLUMPF_PARAMETER_T tParameter;
	} uData;
	int iLength;
	unsigned int uiTimeoutMs;
	int iTransfered;


	/* Expect success. */
	tResult = PAPA_SCHLUMPF_ERROR_Ok;

	/* Fill the struct. */
	//uData.tParameter.ulDummy = 0x12345678;
	uData.tParameter.ulPapaSchlumpfUsbCommand = USB_COMMAND_Start;

	/* Wait at most 1 second. */
	uiTimeoutMs = 1000;

	/* Send data to endpoint 1. */
	iLength = sizeof(PAPA_SCHLUMPF_PARAMETER_T);
	iLibUsbResult = libusb_bulk_transfer(g_ptDevHandlePapaSchlumpf, 0x01, uData.auc, iLength, &iTransfered, uiTimeoutMs);
	if( iLibUsbResult==LIBUSB_SUCCESS )
	{
		/* Were all bytes transfered? */
		if( iTransfered!=iLength )
		{
			/* Not all bytes were transfered. */
			tResult = PAPA_SCHLUMPF_ERROR_TransferError;
		}
	}
	else
	{
		fprintf(stderr, "%s: failed to write data: %d:%s\n", g_pcPluginId, iLibUsbResult, libusb_strerror(libusb_error(iLibUsbResult)));
		tResult = PAPA_SCHLUMPF_ERROR_TransferError;
	}

	return tResult;
}

/* Receive the print messages and the final result.  */
static PAPA_SCHLUMPF_ERROR_T receive_report(char **ppcResponse, size_t *psizResponse, uint32_t *pulStatus)
{
	PAPA_SCHLUMPF_ERROR_T tResult;
	int iLibUsbResult;
	int iLength;
	unsigned int uiTimeoutMs;
	int iTransfered;
	char *pcResponse;
    size_t sizResponse;
	char *pcResponseNew;
    size_t sizResponseNew;
	union
	{
		unsigned char auc[4096];
		char ac[4096];
		uint32_t aul[4096/sizeof(uint32_t)];
	} uData;
	uint32_t ulStatus;
	int iEndPacketReceived;


	/* Expect success. */
	tResult = PAPA_SCHLUMPF_ERROR_Ok;

    pcResponse = NULL;
    sizResponse = 0;
    iEndPacketReceived = 0;

    while( iEndPacketReceived==0 )
    {
        /* Receive data packets in 4096 byte chunks. */
        iLength = 4096;
        uiTimeoutMs = 10000;
        iLibUsbResult = libusb_bulk_transfer(g_ptDevHandlePapaSchlumpf, 0x81,(unsigned char *) uData.ac, iLength, &iTransfered, uiTimeoutMs);
        if( iLibUsbResult==LIBUSB_SUCCESS )
        {
            /* Is this the special end packet? */
            if( iTransfered==8 && uData.aul[0]==0xffffffff )
            {
                ulStatus = uData.aul[1];
                iEndPacketReceived = 1;
            }
            else if( iTransfered>0 )
            {
                /* Add the new chunk to the buffer. */
                if( pcResponse==NULL )
                {
                    /* Allocate a new buffer. */
                    pcResponseNew = (char*)malloc(iTransfered);
                    if( pcResponseNew==NULL )
                    {
                        tResult = PAPA_SCHLUMPF_ERROR_MemoryError;
                        break;
                    }
                    else
                    {
                        /* Copy the received data into the new buffer. */
                        pcResponse = pcResponseNew;
                        memcpy(pcResponse, uData.ac, iTransfered);
                        sizResponse = iTransfered;
                    }
                }
                else
                {
                    sizResponseNew = sizResponse + iTransfered;
                    /* Allocate a new buffer. */
                    pcResponseNew = (char*)malloc(sizResponseNew);
                    if( pcResponseNew==NULL )
                    {
                        tResult = PAPA_SCHLUMPF_ERROR_MemoryError;
                        break;
                    }
                    else
                    {
                        /* Copy the old data into the new buffer. */
                        memcpy(pcResponseNew, pcResponse, sizResponse);
                        /* Append the received data to the new buffer. */
                        memcpy(pcResponseNew+sizResponse, uData.ac, iTransfered);
                        /* Free the old buffer. */
                        free(pcResponse);
                        /* Use the new buffer from now on. */
                        pcResponse = pcResponseNew;
                        sizResponse = sizResponseNew;
                    }
                }
            }
        }
        else
        {
            tResult = PAPA_SCHLUMPF_ERROR_TransferError;
            break;
        }
	}

    if( tResult!=PAPA_SCHLUMPF_ERROR_Ok )
    {
        if( pcResponse!=NULL )
        {
            free(pcResponse);
        }
        pcResponse = NULL;
        sizResponse = 0;
    }

    *ppcResponse = pcResponse;
    *psizResponse = sizResponse;
    *pulStatus = ulStatus;

	return tResult;
}

/* Start here! */
void run_test(void)
{
	/* Our result for the function. */
	unsigned long ulResult;

	/* Results from libusb. */
	int iResult;

	const struct libusb_version *ptLibUsbVersion;

	char *pcResponse;
	size_t sizResponse;
	uint32_t ulStatus;

	pcResponse = NULL;
	sizResponse = 0;

	/* Initialize global variables. */
	g_ptLibUsbContext = NULL;
	g_ptDevHandlePapaSchlumpf = NULL;

	/* Expect success. */
	ulResult = PAPA_SCHLUMPF_ERROR_Ok;

	/* Say "hi". */
	printf("Papa Schlumpf\n");

	/* Show the libusb version. */
	ptLibUsbVersion = libusb_get_version();
	printf("Using libusb %d.%d.%d.%d%s\n", ptLibUsbVersion->major, ptLibUsbVersion->minor, ptLibUsbVersion->micro, ptLibUsbVersion->nano, ptLibUsbVersion->rc);

	/* Initialize the libusb context. */
	iResult = libusb_init(&g_ptLibUsbContext);
	if( iResult!=0 )
	{
		printf("Error initializing libusb: %d\n", iResult);
		ulResult = PAPA_SCHLUMPF_ERROR_LibUsb_Open;
	}
	else
	{
		/* Set the debug level to a bit more verbose. */
		libusb_set_option(g_ptLibUsbContext, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);

		printf("libusb is open!\n");

		/* Search for exactly one papa schlumpf hardware and open it. */
		ulResult = scan_for_papa_schlumpf_hardware();
		if( ulResult==PAPA_SCHLUMPF_ERROR_Ok )
		{
			printf("Found HW!\n");
#if 0
			/* Initialize the device. */
			ptDevice = new PapaSchlumpfDevice(g_ptDevHandlePapaSchlumpf, g_ptLibUsbContext);

			/* Send a packet. */
			ulResult = send_start_packet();
			if( ulResult==PAPA_SCHLUMPF_ERROR_Ok )
			{
				/* TODO: receive response. */
				ulResult = receive_report(&pcResponse, &sizResponse, &ulStatus);
				if( ulResult==PAPA_SCHLUMPF_ERROR_Ok )
				{
					if( ulStatus!=0 )
					{
						ulResult = PAPA_SCHLUMPF_ERROR_netxError;
					}
				}
			}
#endif

			/* Release and close the handle. */
			libusb_release_interface(g_ptDevHandlePapaSchlumpf, 0);
			libusb_close(g_ptDevHandlePapaSchlumpf);
		}

		/* Close the libusb context. */
		libusb_exit(g_ptLibUsbContext);
	}
}
#endif



const PapaSchlumpfFlex::ERRORMESSAGE_T PapaSchlumpfFlex::atErrorMessages[] =
{
	{
		PAPA_SCHLUMPF_RESULT_Ok,
		"No error."
	},
	{
		PAPA_SCHLUMPF_RESULT_NotConnected,
		"Not connected, call connect first."
	},
	{
		PAPA_SCHLUMPF_RESULT_USBError,
		"An USB error occured. Please examine the debug log for details."
	},
	{
		PAPA_SCHLUMPF_RESULT_MoreThanOneDeviceFound,
		"More than one Papa Schlumpf device was found. This plugin can only handle one connected device. Please disconnect all devices except one."
	},
	{
		PAPA_SCHLUMPF_RESULT_NoDeviceFound,
		"No Papa Schlumpf device found."
	},
	{
		PAPA_SCHLUMPF_RESULT_CommandFailed,
		"The Papa Schlumpf device returned an error running the command."
	}
};



const char *PapaSchlumpfFlex::get_error_string(int iResult)
{
	PAPA_SCHLUMPF_RESULT_T tResult;
	const char *pcErrorString;
	const ERRORMESSAGE_T *ptCnt, *ptEnd;


	tResult = (PAPA_SCHLUMPF_RESULT_T)iResult;

	pcErrorString = "Unknown Error.";
	ptCnt = atErrorMessages;
	ptEnd = atErrorMessages + (sizeof(atErrorMessages)/sizeof(atErrorMessages[0]));
	while( ptCnt<ptEnd )
	{
		if( ptCnt->tResult==tResult )
		{
			pcErrorString = ptCnt->pcMessage;
			break;
		}
		++ptCnt;
	}

	return pcErrorString;
}
