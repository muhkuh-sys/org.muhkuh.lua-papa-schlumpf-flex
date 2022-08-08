/***************************************************************************
 *   Copyright (C) 2010 by Christoph Thelen                                *
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

#include "usb_globals.h"

/*-------------------------------------------------------------------------*/

USB_State_t globalState;

unsigned int currentConfig;

/* Buffer for setup and data packets. */
unsigned char setupBuffer[Usb_Ep0_BufferSize];

/* Decoded packet. */
setupPacket_t tSetupPkt;
/* Out transaction needed. */
USB_SetupTransaction_t tOutTransactionNeeded;

USB_ReceiveEndpoint_t tReceiveEpState;
USB_SendEndpoint_t tSendEpState;

/* New address for pending address change. */
unsigned int uiNewAddress;

/*-------------------------------------------------------------------------*/
