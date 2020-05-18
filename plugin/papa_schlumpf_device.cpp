#include "papa_schlumpf_device.h"

#include <stdio.h>
#include <string.h>

#define MUHKUH_PLUGIN_PUSH_ERROR(L,...) { lua_pushfstring(L,__VA_ARGS__); }
#define MUHKUH_PLUGIN_EXIT_ERROR(L) { lua_error(L); } //Springt aus der aktuellen Funktion raus
#define MUHKUH_PLUGIN_ERROR(L,...) { lua_pushfstring(L,__VA_ARGS__); lua_error(L); }


PapaSchlumpfDevice::PapaSchlumpfDevice(void)
{
	this->g_pLibusbDeviceHandle = NULL;
	this->g_pLibusbContext      = NULL;
}

PapaSchlumpfDevice::PapaSchlumpfDevice(libusb_device_handle* libusbDeviceHandle, libusb_context* libusbContext)
{
	this->g_pLibusbDeviceHandle = libusbDeviceHandle;
	this->g_pLibusbContext      = libusbContext;
}

PapaSchlumpfDevice::~PapaSchlumpfDevice()
{
	/* Release and close the handle. */
	libusb_release_interface(this->g_pLibusbDeviceHandle, 0);
	libusb_close(this->g_pLibusbDeviceHandle);

	/* Close the libusb context. */
	libusb_exit(this->g_pLibusbContext);
}
#if 0
uint8_t PapaSchlumpfDevice::read_data08(lua_State *ptClientData, uint32_t ulDataSourceAddress)
{
	/*Init values*/
	uint32_t    sizInBuf = 0;
	USB_STATUS_T tUsbStatus;
	PAPA_SCHLUMPF_USB_COMMAND_STATUS_T tPapaSchlumpfUsbCommandStatus;

	bool isRunning = true;
	bool isOk = true;

	/*Prepare the Parameter struct*/
	this->uPapaSchlumpfParameterData
		  .tPapaSchlumpfParameter
		  .ulPapaSchlumpfUsbCommand      = USB_COMMAND_Read_8;

	this->uPapaSchlumpfParameterData
		  .tPapaSchlumpfParameter
		  .uPapaSchlumpfUsbParameter
		  .tPapaSchlumpfUsbParameterRead
		  .ulAddress                     = ulDataSourceAddress;

	this->uPapaSchlumpfParameterData
		  .tPapaSchlumpfParameter
		  .uPapaSchlumpfUsbParameter
		  .tPapaSchlumpfUsbParameterRead
		  .ulElemCount                   = 1;

	/*Send Command*/
	tUsbStatus = sendAndReceivePackets(this->uPapaSchlumpfParameterData.auc, sizeof(this->uPapaSchlumpfParameterData), uDataBuf.aucBuf, &sizInBuf);
	if(tUsbStatus != USB_STATUS_OK)
	{
		MUHKUH_PLUGIN_PUSH_ERROR(ptClientData, "%p: failed to send command!\n", this);
		isOk = false;
	}

	/*Check if error in response*/
	tPapaSchlumpfUsbCommandStatus = PAPA_SCHLUMPF_USB_COMMAND_STATUS_T(uDataBuf.aulBuf[0]);
	if(tPapaSchlumpfUsbCommandStatus != USB_COMMAND_STATUS_Ok)
	{
		MUHKUH_PLUGIN_PUSH_ERROR(ptClientData,"%s: failed to execute command!\n", this->m_tChiptyp);
		isOk = false;
	}

	if(!isOk)
	{
		printf("Exit Error\n");
		MUHKUH_PLUGIN_EXIT_ERROR(ptClientData);
	}

	return uDataBuf.aucBuf[4];
}
#endif

USB_STATUS_T PapaSchlumpfDevice::sendAndReceivePackets(uint8_t* aucOutBuf, uint32_t sizOutBuf, uint8_t* aucInBuf, uint32_t* sizInBuf)
{
	USB_STATUS_T tUsbStatus;

	tUsbStatus = this->sendPacket(aucOutBuf, sizOutBuf);
	if(tUsbStatus != USB_STATUS_OK)
	{
		return tUsbStatus;
	}

	tUsbStatus = this->receivePacket(aucInBuf, sizInBuf);
	if(tUsbStatus != USB_STATUS_OK)
	{
		return tUsbStatus;
	}

	return tUsbStatus;
}

USB_STATUS_T PapaSchlumpfDevice::sendPacket(uint8_t* aucOutBuf, uint32_t sizOutBuf)
{
	int iResult     = 0;
	int iTransfered = 0;
	int iTimeoutMs  = 60000;

	iResult = libusb_bulk_transfer(this->g_pLibusbDeviceHandle, 0x01, aucOutBuf, sizOutBuf, &iTransfered, iTimeoutMs);

	if(iResult != LIBUSB_SUCCESS)
	{
		//fprintf(stderr, "USB Error: Writing Data to Papa Schlumpf device failed!\n");
		return USB_STATUS_ERROR;
	}
	else
	{
		/*Check if all bytes were transfered*/
		if(iTransfered != sizOutBuf)
		{
			//fprintf(stderr, "USB Error: Not all bytes written to Papa Schlumpf device!\n");
			return USB_STATUS_NOT_ALL_BYTES_TRANSFERED;
		}
	}

	return USB_STATUS_OK;
}

USB_STATUS_T PapaSchlumpfDevice::receivePacket(uint8_t* aucInBuf, uint32_t* pSizInBuf)
{
	int iResult        = 0;
	int iTransfered    = 0;
	int iTimeoutMs     = 60000;

	iResult = libusb_bulk_transfer(this->g_pLibusbDeviceHandle, 0x81, aucInBuf, 64, &iTransfered, iTimeoutMs);

	if(iResult != LIBUSB_SUCCESS)
	{
		return USB_STATUS_ERROR;
	}
	else
	{
		if (iTransfered == 0)
		{
			return USB_STATUS_RECEIVED_EMPTY_PACKET;
		}
		else
		{
			*pSizInBuf = iTransfered;
		}
	}

	return USB_STATUS_OK;
}
