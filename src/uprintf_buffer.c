/**
 *
 *
 */

#include "uprintf_buffer.h"

#include <string.h>


#define UPRINTF_BUFFER_MAX 32768
static union BUFFER_UNION
{
    char ac[UPRINTF_BUFFER_MAX];
    unsigned char auc[UPRINTF_BUFFER_MAX];
} uBuffer;
static unsigned long ulBufferFill;



void uprintf_buffer_init(void)
{
    /* The buffer is empty. */
    ulBufferFill = 0;

    /* Clear the complete buffer with 0.
     * This helps to find the end with the debugger.
     */
    memset(uBuffer.ac, 0, UPRINTF_BUFFER_MAX);
}



void uprintf_buffer_putchar(unsigned int uiChar)
{
    char cChar;


    /* Limit the char to the usual ASCII range of 0..127 and cast it to a char. */
    cChar = (char)(uiChar&0x7f);

    /* Is the buffer full? */
    if( ulBufferFill>=UPRINTF_BUFFER_MAX )
    {
        /* Flush the buffer. */
        uprintf_buffer_flush();
    }

    /* Put the char in the buffer. */
    uBuffer.ac[ulBufferFill] = cChar;
    /* The buffer contains now one more char. */
    ++ulBufferFill;
}



void uprintf_buffer_flush(void)
{
    /* Only flush if there are chars in the buffer. */
    if( ulBufferFill!=0 )
    {
        /* Send the complete buffer. */
//        usb_send_packet(uBuffer.auc, ulBufferFill);
        /* Now the buffer is empty. */
        ulBufferFill = 0;
    }
}



void uprintf_buffer_putrawchar(unsigned char ucChar)
{
    /* Is the buffer full? */
    if( ulBufferFill>=UPRINTF_BUFFER_MAX )
    {
        /* Flush the buffer. */
        uprintf_buffer_flush();
    }

    /* Put the char in the buffer. */
    uBuffer.auc[ulBufferFill] = ucChar;
    /* The buffer contains now one more char. */
    ++ulBufferFill;
}

