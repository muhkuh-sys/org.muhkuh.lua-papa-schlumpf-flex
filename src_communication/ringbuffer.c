#include "ringbuffer.h"

#include <string.h>


unsigned int ringbuffer_write(RINGBUFFER_T *ptRingBuffer, const unsigned char *pucBuffer, unsigned int sizBuffer)
{
	unsigned int uiTotal;
	unsigned int uiFree;
	unsigned int uiSizeCanWrite;
	unsigned int uiWriteOffset;
	unsigned int uiChunk;
	unsigned int uiLeftToBufferEnd;
	unsigned char *pucDst;
	const unsigned char *pucSrcCnt;
	const unsigned char *pucSrcEnd;


	/* Get the size of the ring buffer. */
	uiTotal = ptRingBuffer->sizTotal;

	/* Get the number of free bytes. */
	uiFree = uiTotal - ptRingBuffer->sizFill;

	/* Get the current write offset. */
	uiWriteOffset = ptRingBuffer->sizWriteOffset;

	/* Get the number of bytes which can be written to the ring buffer
	 * with this operation. This is limited by...
	 *   1) the number of bytes passed to this function.
	 *   2) the number of free bytes left in the ring buffer.
	 */
	uiSizeCanWrite = sizBuffer;
	if( uiSizeCanWrite>uiFree )
	{
		uiSizeCanWrite = uiFree;
	}

	pucSrcCnt = pucBuffer;
	pucSrcEnd = pucBuffer + uiSizeCanWrite;
	while( pucSrcCnt<pucSrcEnd )
	{
		/* How many bytes can be written in one chunk?
		 * Limits are...
		 * 1) The number of bytes to write.
		 * 2) The wrap-around in the buffer.
		 */
		uiChunk = uiSizeCanWrite;
		uiLeftToBufferEnd = uiTotal - uiWriteOffset;
		if( uiChunk>uiLeftToBufferEnd )
		{
			uiChunk = uiLeftToBufferEnd;
		}

		/* Copy the bytes to the ring buffer. */
		pucDst = ptRingBuffer->aucBuffer + uiWriteOffset;
		memcpy(pucDst, pucSrcCnt, uiChunk);
		pucSrcCnt += uiChunk;

		/* These bytes are in use now. */
		uiFree -= uiChunk;
		uiSizeCanWrite -= uiChunk;

		/* Add the number of written bytes to the write offset and
		 * check for a wrap around at the end of the buffer.
		 */
		uiWriteOffset += uiChunk;
		if( uiWriteOffset>=uiTotal )
		{
			uiWriteOffset -= uiTotal;
		}
	}

	/* Update the number of free bytes. */
	ptRingBuffer->sizFill = uiTotal - uiFree;

	/* Update the write offset. */
	ptRingBuffer->sizWriteOffset = uiWriteOffset;

	return uiSizeCanWrite;
}



unsigned int ringbuffer_write_char(RINGBUFFER_T *ptRingBuffer, unsigned char ucData)
{
	unsigned int uiTotal;
	unsigned int uiFree;
	unsigned int uiWriteOffset;
	unsigned int uiBytesWritten;


	/* Get the size of the ring buffer. */
	uiTotal = ptRingBuffer->sizTotal;

	/* Get the number of free bytes. */
	uiFree = uiTotal - ptRingBuffer->sizFill;

	/* Get the current write offset. */
	uiWriteOffset = ptRingBuffer->sizWriteOffset;

	uiBytesWritten = 0U;
	if( uiFree>0U )
	{
		/* Copy the data to the ring buffer. */
		ptRingBuffer->aucBuffer[uiWriteOffset] = ucData;

		/* These bytes are in use now. */
		--uiFree;

		/* Add the number of written bytes to the write offset and
		 * check for a wrap around at the end of the buffer.
		 */
		++uiWriteOffset;
		if( uiWriteOffset>=uiTotal )
		{
			uiWriteOffset -= uiTotal;
		}

		uiBytesWritten = 1U;
	}

	/* Update the number of free bytes. */
	ptRingBuffer->sizFill = uiTotal - uiFree;

	/* Update the write offset. */
	ptRingBuffer->sizWriteOffset = uiWriteOffset;

	return uiBytesWritten;
}



unsigned int ringbuffer_get_fill_level(RINGBUFFER_T *ptRingBuffer)
{
	return ptRingBuffer->sizFill;
}



unsigned int ringbuffer_get_free(RINGBUFFER_T *ptRingBuffer)
{
	return ptRingBuffer->sizTotal - ptRingBuffer->sizFill;
}



unsigned int ringbuffer_read(RINGBUFFER_T *ptRingBuffer, unsigned char *pucBuffer, unsigned int sizBuffer)
{
	unsigned int uiTotal;
	unsigned int uiFill;
	unsigned int uiSizeCanRead;
	unsigned int uiReadOffset;
	unsigned int uiChunk;
	unsigned int uiLeftToBufferEnd;
	const unsigned char *pucSrc;
	unsigned char *pucDstCnt;
	unsigned char *pucDstEnd;


	/* Get the size of the ring buffer. */
	uiTotal = ptRingBuffer->sizTotal;

	/* Get the number of valid bytes. */
	uiFill = ptRingBuffer->sizFill;

	/* Get the current write offset. */
	uiReadOffset = ptRingBuffer->sizReadOffset;

	/* Get the number of bytes which can be read from the ring buffer
	 * with this operation. This is limited by...
	 *   1) the number of bytes passed to this function.
	 *   2) the number of free bytes left in the ring buffer.
	 */
	uiSizeCanRead = sizBuffer;
	if( uiSizeCanRead>uiFill )
	{
		uiSizeCanRead = uiFill;
	}

	pucDstCnt = pucBuffer;
	pucDstEnd = pucBuffer + uiSizeCanRead;
	while( pucDstCnt<pucDstEnd )
	{
		/* How many bytes can be read in one chunk?
		 * Limits are...
		 * 1) The number of bytes to write.
		 * 2) The wrap-around in the buffer.
		 */
		uiChunk = uiSizeCanRead;
		uiLeftToBufferEnd = uiTotal - uiReadOffset;
		if( uiChunk>uiLeftToBufferEnd )
		{
			uiChunk = uiLeftToBufferEnd;
		}

		/* Copy the bytes from the ring buffer. */
		pucSrc = ptRingBuffer->aucBuffer + uiReadOffset;
		memcpy(pucDstCnt, pucSrc, uiChunk);
		pucDstCnt += uiChunk;

		/* These bytes are free now. */
		uiFill -= uiChunk;
		uiSizeCanRead -= uiChunk;

		/* Add the number of processed bytes to the read offset and
		 * check for a wrap around at the end of the buffer.
		 */
		uiReadOffset += uiChunk;
		if( uiReadOffset>=uiTotal )
		{
			uiReadOffset -= uiTotal;
		}
	}

	/* Update the number of free bytes. */
	ptRingBuffer->sizFill = uiFill;

	/* Update the read offset. */
	ptRingBuffer->sizReadOffset = uiReadOffset;

	return uiSizeCanRead;
}



unsigned int ringbuffer_get_continuous_block(RINGBUFFER_T *ptRingBuffer, unsigned char **pucBuffer, unsigned int sizBuffer)
{
	unsigned int uiTotal;
	unsigned int uiFill;
	unsigned int uiSizeCanRead;
	unsigned int uiReadOffset;
	unsigned int uiChunk;
	unsigned int uiLeftToBufferEnd;


	/* Get the size of the ring buffer. */
	uiTotal = ptRingBuffer->sizTotal;

	/* Get the number of valid bytes. */
	uiFill = ptRingBuffer->sizFill;

	/* Get the current write offset. */
	uiReadOffset = ptRingBuffer->sizReadOffset;

	/* Get the number of bytes which can be read from the ring buffer
	 * with this operation. This is limited by...
	 *   1) the number of bytes passed to this function.
	 *   2) the number of free bytes left in the ring buffer.
	 */
	uiSizeCanRead = sizBuffer;
	if( uiSizeCanRead>uiFill )
	{
		uiSizeCanRead = uiFill;
	}

	/* How many bytes can be read in one chunk?
	 * Limits are...
	 * 1) The number of bytes to write.
	 * 2) The wrap-around in the buffer.
	 */
	uiChunk = uiSizeCanRead;
	uiLeftToBufferEnd = uiTotal - uiReadOffset;
	if( uiChunk>uiLeftToBufferEnd )
	{
		uiChunk = uiLeftToBufferEnd;
	}

	/* Get a pointer to the start of the continuous data block. */
	*pucBuffer = ptRingBuffer->aucBuffer + uiReadOffset;

	/* These bytes are free now. */
	uiFill -= uiChunk;

	/* Add the number of processed bytes to the read offset and
	 * check for a wrap around at the end of the buffer.
	 */
	uiReadOffset += uiChunk;
	if( uiReadOffset>=uiTotal )
	{
		uiReadOffset -= uiTotal;
	}

	/* Update the number of free bytes. */
	ptRingBuffer->sizFill = uiFill;

	/* Update the read offset. */
	ptRingBuffer->sizReadOffset = uiReadOffset;

	return uiChunk;
}



unsigned char ringbuffer_get_char(RINGBUFFER_T *ptRingBuffer)
{
	unsigned int sizTotal;
	unsigned int sizFill;
	unsigned int sizReadOffset;
	unsigned char ucByte;


	/* Get the size of the ring buffer. */
	sizTotal = ptRingBuffer->sizTotal;

	ucByte = 0;
	sizFill = ptRingBuffer->sizFill;
	if( sizFill!=0 )
	{
		sizReadOffset = ptRingBuffer->sizReadOffset;
		ucByte = ptRingBuffer->aucBuffer[sizReadOffset];

		++sizReadOffset;
		if( sizReadOffset>=sizTotal )
		{
			sizReadOffset -= sizTotal;
		}
		ptRingBuffer->sizReadOffset = sizReadOffset;

		--sizFill;
		ptRingBuffer->sizFill = sizFill;
	}

	return ucByte;
}




unsigned char ringbuffer_peek(RINGBUFFER_T *ptRingBuffer, unsigned int sizOffset)
{
	unsigned int sizTotal;
	unsigned int sizReadOffset;
	unsigned char ucByte;


	/* Get the size of the ring buffer. */
	sizTotal = ptRingBuffer->sizTotal;

	sizReadOffset = ptRingBuffer->sizReadOffset + sizOffset;
	if( sizReadOffset>=sizTotal )
	{
		sizReadOffset -= sizTotal;
	}

	ucByte = ptRingBuffer->aucBuffer[sizReadOffset];

	return ucByte;
}



void ringbuffer_skip(RINGBUFFER_T *ptRingBuffer, unsigned int sizSkip)
{
	unsigned int sizTotal;
	unsigned int sizReadOffset;


	/* Get the size of the ring buffer. */
	sizTotal = ptRingBuffer->sizTotal;

	sizReadOffset = ptRingBuffer->sizReadOffset;
	sizReadOffset += sizSkip;
	if( sizReadOffset>=sizTotal )
	{
		sizReadOffset -= sizTotal;
	}
	ptRingBuffer->sizReadOffset = sizReadOffset;

	ptRingBuffer->sizFill -= sizSkip;
}
