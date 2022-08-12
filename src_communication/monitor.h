#include "ringbuffer.h"


#ifndef __MONITOR_H__
#define __MONITOR_H__


typedef void (*PFN_TRANSPORT_RECEIVE)(void *pvUser, RINGBUFFER_T *ptRingBuffer);
typedef int (*PFN_TRANSPORT_SEND_PACKET)(void *pvUser, void *pvData, unsigned int sizData);


void monitor_init(PFN_TRANSPORT_RECEIVE pfnTransportReceive, PFN_TRANSPORT_SEND_PACKET pfnTransportSendPacket, void *pvTransportUserData);
void monitor_loop(void);


#endif  /* __MONITOR_H__ */
