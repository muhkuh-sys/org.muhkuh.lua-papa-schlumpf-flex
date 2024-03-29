#include <stdint.h>
#include <string.h>

#include "monitor.h"
#include "monitor_commands.h"
#include "ringbuffer.h"
#include "serial_vectors.h"


typedef enum MONITOR_PACKET_STATE_ENUM
{
	MONITOR_PACKET_STATE_WaitForPacketStart     = 0,
	MONITOR_PACKET_STATE_WaitForSizeComplete    = 1,
	MONITOR_PACKET_STATE_WaitForPacketComplete  = 2,
	MONITOR_PACKET_STATE_ExecutePacket          = 3
} MONITOR_PACKET_STATE_T;


typedef enum MONITOR_COMMUNICATION_STATE_ENUM
{
//	MONITOR_COMMUNICATION_STATE_Disconnected                  = 0,
	MONITOR_COMMUNICATION_STATE_Connected                     = 0,
	MONITOR_COMMUNICATION_STATE_CommandResponseWaitForAck     = 1,
	MONITOR_COMMUNICATION_STATE_InCall                        = 2,
//	MONITOR_COMMUNICATION_STATE_CallDataWaitForAck            = 3
} MONITOR_COMMUNICATION_STATE_T;


typedef enum MONITOR_ACCESSSIZE_ENUM
{
	MONITOR_ACCESSSIZE_08  = 0,
	MONITOR_ACCESSSIZE_16  = 1,
	MONITOR_ACCESSSIZE_32  = 2,
	MONITOR_ACCESSSIZE_64  = 3
} MONITOR_ACCESSSIZE_T;


typedef struct MONITOR_HANDLE_STRUCT
{
	MONITOR_PACKET_STATE_T tPacketState;
	unsigned int uiPacketSize;

	PFN_TRANSPORT_RECEIVE pfnTransportReceive;
	PFN_TRANSPORT_SEND_PACKET pfnTransportSendPacket;
	void *pvTransportUserData;

	MONITOR_COMMUNICATION_STATE_T tCommunicationState;

	unsigned int sizTxBuffer;
	unsigned int sizCallTxData;

	unsigned char aucTxBuffer[MONITOR_MAXIMUM_PACKET_SIZE];

	/* The "call tx" buffer holds the pure message data. */
	unsigned char aucCallTxBuffer[320U];
} MONITOR_HANDLE_T;


typedef union ADR_UNION
{
	unsigned long ul;
	uint8_t *puc;
	uint16_t *pus;
	uint32_t *pul;
	uint64_t *pull;
} ADR_T;


typedef union VAL_UNION
{
	uint8_t uc;
	uint16_t us;
	uint32_t ul;
	uint64_t ull;
} VAL_T;


typedef void (*PFN_MONITOR_CALL_T)(unsigned long ulR0);


typedef union PFN_UNION
{
	unsigned long ul;
	PFN_MONITOR_CALL_T pfn;
} PFN_T;



static MONITOR_HANDLE_T tMonitorHandle;

/* Define ringbuffer for receiving data.
 * The size should be at least the size of the receive mailbox.
 */
static DEFINE_RINGBUFFER(tRingbufferRx, MONITOR_MAXIMUM_PACKET_SIZE);

/*
static unsigned short get_data16(RINGBUFFER_T *ptRingBuffer)
{
	unsigned long ulValue;


	ulValue  =  (unsigned long)ringbuffer_get_char(ptRingBuffer);
	ulValue |= ((unsigned long)ringbuffer_get_char(ptRingBuffer) << 8U);

	return (unsigned short)ulValue;
}
*/

static unsigned short peek_data16(RINGBUFFER_T *ptRingBuffer, unsigned int uiOffset)
{
	unsigned long ulValue;


	ulValue  =  (unsigned long)ringbuffer_peek(ptRingBuffer, uiOffset+0U);
	ulValue |= ((unsigned long)ringbuffer_peek(ptRingBuffer, uiOffset+1U) << 8U);

	return (unsigned short)ulValue;
}


static unsigned short get_data16(RINGBUFFER_T *ptRingBuffer)
{
	unsigned long ulValue;


	ulValue  =  (unsigned long)ringbuffer_get_char(ptRingBuffer);
	ulValue |= ((unsigned long)ringbuffer_get_char(ptRingBuffer) << 8U);

	return (unsigned short)ulValue;
}


static unsigned long get_data32(RINGBUFFER_T *ptRingBuffer)
{
	unsigned long ulValue;


	ulValue  =  (unsigned long)ringbuffer_get_char(ptRingBuffer);
	ulValue |= ((unsigned long)ringbuffer_get_char(ptRingBuffer) << 8U);
	ulValue |= ((unsigned long)ringbuffer_get_char(ptRingBuffer) <<16U);
	ulValue |= ((unsigned long)ringbuffer_get_char(ptRingBuffer) <<24U);

	return ulValue;
}


/* This is a very nice routine for the CITT XModem CRC from
 * http://www.eagleairaust.com.au/code/crc16.htm or now
 * https://web.archive.org/web/20130106081206/http://www.eagleairaust.com.au/code/crc16.htm
 */
static unsigned short crc16(unsigned short usCrc, unsigned char ucData)
{
	unsigned int uiCrc;


	uiCrc  = (usCrc >> 8U) | ((usCrc & 0xffU) << 8U);
	uiCrc ^= ucData;
	uiCrc ^= (uiCrc & 0xffU) >> 4U;
	uiCrc ^= (uiCrc & 0x0fU) << 12U;
	uiCrc ^= ((uiCrc & 0xffU) << 4U) << 1U;

	return (unsigned short)uiCrc;
}



static unsigned short crc16_area(const unsigned char *pucData, unsigned int sizData)
{
	unsigned short usCrc;
	const unsigned char *pucCnt;
	const unsigned char *pucEnd;


	usCrc = 0U;
	pucCnt= pucData;
	pucEnd = pucData + sizData;
	while( pucCnt<pucEnd )
	{
		usCrc = crc16(usCrc, *(pucCnt++));
	}
	return usCrc;
}



static void send_status(MONITOR_STATUS_T tStatus)
{
	unsigned short usCrc;
	unsigned char *pucPacket;

	pucPacket = tMonitorHandle.aucTxBuffer;

	/* Construct the packet. */
	pucPacket[0] = MONITOR_PACKET_START;
	pucPacket[1] = 2U;
	pucPacket[2] = 0U;
	pucPacket[3] = MONITOR_PACKET_TYP_Status;
	pucPacket[4] = (unsigned char)tStatus;
	/* Get the CRC for the size and data part. */
	usCrc = crc16_area(pucPacket+1, 4U);
	pucPacket[5] = (unsigned char)( usCrc         & 0xff);
	pucPacket[6] = (unsigned char)((usCrc >>  8U) & 0xff);

	/* Send the packet. */
	tMonitorHandle.pfnTransportSendPacket(tMonitorHandle.pvTransportUserData, pucPacket, 7U);
}



static void papa_schlumpf_vector_flush(void)
{
	unsigned int sizCallTxData;
	unsigned int uiDataSize;
	unsigned int uiOutputSize;
	unsigned char *pucOutput;
	unsigned short usCrc;


	sizCallTxData = tMonitorHandle.sizCallTxData;
	if( sizCallTxData!=0 )
	{
		/* Get the data size of the packet.
		 * This includes the type and the message data.
		 */
		uiDataSize = 1U + sizCallTxData;
		uiOutputSize = 1U + 2U + uiDataSize + 2U;

		/* Create a new packet. */
		pucOutput = tMonitorHandle.aucTxBuffer;

		*(pucOutput++) = MONITOR_PACKET_START;
		*(pucOutput++) = (unsigned char)( uiDataSize      & 0xffU);
		*(pucOutput++) = (unsigned char)((uiDataSize>>8U) & 0xffU);
		*(pucOutput++) = MONITOR_PACKET_TYP_Call_Data;

		/* Copy the message data to the buffer. */
		memcpy(pucOutput, tMonitorHandle.aucCallTxBuffer, sizCallTxData);
		pucOutput += sizCallTxData;

		/* Build the CRC. */
		usCrc = crc16_area(tMonitorHandle.aucTxBuffer+1, 2U+uiDataSize);
		*(pucOutput++) = (unsigned char)( usCrc         & 0xff);
		*(pucOutput++) = (unsigned char)((usCrc >>  8U) & 0xff);

		/* Set the packet size. */
		tMonitorHandle.sizTxBuffer = uiOutputSize;
		tMonitorHandle.pfnTransportSendPacket(tMonitorHandle.pvTransportUserData, tMonitorHandle.aucTxBuffer, uiOutputSize);

		tMonitorHandle.sizCallTxData = 0;
	}
}



static void papa_schlumpf_vector_put(unsigned int uiChar)
{
	unsigned int sizCallTxData;


	/* Does the TX buffer have space left? */
	sizCallTxData = tMonitorHandle.sizCallTxData;
	if( sizCallTxData<sizeof(tMonitorHandle.aucCallTxBuffer) )
	{
		/* Append the char to the buffer. */
		tMonitorHandle.aucCallTxBuffer[sizCallTxData] = (unsigned char)(uiChar & 0xffU);
		tMonitorHandle.sizCallTxData = ++sizCallTxData;

		/* Send the data if the buffer is full. */
		if( sizCallTxData>=sizeof(tMonitorHandle.aucCallTxBuffer) )
		{
			papa_schlumpf_vector_flush();
		}
	}
}



static unsigned char papa_schlumpf_vector_get(void)
{
	return 0;
}



static unsigned int papa_schlumpf_vector_peek(void)
{
	return 0;
}



static void command_read(unsigned int uiPacketSize, MONITOR_ACCESSSIZE_T tAccessSize)
{
	RINGBUFFER_T *ptRingBufferRx;
	ADR_T tAddress;
	unsigned int uiDataSize;
	unsigned int uiOutputSize;
	VAL_T tVal;
	unsigned char *pucOutput;
	unsigned short usCrc;


	ptRingBufferRx = &(tRingbufferRx.tRingBuffer);

	/* The "read" command needs...
	 *   an address (4 bytes)
	 * This makes a total of 4 bytes.
	 */
	if( uiPacketSize!=4U )
	{
		/* Skip the complete packet and the CRC. */
		ringbuffer_skip(ptRingBufferRx, uiPacketSize+2U);

		/* Respond with an error. */
		send_status(MONITOR_STATUS_InvalidPacketSize);
	}
	else
	{
		tAddress.ul = get_data32(ptRingBufferRx);
		/* Skip the CRC. */
		ringbuffer_skip(ptRingBufferRx, 2U);

		/* Does the number of elements fit into the output buffer?
		 * The output size includes...
		 *   1 byte packet start
		 *   2 bytes data size
		 *   1 byte type information
		 *   data
		 *   2 bytes CRC
		 */
		uiDataSize = 1U;
		switch(tAccessSize)
		{
		case MONITOR_ACCESSSIZE_08:
			uiDataSize += sizeof(uint8_t);
			break;

		case MONITOR_ACCESSSIZE_16:
			uiDataSize += sizeof(uint16_t);
			break;

		case MONITOR_ACCESSSIZE_32:
			uiDataSize += sizeof(uint32_t);
			break;

		case MONITOR_ACCESSSIZE_64:
			uiDataSize += sizeof(uint64_t);
			break;
		}
		uiOutputSize = 1U + 2U + uiDataSize + 2U;

		/* Generate the output packet. */
		pucOutput = tMonitorHandle.aucTxBuffer;

		*(pucOutput++) = MONITOR_PACKET_START;
		*(pucOutput++) = (unsigned char)( uiDataSize      & 0xffU);
		*(pucOutput++) = (unsigned char)((uiDataSize>>8U) & 0xffU);
		*(pucOutput++) = MONITOR_PACKET_TYP_Read_Data;

		switch(tAccessSize)
		{
		case MONITOR_ACCESSSIZE_08:
			*(pucOutput++) = *(tAddress.puc++);
			break;

		case MONITOR_ACCESSSIZE_16:
			tVal.us = *(tAddress.pus++);
			*(pucOutput++) = (unsigned char)( tVal.us         & 0xffU);
			*(pucOutput++) = (unsigned char)((tVal.us >>  8U) & 0xffU);
			break;

		case MONITOR_ACCESSSIZE_32:
			tVal.ul = *(tAddress.pul++);
			*(pucOutput++) = (unsigned char)( tVal.ul         & 0xffU);
			*(pucOutput++) = (unsigned char)((tVal.ul >>  8U) & 0xffU);
			*(pucOutput++) = (unsigned char)((tVal.ul >> 16U) & 0xffU);
			*(pucOutput++) = (unsigned char)((tVal.ul >> 24U) & 0xffU);
			break;

		case MONITOR_ACCESSSIZE_64:
			tVal.ull = *(tAddress.pull++);
			*(pucOutput++) = (unsigned char)( tVal.ull         & 0xffU);
			*(pucOutput++) = (unsigned char)((tVal.ull >>  8U) & 0xffU);
			*(pucOutput++) = (unsigned char)((tVal.ull >> 16U) & 0xffU);
			*(pucOutput++) = (unsigned char)((tVal.ull >> 24U) & 0xffU);
			*(pucOutput++) = (unsigned char)((tVal.ull >> 32U) & 0xffU);
			*(pucOutput++) = (unsigned char)((tVal.ull >> 40U) & 0xffU);
			*(pucOutput++) = (unsigned char)((tVal.ull >> 48U) & 0xffU);
			*(pucOutput++) = (unsigned char)((tVal.ull >> 56U) & 0xffU);
			break;
		}

		/* Build the CRC. */
		usCrc = crc16_area(tMonitorHandle.aucTxBuffer+1, 2U+uiDataSize);
		*(pucOutput++) = (unsigned char)( usCrc         & 0xff);
		*(pucOutput++) = (unsigned char)((usCrc >>  8U) & 0xff);

		/* Set the packet size. */
		tMonitorHandle.sizTxBuffer = uiOutputSize;
		tMonitorHandle.pfnTransportSendPacket(tMonitorHandle.pvTransportUserData, tMonitorHandle.aucTxBuffer, uiOutputSize);
	}
}


static void command_read_area(unsigned int uiPacketSize)
{
	RINGBUFFER_T *ptRingBufferRx;
	ADR_T tAddress;
	unsigned int uiSize;
	unsigned int uiDataSize;
	unsigned int uiOutputSize;
	unsigned char *pucOutput;
	unsigned short usCrc;


	ptRingBufferRx = &(tRingbufferRx.tRingBuffer);

	/* The "read_area" command needs...
	 *   an address (4 bytes)
	 *   a size (2 bytes)
	 * This makes a total of 6 bytes.
	 */
	if( uiPacketSize!=6U )
	{
		/* Skip the complete packet and the CRC. */
		ringbuffer_skip(ptRingBufferRx, uiPacketSize+2U);

		/* Respond with an error. */
		send_status(MONITOR_STATUS_InvalidPacketSize);
	}
	else
	{
		tAddress.ul = get_data32(ptRingBufferRx);
		uiSize = get_data16(ptRingBufferRx);
		/* Skip the CRC. */
		ringbuffer_skip(ptRingBufferRx, 2U);

		/* Does the number of elements fit into the output buffer?
		 * The output size includes...
		 *   1 byte packet start
		 *   2 bytes data size
		 *   1 byte type information
		 *   data
		 *   2 bytes CRC
		 */
		uiDataSize = 1U + uiSize;
		uiOutputSize = 1U + 2U + uiDataSize + 2U;
		if( uiOutputSize>MONITOR_MAXIMUM_PACKET_SIZE )
		{
			send_status(MONITOR_STATUS_InvalidSizeParameter);
		}
		else
		{
			/* Generate the output packet. */
			pucOutput = tMonitorHandle.aucTxBuffer;

			*(pucOutput++) = MONITOR_PACKET_START;
			*(pucOutput++) = (unsigned char)( uiDataSize      & 0xffU);
			*(pucOutput++) = (unsigned char)((uiDataSize>>8U) & 0xffU);
			*(pucOutput++) = MONITOR_PACKET_TYP_Read_Data;

			/* Copy the data block to the buffer. */
			memcpy(pucOutput, tAddress.puc, uiSize);
			pucOutput += uiSize;

			/* Build the CRC. */
			usCrc = crc16_area(tMonitorHandle.aucTxBuffer+1, 2U+uiDataSize);
			*(pucOutput++) = (unsigned char)( usCrc         & 0xff);
			*(pucOutput++) = (unsigned char)((usCrc >>  8U) & 0xff);

			/* Set the packet size. */
			tMonitorHandle.sizTxBuffer = uiOutputSize;
			tMonitorHandle.pfnTransportSendPacket(tMonitorHandle.pvTransportUserData, tMonitorHandle.aucTxBuffer, uiOutputSize);
		}
	}
}



static void command_write(unsigned int uiPacketSize, MONITOR_ACCESSSIZE_T tAccessSize)
{
	RINGBUFFER_T *ptRingBufferRx;
	unsigned int uiExpectedSize;
	ADR_T tAddress;
	VAL_T tVal;


	ptRingBufferRx = &(tRingbufferRx.tRingBuffer);

	/* The "write" command needs...
	 *   an address (4 bytes)
	 *   data
	 */
	uiExpectedSize = 4U;
	switch(tAccessSize)
	{
	case MONITOR_ACCESSSIZE_08:
		uiExpectedSize += sizeof(uint8_t);
		break;

	case MONITOR_ACCESSSIZE_16:
		uiExpectedSize += sizeof(uint16_t);
		break;

	case MONITOR_ACCESSSIZE_32:
		uiExpectedSize += sizeof(uint32_t);
		break;

	case MONITOR_ACCESSSIZE_64:
		uiExpectedSize += sizeof(uint64_t);
		break;
	}
	if( uiPacketSize!=uiExpectedSize )
	{
		/* Skip the complete packet and the CRC. */
		ringbuffer_skip(ptRingBufferRx, uiPacketSize+2U);

		/* Respond with an error. */
		send_status(MONITOR_STATUS_InvalidPacketSize);
	}
	else
	{
		tAddress.ul = get_data32(ptRingBufferRx);

		switch(tAccessSize)
		{
		case MONITOR_ACCESSSIZE_08:
			*(tAddress.puc) = ringbuffer_get_char(ptRingBufferRx);
			break;

		case MONITOR_ACCESSSIZE_16:
			tVal.ul  = (unsigned long)(ringbuffer_get_char(ptRingBufferRx));
			tVal.ul |= (unsigned long)(ringbuffer_get_char(ptRingBufferRx)) <<  8U;
			*(tAddress.pus) = tVal.us;
			break;

		case MONITOR_ACCESSSIZE_32:
			tVal.ul  = (unsigned long)(ringbuffer_get_char(ptRingBufferRx));
			tVal.ul |= (unsigned long)(ringbuffer_get_char(ptRingBufferRx)) <<  8U;
			tVal.ul |= (unsigned long)(ringbuffer_get_char(ptRingBufferRx)) << 16U;
			tVal.ul |= (unsigned long)(ringbuffer_get_char(ptRingBufferRx)) << 24U;
			*(tAddress.pul) = tVal.ul;
			break;

		case MONITOR_ACCESSSIZE_64:
			tVal.ull  = (uint64_t)(ringbuffer_get_char(ptRingBufferRx));
			tVal.ull |= (uint64_t)(ringbuffer_get_char(ptRingBufferRx)) <<  8U;
			tVal.ull |= (uint64_t)(ringbuffer_get_char(ptRingBufferRx)) << 16U;
			tVal.ull |= (uint64_t)(ringbuffer_get_char(ptRingBufferRx)) << 24U;
			tVal.ull |= (uint64_t)(ringbuffer_get_char(ptRingBufferRx)) << 32U;
			tVal.ull |= (uint64_t)(ringbuffer_get_char(ptRingBufferRx)) << 40U;
			tVal.ull |= (uint64_t)(ringbuffer_get_char(ptRingBufferRx)) << 48U;
			tVal.ull |= (uint64_t)(ringbuffer_get_char(ptRingBufferRx)) << 56U;
			*(tAddress.pull) = tVal.ull;
			break;
		}

		/* Skip the CRC. */
		ringbuffer_skip(ptRingBufferRx, 2U);

		/* Send a status packet with "OK". */
		send_status(MONITOR_STATUS_Ok);
	}
}



static void command_write_area(unsigned int uiPacketSize)
{
	RINGBUFFER_T *ptRingBufferRx;
	ADR_T tAddress;
	unsigned int uiSize;


	ptRingBufferRx = &(tRingbufferRx.tRingBuffer);

	/* The "write_area" command needs at least...
	 *   4 bytes address
	 *   1 byte of data
	 * This makes a total of 5 bytes.
	 * There can be more data if the "size" is > 1.
	 */
	if( uiPacketSize<5U )
	{
		/* Skip the complete packet and the CRC. */
		ringbuffer_skip(ptRingBufferRx, uiPacketSize+2U);

		/* Respond with an error. */
		send_status(MONITOR_STATUS_InvalidPacketSize);
	}
	else
	{
		/* Get address and size. */
		tAddress.ul = get_data32(ptRingBufferRx);
		/* The packet size includes the address, which is in total 4 bytes. */
		uiSize = uiPacketSize - 4U;

		/* Copy the data from the packet to the memory. */
		ringbuffer_read(ptRingBufferRx, tAddress.puc, uiSize);

		/* Skip the CRC. */
		ringbuffer_skip(ptRingBufferRx, 2U);

		/* Send a status packet with "OK". */
		send_status(MONITOR_STATUS_Ok);
	}
}



static void command_call(unsigned int uiPacketSize)
{
	RINGBUFFER_T *ptRingBufferRx;
	PFN_T tPfn;
	unsigned long ulR0;


	ptRingBufferRx = &(tRingbufferRx.tRingBuffer);

	/* The "call" command needs...
	 *   4 bytes address
	 *   4 bytes parameter for R0
	 * This makes a total of 8 bytes.
	 */
	if( uiPacketSize!=8U )
	{
		/* Skip the complete packet and the CRC. */
		ringbuffer_skip(ptRingBufferRx, uiPacketSize+2U);

		/* Respond with an error. */
		send_status(MONITOR_STATUS_InvalidPacketSize);
	}
	else
	{
		tPfn.ul = get_data32(ptRingBufferRx);
		ulR0 = get_data32(ptRingBufferRx);
		/* Skip the CRC. */
		ringbuffer_skip(ptRingBufferRx, 2U);

		/* The call starts. */
		tMonitorHandle.tCommunicationState = MONITOR_COMMUNICATION_STATE_InCall;

		send_status(MONITOR_STATUS_Ok);

		/* Initialize the message buffer. */
		tMonitorHandle.sizCallTxData = 0U;

		/* Call the routine. */
		tPfn.pfn(ulR0);

		/* Flush any remaining bytes in the FIFO. */
		papa_schlumpf_vector_flush();

		/* The call finished, notify the PC. */
		send_status(MONITOR_STATUS_Call_Finished);
		tMonitorHandle.tCommunicationState = MONITOR_COMMUNICATION_STATE_Connected;
	}
}



static int monitor_process_packet(void)
{
	int iDone;
	unsigned char ucPacketTyp;
	MONITOR_PACKET_TYP_T tPacketTyp;
	int iPacketTypOk;
	RINGBUFFER_T *ptRingBufferReceive;
	unsigned int uiPacketSize;


	/* Processing is not done yet. */
	iDone = 0;

	ptRingBufferReceive = &(tRingbufferRx.tRingBuffer);

	/* Get the packet type. */
	ucPacketTyp = ringbuffer_get_char(ptRingBufferReceive);

	/* Get the packet size.
	 * 1 byte for the packet type was already processed.
	 */
	uiPacketSize = tMonitorHandle.uiPacketSize - 1U;

	/* Is this a valid packet type? */
	iPacketTypOk = 0;
	tPacketTyp = (MONITOR_PACKET_TYP_T)ucPacketTyp;
	switch(tPacketTyp)
	{
		case MONITOR_PACKET_TYP_Command_Read08:
		case MONITOR_PACKET_TYP_Command_Read16:
		case MONITOR_PACKET_TYP_Command_Read32:
		case MONITOR_PACKET_TYP_Command_Read64:
		case MONITOR_PACKET_TYP_Command_ReadArea:
		case MONITOR_PACKET_TYP_Command_Write08:
		case MONITOR_PACKET_TYP_Command_Write16:
		case MONITOR_PACKET_TYP_Command_Write32:
		case MONITOR_PACKET_TYP_Command_Write64:
		case MONITOR_PACKET_TYP_Command_WriteArea:
		case MONITOR_PACKET_TYP_Command_Call:
		case MONITOR_PACKET_TYP_ACK:
		case MONITOR_PACKET_TYP_Status:
		case MONITOR_PACKET_TYP_Read_Data:
		case MONITOR_PACKET_TYP_Call_Data:
		case MONITOR_PACKET_TYP_Call_Cancel:
		case MONITOR_PACKET_TYP_MagicData:
		case MONITOR_PACKET_TYP_Command_Magic:
			iPacketTypOk = 1;
			break;
	}
	/* Send an error if the command is invalid. */
	if( iPacketTypOk==0 )
	{
		send_status(MONITOR_STATUS_InvalidCommand);

		/* Finished processing the packet. */
		iDone = 1;
	}
	else
	{
		switch(tMonitorHandle.tCommunicationState)
		{
//		case MONITOR_COMMUNICATION_STATE_Disconnected:

		case MONITOR_COMMUNICATION_STATE_Connected:
			/* No command is running, ready to execute a new command. */
			switch(tPacketTyp)
			{
			case MONITOR_PACKET_TYP_Command_Read08:
				command_read(uiPacketSize, MONITOR_ACCESSSIZE_08);
				/* Finished processing the packet. */
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Command_Read16:
				command_read(uiPacketSize, MONITOR_ACCESSSIZE_16);
				/* Finished processing the packet. */
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Command_Read32:
				command_read(uiPacketSize, MONITOR_ACCESSSIZE_32);
				/* Finished processing the packet. */
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Command_Read64:
				command_read(uiPacketSize, MONITOR_ACCESSSIZE_64);
				/* Finished processing the packet. */
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Command_ReadArea:
				command_read_area(uiPacketSize);
				/* Finished processing the packet. */
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Command_Write08:
				command_write(uiPacketSize, MONITOR_ACCESSSIZE_08);
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Command_Write16:
				command_write(uiPacketSize, MONITOR_ACCESSSIZE_16);
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Command_Write32:
				command_write(uiPacketSize, MONITOR_ACCESSSIZE_32);
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Command_Write64:
				command_write(uiPacketSize, MONITOR_ACCESSSIZE_64);
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Command_WriteArea:
				command_write_area(uiPacketSize);
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Command_Call:
				command_call(uiPacketSize);
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_ACK:
				/* Silently ignore spurious ACKs. */

				/* Finished processing the packet. */
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Status:
			case MONITOR_PACKET_TYP_Read_Data:
			case MONITOR_PACKET_TYP_Call_Data:
			case MONITOR_PACKET_TYP_Call_Cancel:
			case MONITOR_PACKET_TYP_MagicData:
			case MONITOR_PACKET_TYP_Command_Magic:
				send_status(MONITOR_STATUS_InvalidCommand);

				/* Finished processing the packet. */
				iDone = 1;
				break;
			}
			break;

		case MONITOR_COMMUNICATION_STATE_CommandResponseWaitForAck:
			/* A "read" or "write" command was executed and a response was sent to the host.
			 * Now the host must respond with an ACK.
			 */
			switch(tPacketTyp)
			{
			case MONITOR_PACKET_TYP_Command_Read08:
			case MONITOR_PACKET_TYP_Command_Read16:
			case MONITOR_PACKET_TYP_Command_Read32:
			case MONITOR_PACKET_TYP_Command_Read64:
			case MONITOR_PACKET_TYP_Command_ReadArea:
			case MONITOR_PACKET_TYP_Command_Write08:
			case MONITOR_PACKET_TYP_Command_Write16:
			case MONITOR_PACKET_TYP_Command_Write32:
			case MONITOR_PACKET_TYP_Command_Write64:
			case MONITOR_PACKET_TYP_Command_WriteArea:
			case MONITOR_PACKET_TYP_Command_Call:
				/* No new commands are accepted until the last command is acknowledged. */
				send_status(MONITOR_STATUS_CommandInProgress);

				/* Finished processing the packet. */
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_ACK:
				/* Go back to the "connected" state. */
				tMonitorHandle.tCommunicationState = MONITOR_COMMUNICATION_STATE_Connected;
				/* Finished processing the packet. */
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Status:
			case MONITOR_PACKET_TYP_Read_Data:
			case MONITOR_PACKET_TYP_Call_Data:
			case MONITOR_PACKET_TYP_Call_Cancel:
			case MONITOR_PACKET_TYP_MagicData:
			case MONITOR_PACKET_TYP_Command_Magic:
				send_status(MONITOR_STATUS_InvalidCommand);

				/* Finished processing the packet. */
				iDone = 1;
				break;
			}
			break;

		case MONITOR_COMMUNICATION_STATE_InCall:
			/* A "call" command was executed and a response was sent to the host.
			 * Now the host must respond with an ACK.
			 */
			switch(tPacketTyp)
			{
			case MONITOR_PACKET_TYP_Command_Read08:
			case MONITOR_PACKET_TYP_Command_Read16:
			case MONITOR_PACKET_TYP_Command_Read32:
			case MONITOR_PACKET_TYP_Command_Read64:
			case MONITOR_PACKET_TYP_Command_ReadArea:
			case MONITOR_PACKET_TYP_Command_Write08:
			case MONITOR_PACKET_TYP_Command_Write16:
			case MONITOR_PACKET_TYP_Command_Write32:
			case MONITOR_PACKET_TYP_Command_Write64:
			case MONITOR_PACKET_TYP_Command_WriteArea:
			case MONITOR_PACKET_TYP_Command_Call:
				/* No new commands are accepted until the last command is acknowledged. */
				send_status(MONITOR_STATUS_CommandInProgress);

				/* Finished processing the packet. */
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_ACK:
				/* Silently ignore spurious ACKs. */

				/* Finished processing the packet. */
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Call_Data:
				/* TODO: add this to the input queue. */

				/* Finished processing the packet. */
				iDone = 1;
				break;

			case MONITOR_PACKET_TYP_Status:
			case MONITOR_PACKET_TYP_Read_Data:
			case MONITOR_PACKET_TYP_Call_Cancel:
			case MONITOR_PACKET_TYP_MagicData:
			case MONITOR_PACKET_TYP_Command_Magic:
				send_status(MONITOR_STATUS_InvalidCommand);

				/* Finished processing the packet. */
				iDone = 1;
				break;
			}
			break;

//		case MONITOR_COMMUNICATION_STATE_CallDataWaitForAck:
		}
	}

	return iDone;
}



static const SERIAL_COMM_UI_FN_T tPapaSchlumpfPluginSerialVectors =
{
	.fn =
	{
		.fnGet = papa_schlumpf_vector_get,
		.fnPut = papa_schlumpf_vector_put,
		.fnPeek = papa_schlumpf_vector_peek,
		.fnFlush = papa_schlumpf_vector_flush
	}
};



void monitor_init(PFN_TRANSPORT_RECEIVE pfnTransportReceive, PFN_TRANSPORT_SEND_PACKET pfnTransportSendPacket, void *pvTransportUserData)
{
	/* Initialize the ringbuffer. */
	INITIALIZE_RINGBUFFER(tRingbufferRx);

	tMonitorHandle.tPacketState = MONITOR_PACKET_STATE_WaitForPacketStart;
	tMonitorHandle.uiPacketSize = 0U;

	tMonitorHandle.pfnTransportReceive = pfnTransportReceive;
	tMonitorHandle.pfnTransportSendPacket = pfnTransportSendPacket;
	tMonitorHandle.pvTransportUserData = pvTransportUserData;

	/* FIXME: start in "disconnected" state. */
	tMonitorHandle.tCommunicationState = MONITOR_COMMUNICATION_STATE_Connected;

	tMonitorHandle.sizTxBuffer = 0U;
	tMonitorHandle.sizCallTxData = 0U;

	/* Initialize the serial vectors. */
	memcpy(&tSerialVectors, &tPapaSchlumpfPluginSerialVectors, sizeof(SERIAL_COMM_UI_FN_T));
}


void monitor_loop(void)
{
	unsigned char ucData;
	unsigned int uiFillLevel;
	unsigned int uiPacketSize;
	unsigned short usCrcMy;
	unsigned short usCrcPacket;
	unsigned int uiCnt;
	int iDone;
	RINGBUFFER_T *ptRingBufferReceive;


	ptRingBufferReceive = &(tRingbufferRx.tRingBuffer);

	uiFillLevel = ringbuffer_get_fill_level(ptRingBufferReceive);
	switch(tMonitorHandle.tPacketState)
	{
	case MONITOR_PACKET_STATE_WaitForPacketStart:
		/* Look for a start character in the available data. */
		if( uiFillLevel>0 )
		{
			while( uiFillLevel!=0 )
			{
				ucData = ringbuffer_get_char(ptRingBufferReceive);
				if( ucData==MONITOR_PACKET_START )
				{
					tMonitorHandle.tPacketState = MONITOR_PACKET_STATE_WaitForSizeComplete;
					break;
				}

				--uiFillLevel;
			}
		}
		else
		{
			/* Get more data. */
			tMonitorHandle.pfnTransportReceive(tMonitorHandle.pvTransportUserData, ptRingBufferReceive);
		}
		break;

	case MONITOR_PACKET_STATE_WaitForSizeComplete:
		/* Wait until enough data for a header is available in the ringbuffer.
		 * The header is "only" the "size" information, which is 2 bytes.
		 */
		if( uiFillLevel>=2 )
		{
			/* Get the size. */
			uiPacketSize = peek_data16(ptRingBufferReceive, 0);

			/* Does the size exceed the allowed limit?
			 * The maximum size of the data part is smaller than the complete packet size as it does not include...
			 *   1 byte start char
			 *   2 bytes size
			 *   2 bytes CRC
			 * This makes a total of 5 bytes.
			 */
			if( uiPacketSize>(MONITOR_MAXIMUM_PACKET_SIZE-5U) )
			{
				/* Ignore packets with invalid data.
				 * Do not skip data here. We might be in the middle of a packet.
				 * Better look for the next packet start.
				 */
				tMonitorHandle.tPacketState = MONITOR_PACKET_STATE_WaitForPacketStart;
			}
			else
			{
				tMonitorHandle.uiPacketSize = uiPacketSize;
				tMonitorHandle.tPacketState = MONITOR_PACKET_STATE_WaitForPacketComplete;
			}
		}
		else
		{
			/* Get more data. */
			tMonitorHandle.pfnTransportReceive(tMonitorHandle.pvTransportUserData, ptRingBufferReceive);
		}
		break;

	case MONITOR_PACKET_STATE_WaitForPacketComplete:
		/* TODO: check for an old "knock" packet.
		 * It has a size of 0 and "*#" as data.
		 */

		/* Wait until data and CRC is available.
		 * The ringbuffer already contains the data size. Now the data
		 * itself and the 2 byte CRC is missing.
		 */
		if( uiFillLevel>=(2+tMonitorHandle.uiPacketSize+2) )
		{
			/* The packet is complete. Now build the CRC.
			 * The CRC includes the size and data.
			 */
			usCrcMy = 0;
			for(uiCnt=0; uiCnt<2+tMonitorHandle.uiPacketSize; ++uiCnt)
			{
				usCrcMy = crc16(usCrcMy, ringbuffer_peek(ptRingBufferReceive, uiCnt));
			}
			usCrcPacket = peek_data16(ptRingBufferReceive, 2+tMonitorHandle.uiPacketSize);
			if( usCrcMy!=usCrcPacket )
			{
				/* Ignore packets with invalid CRC.
				 * Do not skip data here. We might be in the middle of a packet.
				 * Better look for the next packet start.
				 */
				tMonitorHandle.tPacketState = MONITOR_PACKET_STATE_WaitForPacketStart;
			}
			else
			{
				/* Remove the "size" at the start of the packet.
				 * It is already stored in the handle.
				 */
				ringbuffer_skip(ptRingBufferReceive, 2);

				tMonitorHandle.tPacketState = MONITOR_PACKET_STATE_ExecutePacket;
			}
		}
		else
		{
			/* Get more data. */
			tMonitorHandle.pfnTransportReceive(tMonitorHandle.pvTransportUserData, ptRingBufferReceive);
		}
		break;

	case MONITOR_PACKET_STATE_ExecutePacket:
		/* Process the packet. */
		iDone = monitor_process_packet();
		if( iDone!=0 )
		{
			tMonitorHandle.tPacketState = MONITOR_PACKET_STATE_WaitForPacketStart;
		}
	}
}
