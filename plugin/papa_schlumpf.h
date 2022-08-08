#include <stddef.h>
#include <stdint.h>


#ifndef SWIG
#include <libusb.h>
#endif


#ifndef __PAPA_SCHLUMPF_H__
#define __PAPA_SCHLUMPF_H__


typedef unsigned long * PUL_ARGUMENT_OUT;
typedef char ** PPC_ARGUMENT_OUT;
typedef int RESULT_INT_TRUE_OR_NIL_WITH_ERR;
typedef int RESULT_INT_NOTHING_OR_NIL_WITH_ERR;
typedef int RESULT_INT_INT_OR_NIL_WITH_ERR;


typedef enum PAPA_SCHLUMPF_RESULT_ENUM
{
	PAPA_SCHLUMPF_RESULT_Ok = 0,
	PAPA_SCHLUMPF_RESULT_NotConnected = -1,
	PAPA_SCHLUMPF_RESULT_USBError = -2,
	PAPA_SCHLUMPF_RESULT_MoreThanOneDeviceFound = -3,
	PAPA_SCHLUMPF_RESULT_NoDeviceFound = -4,
	PAPA_SCHLUMPF_RESULT_CommandFailed = -5,
	PAPA_SCHLUMPF_RESULT_InvalidSize = -6,
	PAPA_SCHLUMPF_RESULT_OutOfMemory = -7
} PAPA_SCHLUMPF_RESULT_T;


class PapaSchlumpfFlex
{
public:
	PapaSchlumpfFlex(void);
	~PapaSchlumpfFlex(void);

	RESULT_INT_TRUE_OR_NIL_WITH_ERR connect(void);
	RESULT_INT_NOTHING_OR_NIL_WITH_ERR getFirmwareVersion(PUL_ARGUMENT_OUT pulVersionMajor, PUL_ARGUMENT_OUT pulVersionMinor, PUL_ARGUMENT_OUT pulVersionSub, PPC_ARGUMENT_OUT ppcVcsVersion);
	RESULT_INT_TRUE_OR_NIL_WITH_ERR resetPCI(uint32_t ulResetActiveToClock, uint32_t ulResetActiveDelayAfterClock, uint32_t ulBusIdleDelay);
	RESULT_INT_NOTHING_OR_NIL_WITH_ERR ioRead(uint32_t ulAddress, PUL_ARGUMENT_OUT pulData);
	RESULT_INT_NOTHING_OR_NIL_WITH_ERR memRead(uint32_t ulAddress, PUL_ARGUMENT_OUT pulData);
	RESULT_INT_NOTHING_OR_NIL_WITH_ERR memReadArea(uint32_t ulAddress, uint32_t ulSize, char **ppcBUFFER_OUT, size_t *psizBUFFER_OUT);
	RESULT_INT_NOTHING_OR_NIL_WITH_ERR cfg0Read(uint32_t ulAddress, PUL_ARGUMENT_OUT pulData);
	RESULT_INT_NOTHING_OR_NIL_WITH_ERR cfg1Read(uint32_t ulAddress, PUL_ARGUMENT_OUT pulData);
	RESULT_INT_TRUE_OR_NIL_WITH_ERR ioWrite(uint32_t ulAddress, uint32_t ulData);
	RESULT_INT_TRUE_OR_NIL_WITH_ERR memWrite(uint32_t ulAddress, uint32_t ulData);
	RESULT_INT_TRUE_OR_NIL_WITH_ERR memWriteArea(uint32_t ulAddress, const char *pcBUFFER_IN, size_t sizBUFFER_IN);
	RESULT_INT_TRUE_OR_NIL_WITH_ERR cfg0Write(uint32_t ulAddress, uint32_t ulData);
	RESULT_INT_TRUE_OR_NIL_WITH_ERR cfg1Write(uint32_t ulAddress, uint32_t ulData);
	RESULT_INT_TRUE_OR_NIL_WITH_ERR disconnect(void);

	const char *get_error_string(int iResult);

/* Do not wrap the private members. */
#ifndef SWIG
private:
	PAPA_SCHLUMPF_RESULT_T __scan_for_papa_schlumpf_hardware(void);
	int __send_packet(const unsigned char *pucOutBuf, int sizOutBuf, unsigned int uiTimeoutMs);
	int __receivePacket(unsigned char *pucInBuf, int sizInBufMax, int *psizInBuf, unsigned int uiTimeoutMs);
	void __disconnect(void);

	typedef struct ERRORMESSAGE_STRUCT
	{
		PAPA_SCHLUMPF_RESULT_T tResult;
		const char *pcMessage;
	} ERRORMESSAGE_T;
	static const ERRORMESSAGE_T atErrorMessages[];

	/* This is the context of the libusb. */
	libusb_context *m_ptLibUsbContext;

	/* This is the handle for the papa_schlumpf device. */
	libusb_device_handle *m_ptDevHandlePapaSchlumpf;

	/* This is the name of the plugin. It is used for error messages on the screen. */
	char *m_pcPluginId;
#endif
};


#endif  /* __PAPA_SCHLUMPF_H__ */
