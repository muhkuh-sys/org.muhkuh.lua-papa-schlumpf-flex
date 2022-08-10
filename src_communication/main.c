#include <string.h>

#include "mailbox.h"
#include "netx_io_areas.h"
#include "rdy_run.h"
#include "systime.h"


void communication_main(void);
void communication_main(void)
{
	BLINKI_HANDLE_T tBlinki;
	unsigned long ulPacketCnt;
	void *pvData;
	unsigned long ulSize;
	MAILBOX_ERROR_T tResult;
	unsigned long aulDemoPacket[3];


	systime_init();

	/* Initialize the DPM. */
	mailbox_init();

	/* FIXME: This is just an "I am alive" blinking.
	 *        Remove this later as the test routines need full control
	 *        over the SYS LED.
	 */
	/* Slow green blinking   *__GG   */
	rdy_run_blinki_init(&tBlinki, 0x03, 0x13);
	ulPacketCnt = 0;
	while(1)
	{
		/* Is something in the mailbox? */
		pvData = mailbox_receive_poll(&ulSize);
		if( pvData!=NULL )
		{
			/* TODO: do something with the data. */
			/* Acknowledge the data. */
			mailbox_receive_ack();

			/* One more packet was received. */
			ulPacketCnt += 1;

			/* Send a response packet. */
			aulDemoPacket[0] = ulPacketCnt;
			aulDemoPacket[1] = (unsigned long)pvData;
			aulDemoPacket[2] = ulSize;
			tResult = mailbox_send_data(aulDemoPacket, sizeof(aulDemoPacket));
			if( tResult!=MAILBOX_ERROR_Ok )
			{
				break;
			}
			mailbox_send_wait_for_ack();
		}

		rdy_run_blinki(&tBlinki);
	}

	/* Stop in error state. */
	rdy_run_setLEDs(RDYRUN_YELLOW);
	while(1) {};
}
