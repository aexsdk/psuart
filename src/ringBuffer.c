/*
 *
 */
#include <stdio.h>      /*标准输入输出定义*/
#include <stdlib.h>     /*标准函数库定义*/
#include <string.h>

#include <ringbuffer.h>
#define EINVAL 22
#define ENOMEM 12

/*
 * internal helper to calculate the unused elements in a fifo
 */
static rbUint ringbuffer_unused(struct ringbuffer *fifo)
{
	return (fifo->mask + 1) - (fifo->in - fifo->out);
}

static void ringbuffer_copy_in(struct ringbuffer *fifo, const char *src,rbUint len, rbUint off)
{
	rbUint slen;

	slen = min(len, (fifo->mask + 1) - off);
	
	if(slen > 0)
		memcpy(fifo->rdata + off, src, slen);
	if(len - slen > 0)
		memcpy(fifo->rdata,src + slen, len - slen);
}

static void ringbuffer_copy_out(struct ringbuffer *fifo, char *dst,
		rbUint len, rbUint off)
{
	rbUint slen;

	slen = min(len, (fifo->mask + 1) - off);

	if(slen>0)
		memcpy(dst, fifo->rdata + off, slen);
	if(len - slen > 0){
		memcpy(dst + slen, fifo->rdata, len - slen);
	}
}

/**
 * ringbuffer_reset - removes the entire fifo content
 * @fifo: address of the fifo to be used
 *
 * Note: usage of ringbuffer_reset() is dangerous. It should be only called when the
 * fifo is exclusived locked or when it is secured that no other thread is
 * accessing the fifo.
 */
void ringbuffer_reset(struct ringbuffer *fifo) 
{
	fifo->in = 0;
	fifo->out = 0; 
}

/**
 * ringbuffer_len - returns the number of used elements in the fifo
 * @fifo: address of the fifo to be used
 */
rbUint ringbuffer_len(struct ringbuffer *fifo) 
{ 
	rbUint lenth;
	
	if(fifo->in >= fifo->out)
		return fifo->in - fifo->out;
	else
	{	
		lenth =(rbUint)((fifo->in)+(fifo->mask)+1-(fifo->out));
		return lenth;
		
	}
}

/**
 * ringbuffer_skip - skip output data
 * @fifo: address of the fifo to be used
 */
void ringbuffer_skip(struct ringbuffer *fifo)
{
	if(ringbuffer_len(fifo)>0)fifo->out++;
}

/**
 * ringbuffer_peek_len - gets the slen of the next fifo record
 * @fifo: address of the fifo to be used
 *
 * This function returns the slen of the next fifo record in number of bytes.
 */
rbUint ringbuffer_peek_len(struct ringbuffer *fifo)
{
	return ringbuffer_len(fifo);
}

/**
 * ringbuffer_init - initialize a fifo using a preallocated buffer
 * @fifo: the fifo to assign the buffer
 * @buffer: the preallocated buffer to be used
 * @slen: the slen of the internal buffer, this have to be a power of 2
 *
 * This macro initialize a fifo using a preallocated buffer.
 *
 * The numer of elements will be rounded-up to a power of 2.
 * Return 0 if no error, otherwise an error code.
 */
rbUint ringbuffer_init(struct ringbuffer *fifo, char *buffer, size_t slen) 
{
	fifo->in = 0;
	fifo->out = 0;
	fifo->rdata = buffer;

	if (slen < 2) {
		fifo->mask = 0;
		return -EINVAL;
	}
	fifo->mask = slen - 1;

	return 0;
}

/**
 * ringbuffer_in - put data into the fifo
 * @fifo: address of the fifo to be used
 * @buf: the data to be added
 * @n: number of elements to be added
 *
 * This macro copies the given buffer into the fifo and returns the
 * number of copied elements.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these macro.
 */
rbUint ringbuffer_in(struct ringbuffer *fifo,char *buf,size_t len)
{
	rbUint l;

	l = ringbuffer_unused(fifo);
	if (len > l)
		len = l;

	ringbuffer_copy_in(fifo, buf, len, fifo->in);
	fifo->in += len;
	if(fifo->in > fifo->mask)
		fifo->in -= fifo->mask+1;
	return ringbuffer_len(fifo);//len;
}

rbUint ringbuffer_in_byte(struct ringbuffer *fifo,unsigned char dbyte)
{
	rbUint l;

	l = ringbuffer_unused(fifo);
	if (l > 0){
		char *p;
		rbUint slen = fifo->mask + 1;

		if(min(1, slen - fifo->in) > 0){
			p = fifo->rdata + fifo->in;
		}else{
			p = fifo->rdata                              ;
		}
		*p = dbyte;
	}
	fifo->in += 1;
	if(fifo->in > fifo->mask)
	{
		fifo->in -= ((fifo->mask)+1);
	}
	return ringbuffer_len(fifo);//len;
}
/**
 * ringbuffer_out - get data from the fifo
 * @fifo: address of the fifo to be used
 * @buf: pointer to the storage buffer
 * @n: max. number of elements to get
 *
 * This macro get some data from the fifo and return the numbers of elements
 * copied.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these macro.
 */
rbUint ringbuffer_out(struct ringbuffer *fifo,char *buf,size_t len)
{
	len = ringbuffer_out_peek(fifo, buf, len);
	fifo->out += len;
	if(fifo->out > fifo->mask)
		fifo->out -= fifo->mask+1;
	return len;
}

/**
 * ringbuffer_out_peek - gets some data from the fifo
 * @fifo: address of the fifo to be used
 * @buf: pointer to the storage buffer
 * @n: max. number of elements to get
 *
 * This macro get the data from the fifo and return the numbers of elements
 * copied. The data is not removed from the fifo.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these macro.
 */
rbUint ringbuffer_out_peek(struct ringbuffer *fifo,char *buf,size_t len)
{
	rbUint l;

	l = ringbuffer_len(fifo);
	if (len > l)
		len = l;

	ringbuffer_copy_out(fifo, buf, len, fifo->out);
	return len;
}

unsigned char ringbuffer_out_byte(struct ringbuffer *fifo)
{
	unsigned char r = ringbuffer_peek_byte(fifo);
	fifo->out++;
	if(fifo->out > fifo->mask)
		fifo->out -= fifo->mask+1;
	return r;
}

unsigned char ringbuffer_peek_byte(struct ringbuffer *fifo)
{
	unsigned char r=0;
	rbUint off = fifo->out;
	rbUint slen = fifo->mask + 1;
	char *p;
	
	if(min(1, slen - fifo->out) > 0){
		p = fifo->rdata + fifo->out;
	}else{
		p = fifo->rdata;
	}
	r = *p;
	return r;
}
