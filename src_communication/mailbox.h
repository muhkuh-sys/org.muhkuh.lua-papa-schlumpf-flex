#ifndef __MAILBOX_H__
#define __MAILBOX_H__


typedef enum MAILBOX_ERROR_ENUM
{
	MAILBOX_ERROR_Ok             = 0,  /* No error. */
	MAILBOX_ERROR_TxDataTooBig   = 1,  /* The provided data does not fit into the mailbox. */
	MAILBOX_ERROR_TxBusy         = 2,  /* Trying to send data while the mailbox is still busy. */
} MAILBOX_ERROR_T;


void mailbox_init(void);

void *mailbox_receive_poll(unsigned long *pulSize);
void mailbox_receive_ack(void);

MAILBOX_ERROR_T mailbox_send_data(void *pvData, unsigned long ulSize);
void mailbox_send_wait_for_ack(void);


#endif  /* __MAILBOX_H__ */
