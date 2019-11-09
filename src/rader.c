#include <string.h>
#include "rader.h"
#include "ringBuffer.h"
#include "utils.h"

unsigned char crc8(unsigned char *buffer, unsigned int len)
{
	unsigned char crc;
	unsigned char i;
	
	crc = 0;
	while(len--)
	{
		crc ^= *buffer++;
		for(i = 0;i < 8;i++)
		{
			if(crc & 0x01)
			{
				crc = (crc >> 1) ^ 0x8c;
			}else{
				crc >>= 1;
			}
		}
	}
	return crc; 
}

static char raderBuf[4096*2];
static struct ringbuffer raderFifo;
static unsigned char rdaerCmdFlag = 0;

int rader_init()
{
	ringbuffer_init(&raderFifo,raderBuf,sizeof(raderBuf));
	return 0;
}

unsigned char rader_flag(char *cmd,int len)
{
	if(len < 1)return 0;
	if(cmd[0] == 'f') return DATA_FLAG_f;
	if(cmd[0] == 'F') return DATA_FLAG_F;
	if(cmd[0] == 'p') return DATA_FLAG_p;
	if(cmd[0] == 'm') return DATA_FLAG_m;
	if(cmd[0] == 'M') return DATA_FLAG_M;
	return 0;
}

unsigned short rader_dlen(unsigned char flag)
{
	switch(flag)
	{
	case DATA_FLAG_f:
		return 3172;
	case DATA_FLAG_F:
		return 3178;
	case DATA_FLAG_p:
		return 1024;
	case DATA_FLAG_m:
		return 2144;
	case DATA_FLAG_M:
		return 64;
	default :
		return 0;
	}
}


void rader_recv(int fd,char *data,size_t len)
{
	char buf[4096];
	unsigned short dlen = rader_dlen(rdaerCmdFlag);
	if(dlen > 0 && ringbuffer_in(&raderFifo,data,len) >= dlen)
	{
		memset(buf,0,sizeof(buf));
		ERUP_SET_BEGIN(buf);
		ERUP_SET_LEN(buf,len);
		ERUP_SET_FLAG(buf,rdaerCmdFlag);
		ringbuffer_out(&raderFifo,ERUP_GET_DATA(buf),dlen);
		ERUP_SET_CRC(buf);
		ERUP_SET_END(buf);
		write(fd,buf,ERUP_GET_PACKET_LEN(buf));
	}
}

void rader_cmd(int fd,char *buf,size_t len)
{
	char *p = buf;
	
	while(len>0 && ERUP_GET_BEGIN(buf) != ERUP_BEGIN){
		p++;
		len--;
	}
	if(len < ERUP_GET_PACKET_LEN(buf))return;		//need continue recv data
	if(ERUP_GET_END(buf) != ERUP_END || ERUP_GET_CRC(buf) != crc8(ERUP_GET_DATA(buf),ERUP_GET_LEN(buf))){
		return ; //packnet error 
	}
	ringbuffer_reset(&raderFifo);
	rdaerCmdFlag = rader_flag(ERUP_GET_DATA(buf),ERUP_GET_LEN(buf));
	write(fd,ERUP_GET_DATA(buf),ERUP_GET_LEN(buf));
}


void rader_cmd_from_stdin(int fd,char *buf,size_t len)
{
	char *p = buf;
	
	ringbuffer_reset(&raderFifo);
	rdaerCmdFlag = rader_flag(buf,len);
	write(fd,buf,len);
}
