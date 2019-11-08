/*
 */

#ifndef _LINUX_RINGBUFFER_H
#define _LINUX_RINGBUFFER_H

#include <stdio.h>      /*标准输入输出定义*/
#include <stdlib.h>     /*标准函数库定义*/

typedef unsigned int rbUint;

typedef struct ringbuffer {
	rbUint	in;
	rbUint	out;
	rbUint	mask;
	char *rdata;
}RingBuffer,*PRingBuffer;

#define min(x,y)		(x>y?y:x)	

/**
 * ringbuffer_initialized - Check if the fifo is initialized
 * @fifo: address of the fifo to check
 *
 * Return %true if fifo is initialized, otherwise %false.
 * Assumes the fifo was 0 before.
 */
#define ringbuffer_initialized(fifo) ((fifo)->mask)

/**
 * ringbuffer_size - returns the slen of the fifo in elements
 * @fifo: address of the fifo to be used
 */
#define ringbuffer_size(fifo)	((fifo)->mask + 1)

rbUint ringbuffer_out_peek(struct ringbuffer *fifo,char *buf,size_t len);
unsigned char ringbuffer_peek_byte(struct ringbuffer *fifo);
rbUint ringbuffer_out(struct ringbuffer *fifo,char *buf,size_t len);
rbUint ringbuffer_in(struct ringbuffer *fifo,char *buf,size_t len);
rbUint ringbuffer_in_byte(struct ringbuffer *fifo,unsigned char dbyte);
rbUint ringbuffer_init(struct ringbuffer *fifo, char *buffer, size_t slen);
rbUint ringbuffer_peek_len(struct ringbuffer *fifo);
//rbUint ringbuffer_avail(struct ringbuffer *fifo);
//rbUint ringbuffer_is_full(struct ringbuffer *fifo);
rbUint ringbuffer_len(struct ringbuffer *fifo);
void ringbuffer_reset_out(struct ringbuffer *fifo);
void ringbuffer_reset(struct ringbuffer *fifo);
void ringbuffer_skip(struct ringbuffer *fifo);

#endif
