#include <stddef.h>
#include <string.h>

#include "mailbox.h"


typedef enum MAILBOX_STATE_ENUM
{
	MAILBOX_STATE_Free = 0,
	MAILBOX_STATE_Full = 1
} MAILBOX_STATE_T;


/* Mailbox information with 0x40 bytes. */
typedef struct MAILBOX_INFORMATION_STRUCT
{
	unsigned char aucMagic[16];
	unsigned long ulVersion;
	unsigned long ulControlRxOffset;
	unsigned long ulControlTxOffset;
	unsigned long ulBufferRxOffset;
	unsigned long ulBufferTxOffset;
	unsigned long ulBufferRxSize;
	unsigned long ulBufferTxSize;
	unsigned long aulReserved[5];
} MAILBOX_INFORMATION_T;


/* Mailbox control with 0x10 bytes. */
typedef struct MAILBOX_CONTROL_STRUCT
{
	volatile unsigned long ulReqCnt;
	volatile unsigned long ulAckCnt;
	volatile unsigned long ulDataSize;
	unsigned long ulReserved0c;
} MAILBOX_CONTROL_T;


/* Complete Mailbox structure with 0x1100 bytes. */
typedef struct MAILBOX_STRUCT
{
	MAILBOX_INFORMATION_T tInformation;
	MAILBOX_CONTROL_T tControlRx;
	MAILBOX_CONTROL_T tControlTx;
	unsigned char aucReserved[0xa0];
	unsigned char aucMailboxRx[2048];
	unsigned char aucMailboxTx[2048];
} MAILBOX_T;

static MAILBOX_T tDpm __attribute__ ((section (".dpm")));


static const unsigned char aucDpmMagic[16] =
{
	0x4d, 0x75, 0x68, 0x6b, 0x75, 0x68, 0x20, 0x44, 0x50, 0x4d, 0x20, 0x44, 0x61, 0x74, 0x61, 0x20
};


static void mailbox_control_init(MAILBOX_CONTROL_T *ptMailbox)
{
	ptMailbox->ulReqCnt = 0;
	ptMailbox->ulAckCnt = 0;
	ptMailbox->ulDataSize = 0;
}


static MAILBOX_STATE_T mailbox_get_state(MAILBOX_CONTROL_T *ptMailbox)
{
	MAILBOX_STATE_T tState = MAILBOX_STATE_Free;


	if( ptMailbox->ulAckCnt!=ptMailbox->ulReqCnt )
	{
		tState = MAILBOX_STATE_Full;
	}
	return tState;
}


void mailbox_init(void)
{
	memset(&tDpm, 0, sizeof(MAILBOX_T));
	memcpy(tDpm.tInformation.aucMagic, aucDpmMagic, sizeof(aucDpmMagic));
	tDpm.tInformation.ulVersion = 0x00010000U;
	tDpm.tInformation.ulControlRxOffset = offsetof(MAILBOX_T, tControlRx);
	tDpm.tInformation.ulControlTxOffset = offsetof(MAILBOX_T, tControlTx);
	tDpm.tInformation.ulBufferRxOffset = offsetof(MAILBOX_T, aucMailboxRx);
	tDpm.tInformation.ulBufferTxOffset = offsetof(MAILBOX_T, aucMailboxTx);
	tDpm.tInformation.ulBufferRxSize = sizeof(tDpm.aucMailboxRx);
	tDpm.tInformation.ulBufferTxSize = sizeof(tDpm.aucMailboxTx);

	mailbox_control_init(&(tDpm.tControlRx));
	mailbox_control_init(&(tDpm.tControlTx));
}


void *mailbox_receive_poll(unsigned long *pulSize)
{
	MAILBOX_STATE_T tState;
	void *pvData;
	unsigned long ulSize;


	pvData = NULL;
	ulSize = 0;
	tState = mailbox_get_state(&(tDpm.tControlRx));
	if( tState==MAILBOX_STATE_Full )
	{
		pvData = tDpm.aucMailboxRx;
		ulSize = tDpm.tControlRx.ulDataSize;
	}

	if( pulSize!=NULL )
	{
		*pulSize = ulSize;
	}
	return pvData;
}


void mailbox_receive_ack(void)
{
	tDpm.tControlRx.ulAckCnt = tDpm.tControlRx.ulReqCnt;
	tDpm.tControlRx.ulDataSize = 0U;
}


MAILBOX_ERROR_T mailbox_send_data(void *pvData, unsigned long ulSize)
{
	MAILBOX_ERROR_T tResult;
	MAILBOX_STATE_T tState;


	/* Does the data fit into the buffer? */
	if( ulSize>sizeof(tDpm.aucMailboxTx) )
	{
		/* The data is too big for the mailbox! */
		tResult = MAILBOX_ERROR_TxDataTooBig;
	}
	else
	{
		tState = mailbox_get_state(&(tDpm.tControlTx));
		if( tState==MAILBOX_STATE_Full )
		{
			/* The mailbox is already full. */
			tResult = MAILBOX_ERROR_TxBusy;
		}
		else
		{
			/* Copy the data into the mailbox. */
			memcpy(tDpm.aucMailboxTx, pvData, ulSize);
			/* Set the size of the data. */
			tDpm.tControlTx.ulDataSize = ulSize;
			/* Set the mailbox state to "full". */
			tDpm.tControlTx.ulReqCnt += 1U;

			tResult = MAILBOX_ERROR_Ok;
		}
	}

	return tResult;
}


void mailbox_send_wait_for_ack(void)
{
	MAILBOX_STATE_T tState;


	do
	{
		tState = mailbox_get_state(&(tDpm.tControlTx));
	} while( tState!=MAILBOX_STATE_Free );
}
