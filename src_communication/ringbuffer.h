#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__



typedef struct RINGBUFFER_STRUCT
{
	unsigned int sizTotal;
	unsigned int sizFill;
	unsigned int sizReadOffset;
	unsigned int sizWriteOffset;
	unsigned char aucBuffer[4];
} RINGBUFFER_T;

#define DEFINE_RINGBUFFER(NAME, SIZE) struct { RINGBUFFER_T tRingBuffer; unsigned char aucRestOfBuffer[SIZE-4]; } NAME
#define INITIALIZE_RINGBUFFER(TRINGBUFFER) { TRINGBUFFER.tRingBuffer.sizTotal=sizeof(TRINGBUFFER.aucRestOfBuffer)+4U; TRINGBUFFER.tRingBuffer.sizFill=0; TRINGBUFFER.tRingBuffer.sizReadOffset=0; TRINGBUFFER.tRingBuffer.sizWriteOffset=0; }


unsigned int ringbuffer_write(RINGBUFFER_T *ptRingBuffer, const unsigned char *pucBuffer, unsigned int sizBuffer);
unsigned int ringbuffer_write_char(RINGBUFFER_T *ptRingBuffer, unsigned char ucData);
unsigned int ringbuffer_get_fill_level(RINGBUFFER_T *ptRingBuffer);
unsigned int ringbuffer_get_free(RINGBUFFER_T *ptRingBuffer);
unsigned int ringbuffer_read(RINGBUFFER_T *ptRingBuffer, unsigned char *pucBuffer, unsigned int sizBuffer);
unsigned int ringbuffer_get_continuous_block(RINGBUFFER_T *ptRingBuffer, unsigned char **pucBuffer, unsigned int sizBuffer);
unsigned char ringbuffer_get_char(RINGBUFFER_T *ptRingbuffer);
unsigned char ringbuffer_peek(RINGBUFFER_T *ptRingBuffer, unsigned int sizOffset);
void ringbuffer_skip(RINGBUFFER_T *ptRingBuffer, unsigned int sizSkip);


#endif  /* __RINGBUFFER_H__ */
