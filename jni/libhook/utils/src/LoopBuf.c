#include <string.h>

#include "../include/LoopBuf.h"




int InitLoopBuf(loop_buf *buf, unsigned int len) {
	if ( !buf || len <= 0)
		return 1;

	buf->buff_len = len;
	buf->buffer = (char*) malloc(buf->buff_len);
	if ( !buf->buffer )
		return 1;
	memset(buf->buffer, 0, buf->buff_len);
	buf->in = buf->out = 0;
	pthread_mutex_init(&buf->mutex,NULL);

	return 0;
}

void UninitLoopBuf(loop_buf *buf) {
	if (buf) {
		pthread_mutex_destroy(&buf->mutex);
		if (buf->buffer)
			free(buf->buffer);
	}
}

unsigned int PutData(loop_buf *buf, const char *buffer, unsigned int len) {
	if (!buf)
		return 0;

	unsigned int l;

	len = MIN(len, buf->buff_len - buf->in + buf->out);

	// first put the data starting from buf->in to buffer end
	l = MIN(len, buf->buff_len - (buf->in & (buf->buff_len - 1)));
	memcpy(buf->buffer + (buf->in & (buf->buff_len - 1)), buffer, l);

	// then put the rest (if any) at the beginning of the buffer
	memcpy(buf->buffer, buffer + l, len - l);

	buf->in += len;

	return len;
}

unsigned int GetData(loop_buf *buf, char *buffer, unsigned int len) {
	if (!buf)
		return 0;

	unsigned int l;

	len = MIN(len, buf->in - buf->out);

	// first get the data from buf->out until the end of the buffer
	l = MIN(len, buf->buff_len - (buf->out & (buf->buff_len - 1)));
	memcpy(buffer, buf->buffer + (buf->out & (buf->buff_len - 1)), l);

	// then get the rest (if any) from the beginning of the buffer
	memcpy(buffer + l, buf->buffer, len - l);

	buf->out += len;

	if (buf->out == buf->in)
		buf->out = buf->in = 0;

	return len;
}

unsigned int DiscardSomeData(loop_buf *buf, unsigned int len) {
	if (!buf)
		return 0;

	unsigned int l;

	len = MIN(len, buf->in - buf->out);

	// first get the data from buf->out until the end of the buffer
	l = MIN(len, buf->buff_len - (buf->out & (buf->buff_len - 1)));

	// then get the rest (if any) from the beginning of the buffer

	buf->out += len;

	if (buf->out == buf->in)
		buf->out = buf->in = 0;

	return len;
}

void ResetLoopBuf(loop_buf *buf) {
	if (!buf)
		return;

	buf->in = buf->out = 0;
}

unsigned int GetBufLen(loop_buf *buf) {
	if (!buf)
		return 0;

	return buf->buff_len;
}

unsigned int GetDataLen(loop_buf *buf) {
	if (!buf)
		return 0;

	return buf->in - buf->out;
}

unsigned int GetIdleBufLen(loop_buf *buf) {
	if (!buf)
		return 0;


	return buf->buff_len - buf->in + buf->out;
}


void LockBuf(loop_buf *buf)
{
	if (!buf)
		return;
	
	 pthread_mutex_lock(&buf->mutex);
}

void UnlockBuf(loop_buf *buf)
{
	if (!buf)
		return;
	
	pthread_mutex_unlock(&buf->mutex);
}

