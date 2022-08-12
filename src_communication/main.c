#include <string.h>

#include "mailbox.h"
#include "monitor.h"
#include "netx_io_areas.h"
#include "rdy_run.h"
#include "ringbuffer.h"
#include "systime.h"


static void transport_dpm_receive(void *pvUser __attribute__((unused)), RINGBUFFER_T *ptRingBuffer)
{
	void *pvData;
	unsigned int uiSize;


	/* Is something in the mailbox? */
	pvData = mailbox_receive_poll(&uiSize);
	if( pvData!=NULL )
	{
		/* Copy the data into the ringbuffer.
			* If not all data fits into the buffer, discard it.
			*/
		ringbuffer_write(ptRingBuffer, pvData, uiSize);

		/* Acknowledge the data. */
		mailbox_receive_ack();
	}
}


static int transport_dpm_send(void *pvUser __attribute__((unused)), void *pvData, unsigned int sizData)
{
	int iResult;
	MAILBOX_ERROR_T tResult;

	tResult = mailbox_send_data(pvData, sizData);
	if( tResult!=MAILBOX_ERROR_Ok )
	{
		iResult = -1;
	}
	else
	{
		mailbox_send_wait_for_ack();
		iResult = 0;
	}

	return iResult;
}


void communication_main(void);
void communication_main(void)
{
	BLINKI_HANDLE_T tBlinki;


	systime_init();

	/* Initialize the mailbox. */
	mailbox_init();

	/* Initialize the monitor. */
	monitor_init(transport_dpm_receive, transport_dpm_send, NULL);

	/* FIXME: This is just an "I am alive" blinking.
	 *        Remove this later as the test routines need full control
	 *        over the SYS LED.
	 */
	/* Slow green blinking   *__GG   */
	rdy_run_blinki_init(&tBlinki, 0x03, 0x13);
	while(1)
	{
		/* Process packet data. */
		monitor_loop();

		rdy_run_blinki(&tBlinki);
	}
}
