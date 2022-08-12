#ifndef __MONITOR_COMMANDS_H__
#define __MONITOR_COMMANDS_H__


#define MONITOR_VERSION_MAJOR 4
#define MONITOR_VERSION_MINOR 0

#define MONITOR_PACKET_START 0x2a

#define MONITOR_MAXIMUM_PACKET_SIZE 1024

typedef enum MONITOR_PACKET_TYP
{
	MONITOR_PACKET_TYP_Command_Read08        = 0x00,
	MONITOR_PACKET_TYP_Command_Read16        = 0x01,
	MONITOR_PACKET_TYP_Command_Read32        = 0x02,
	MONITOR_PACKET_TYP_Command_Read64        = 0x03,
	MONITOR_PACKET_TYP_Command_ReadArea      = 0x04,
	MONITOR_PACKET_TYP_Command_Write08       = 0x05,
	MONITOR_PACKET_TYP_Command_Write16       = 0x06,
	MONITOR_PACKET_TYP_Command_Write32       = 0x07,
	MONITOR_PACKET_TYP_Command_Write64       = 0x08,
	MONITOR_PACKET_TYP_Command_WriteArea     = 0x09,
	MONITOR_PACKET_TYP_Command_Execute       = 0x0a,
	MONITOR_PACKET_TYP_ACK                   = 0x0b,
	MONITOR_PACKET_TYP_Status                = 0x0c,
	MONITOR_PACKET_TYP_Read_Data             = 0x0d,
	MONITOR_PACKET_TYP_Call_Data             = 0x0e,
	MONITOR_PACKET_TYP_Call_Cancel           = 0x0f,
	MONITOR_PACKET_TYP_MagicData             = 0x4d,
	MONITOR_PACKET_TYP_Command_Magic         = 0xff
} MONITOR_PACKET_TYP_T;


typedef enum
{
	MONITOR_STATUS_Ok                        = 0x00,
	MONITOR_STATUS_Call_Finished             = 0x01,
	MONITOR_STATUS_InvalidCommand            = 0x02,
	MONITOR_STATUS_InvalidPacketSize         = 0x03,
	MONITOR_STATUS_InvalidSizeParameter      = 0x04,
	MONITOR_STATUS_CommandInProgress         = 0x05
} MONITOR_STATUS_T;


#endif  /* __MONITOR_COMMANDS_H__ */
