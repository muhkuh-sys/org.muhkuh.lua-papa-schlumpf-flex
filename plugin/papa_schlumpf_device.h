/***************************************************************************
 *   Copyright (C) 2007 by Christoph Thelen                                *
 *   doc_bacardi@users.sourceforge.net                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdint.h>
#ifndef SWIG
#       include "papa_schlumpf_firmware_interface.h"
#       include "libusb.h"
#endif


#ifndef __PAPA_SCHLUMPF_DEVICE_H__
#define __PAPA_SCHLUMPF_DEVICE_H__

/*-----------------------------------*/

#if defined(_WINDOWS)
#       define MUHKUH_EXPORT __declspec(dllexport)
#else
#       define MUHKUH_EXPORT
#endif

#ifndef SWIGRUNTIME
#include <swigluarun.h>

typedef struct
{
	lua_State* L; /* the state */
	int ref;      /* a ref in the lua global index */
} SWIGLUA_REF;
#endif


class MUHKUH_EXPORT PapaSchlumpfDevice
{
public:
	PapaSchlumpfDevice(void);
#ifndef SWIG
	PapaSchlumpfDevice(libusb_device_handle* pLibusbDeviceHandle, libusb_context* pLibUsbContext);
#endif
	~PapaSchlumpfDevice();


// *** LUA interface start ***

	void reset_pci(void);

// *** LUA interface end ***

#ifndef SWIG

private:
	union
	{
		PAPA_SCHLUMPF_PARAMETER_T tPapaSchlumpfParameter;
		uint8_t auc[sizeof(PAPA_SCHLUMPF_PARAMETER_T)];
	} uPapaSchlumpfParameterData;
	union
	{
		uint8_t  aucBuf[64];
		uint16_t ausBuf[64 / sizeof(uint16_t)];
		uint32_t aulBuf[64 / sizeof(uint32_t)];
	} uDataBuf;
	libusb_device_handle *g_pLibusbDeviceHandle;
	libusb_context       *g_pLibusbContext;
	void copyData(const char* aucSource, uint8_t* aucDestin, uint32_t sizBytes);
	void copyData(uint8_t* aucSource, char* aucDestin, uint32_t sizBytes);

	USB_STATUS_T sendAndReceivePackets(uint8_t* aucOutBuf, uint32_t sizOutBuf, uint8_t* aucInBuf, uint32_t* sizInBuf);
	USB_STATUS_T sendPacket(uint8_t* aucOutBuf, uint32_t sizOutBuf);
	USB_STATUS_T receivePacket(uint8_t* aucInBuf, uint32_t* pSizInBuf);
#endif
};

/*-----------------------------------*/

#endif  /* __PAPA_SCHLUMPF_DEVICE_H__ */

