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
static unsigned short raderDlen = 0;

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
	if(cmd[0] == 'S') return DATA_FLAG_S;
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
	case DATA_FLAG_S:
		return 10;
	default :
		return 0;
	}
}


unsigned char rader_recv(int fd,char *data,size_t len)
{
	char buf[4096];
	
	if(raderDlen > 0 && ringbuffer_in(&raderFifo,data,len) >= raderDlen)
	{
		memset(buf,0,sizeof(buf));
		ERUP_SET_BEGIN(buf);
		ERUP_SET_LEN(buf,raderDlen);
		ERUP_SET_FLAG(buf,rdaerCmdFlag);
		ringbuffer_out(&raderFifo,ERUP_GET_DATA(buf),raderDlen);
		ERUP_SET_CRC(buf);
		ERUP_SET_END(buf);
		write(fd,buf,ERUP_GET_PACKET_LEN(buf));
		
		if(rdaerCmdFlag == DATA_FLAG_S)
		{
			rdaerCmdFlag = 0;
		    raderDlen = 0;  
		    return DATA_FLAG_S;
		}

		rdaerCmdFlag = 0;
		raderDlen = 0;   
	}
	
	return 0;
}


int rader_heart(int fd,char *buf,char *data,size_t len)
{
	ERUP_SET_BEGIN(buf);
	ERUP_SET_LEN(buf,len);
	ERUP_SET_FLAG(buf,0);
	memcpy(ERUP_GET_DATA(buf),data,len);
	ERUP_SET_CRC(buf);
	ERUP_SET_END(buf);
	return ERUP_GET_PACKET_LEN(buf);
}

unsigned char rader_cmd(int fd,unsigned char *buf,size_t len)
{
	unsigned char *p = buf;
	unsigned char chBuf[30];
	
	memset(chBuf,0,sizeof(chBuf));

	printf("test %02X\n",ERUP_GET_BEGIN(p));
	
	while((len>0) && (ERUP_GET_BEGIN(p) != ERUP_BEGIN)){
		p++;
		len--;
		//printf("test len %d-%02X\n",len,ERUP_GET_BEGIN(p));

	}
	if(len < ERUP_GET_PACKET_LEN(p))
	{
	    //printf("test %d\n",len);
	    return 0;		//need continue recv data
	}

    printf("crc test %02X,%02X\n",ERUP_GET_CRC(p),crc8(ERUP_GET_DATA(p),ERUP_GET_LEN(p)));
	
	if(ERUP_GET_END(p) != ERUP_END || ERUP_GET_CRC(p) != crc8(ERUP_GET_DATA(p),ERUP_GET_LEN(p))){
	    printf("cmd packnet error.\n");
		return 0; //packnet error 
	}
	
	if(raderDlen == 0)
	{
        ringbuffer_reset(&raderFifo);
        rdaerCmdFlag = rader_flag(ERUP_GET_DATA(p),ERUP_GET_LEN(p));
        raderDlen = rader_dlen(rdaerCmdFlag);
        //printf("command %02X-%d\n",rdaerCmdFlag,raderDlen);
        write(fd,ERUP_GET_DATA(p),ERUP_GET_LEN(p));
    }
    else
    {
        printf("receive data is not finish \n");
    }
    
    if(ERUP_GET_LEN(p) <= 30)
    {
        memcpy(chBuf,ERUP_GET_DATA(p),ERUP_GET_LEN(p));
        
        if(strcasecmp(chBuf,"close socket!") == 0)
        {
            printf("receive close socket command!");
            return 1;
        }
    }
	
	return 0;
}


void rader_cmd_from_stdin(int fd,char *buf,size_t len)
{
	char *p = buf;
	
	if(len >= 3)
	{
	    if(strcasecmp(buf,"cls") == 0)
	    {
	    	rdaerCmdFlag = 0;
		    raderDlen = 0;
	        return;
	    }
	    if(buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X'))
	    {
	        p[0] = (char)(StrToInt(buf) && 0xFF);
	        len = 1;
	    }
	}
	
	if(raderDlen == 0)
	{
        ringbuffer_reset(&raderFifo);
        rdaerCmdFlag = rader_flag(p,len);
        raderDlen = rader_dlen(rdaerCmdFlag);
        write(fd,p,len);
    }
    else
    {
        printf("receive data is not finish from stdin \n");
    }
}

void set_raderDlen(unsigned short value)
{
    raderDlen = value;
}
