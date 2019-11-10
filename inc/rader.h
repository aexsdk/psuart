#ifndef __RADER_H__
#define __RADER_H__

#include <stdlib.h>

#define		ERUP_BEGIN		0xFB
#define		ERUP_END		0xFE

//获得起始标志
#define		ERUP_GET_BEGIN(buf)			(buf[0])
//获得荷载数据的长度
#define		ERUP_GET_LEN(buf)			(buf[1]<<8|buf[2])
#define		ERUP_GET_FLAG(buf)			(buf+3)
//获得数据的起始地址
#define		ERUP_GET_DATA(buf)			(buf+4)
//获得信令CRC值
#define		ERUP_GET_CRC(buf)			(buf[ERUP_GET_LEN(buf)+5])
//获得结束标志
#define		ERUP_GET_END(buf)			(buf[ERUP_GET_LEN(buf)+6])
//获得信令参数的长度
#define		ERUP_GET_PACKET_LEN(buf)	(ERUP_GET_LEN(buf)+6)		

#define		ERUP_SET_BEGIN(buf)			{buf[0]=ERUP_BEGIN;}
#define		ERUP_SET_LEN(buf,val)		{buf[1]=(val>>8)&0xFF;buf[2]=val&0xFF;}			//设置荷载数据的长度
#define		ERUP_SET_FLAG(buf,val)		{buf[3]=val&0xFF;}			
#define		ERUP_SET_CRC(buf)			{buf[ERUP_GET_LEN(buf)+4]=crc8(ERUP_GET_DATA(buf),ERUP_GET_LEN(buf));}		//获得数据的CRC值
#define		ERUP_SET_END(buf)			{buf[ERUP_GET_LEN(buf)+5]=ERUP_END;}


//define support data flag. 
#define DATA_FLAG_F		0x01
#define DATA_FLAG_f		0x02
#define DATA_FLAG_m		0x10
#define DATA_FLAG_M		0x11
#define DATA_FLAG_p		0x20
#define DATA_FLAG_S     0x21

unsigned char crc8(unsigned char *buffer, unsigned int len);
int rader_init();
void rader_cmd(int fd,unsigned char *buf,size_t len);
unsigned char rader_recv(int fd,char *data,size_t len);
void rader_cmd_from_stdin(int fd,char *buf,size_t len);

#endif //__RADER_H__