/**
*   @mainpage PCI Express Test
*
*   Description of the PCI Express Test project on netX side.
*
*   @author Alessandra Lang
*
*   Copyright 2016 Hilscher Gesellschaft fuer Systemautomation mbH.
*
*   @file main.c
*
*   @brief Main file
*
*   Contains usb monitor function.
*
*/

#include "main.h"

#include <string.h>

#include "netx_io_areas.h"
#include "pci.h"
#include "rdy_run.h"
#include "serial_vectors.h"
#include "systime.h"
#include "uprintf_buffer.h"
#include "usb_emsys/usb.h"


/*-----------------------------------*/


static unsigned char io_dummy_get(void)
{
	return 0;
}


static unsigned int io_dummy_peek(void)
{
	return 0;
}


static void io_dummy_flush(void)
{
}


const SERIAL_COMM_UI_FN_T tSerialVectors_USB =
{
	.fn =
	{
		.fnGet = io_dummy_get,
		.fnPut = uprintf_buffer_putchar,
		.fnPeek = io_dummy_peek,
		.fnFlush = io_dummy_flush
	}
};

SERIAL_COMM_UI_FN_T tSerialVectors;


void test_main(void)
{
	BLINKI_HANDLE_T tBlinki;
	HOSTDEF(ptPioArea);
	HOSTDEF(ptGpioArea);
//	TEST_RESULT_T tResult;


	systime_init();
	uprintf_buffer_init();

	/* Set the serial vectors. */
	memcpy(&tSerialVectors, &tSerialVectors_USB, sizeof(SERIAL_COMM_UI_FN_T));

	/* Set PIOs 0-3 to their default state */
	ptPioArea->ulPio_oe &= ~(15U);

	/* Initialize some ugly global variables. */
	pciInitGlobals();
	/* Enable the PCI mode. */
	pciModeEnable();

	/* Deactivate the USB pull resistor. */
	ptGpioArea->aulGpio_cfg[12] = 0;
	/* Delay a while to allow the PC to recognize the device disappeared. */
	systime_delay_ms(200);
	/* Activate the USB pull resistor. */
	ptGpioArea->aulGpio_cfg[12] = 5;

#if 0
	tResult = test();
	if( tResult==TEST_RESULT_OK )
	{
		/* Fast green blinking   *_G   */
		rdy_run_blinki_init(&tBlinki, 1, 5);
		while(1)
		{
			rdy_run_blinki(&tBlinki);
		}
	}
	else
	{
		rdy_run_setLEDs(RDYRUN_YELLOW);
	}

	while(1) {};
#endif

	/* Fast green blinking   *_G   */
	rdy_run_blinki_init(&tBlinki, 1, 5);
	while(1)
	{
		usb_loop();
		rdy_run_blinki(&tBlinki);
	}
}


/*-----------------------------------*/
