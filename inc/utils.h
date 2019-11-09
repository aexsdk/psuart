#ifndef __APP_UTILS__
#define __APP_UTILS__
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EZUTILS_API
#define EZUTILS_API
#endif

#define PATHSPLIT       '/'
#define PATHSPLITS      "/"

#define TRUE			1
#define FALSE			0
#define MAX_ARG_LEN		512		/*this define max argument length*/

int HexEncodeGetRequiredLength(int nSrcLen);
int HexDecodeGetRequiredLength(int nSrcLen);
int HexEncode(const unsigned char *pbSrcData, int nSrcLen, char *szDest, int *pnDestLen);
char GetHexValue(char ch);
int HexDecode(const char *pSrcData, int nSrcLen, unsigned char *pbDest, int* pnDestLen);
int utils_strincmp(const char *s1,const char *s2,int n);
unsigned char utils_crc(const unsigned char *buf,int size);
int  Setblocking(int  fd);
/**
 * 将参数字符串分割为数组
 */
int split_arguments(const char *arginfo,char argArray[][MAX_ARG_LEN],int maxsize,char spliter);
unsigned int get_ip_addr(char *ipaddr);
unsigned int ParseIPAddr(char *cAddr,unsigned short *port);
EZUTILS_API	char * LeftString(char *Dest,char *aSource,char BreakChar);
EZUTILS_API	int RemovePrefix(char *Result,char *Source,char *DecPrefix);
EZUTILS_API	char * Addprefix(char *Result,char *Source,char *AddPrefix);
EZUTILS_API	char *Copy(char *dstr,const char *sstr,int from,int count);
EZUTILS_API	char *Trim(char *dstr,const char *str);
EZUTILS_API	void SplitStringValue(char *str,char *buf);
EZUTILS_API	void SplitStringKeyValue(char *str,char *key,char * value);
EZUTILS_API	long GetIPAddressByName(char *aDomain);
EZUTILS_API	char *IP2Str(unsigned long aIP);
EZUTILS_API	unsigned long Str2IP(char *aIP);
EZUTILS_API	int IsPublicIP(unsigned long ip);
EZUTILS_API	int	IsPrivateIP(unsigned long ip);
EZUTILS_API	int IsValidIP(unsigned long ip);
EZUTILS_API	int IsMulticastIP(unsigned long ip);
EZUTILS_API	char * ExtractFilePath(const char *FileName,char *path);
EZUTILS_API	char * ExtractFileName(const char *FileName,char *fn);
EZUTILS_API	char * ChangeFileExt(char *src,char *ext);
EZUTILS_API	char * ChangeFilePath(char * src,char *path);

int GetTickCount(void);
int StrToInt(char * str);
int GetCmdParamValue(int argc, char* argv[],char *param,char* paramValue);
int CheckCmdLine(int argc, char* argv[],char *sw);
int ParseCmdParamValue(const char *cmd,const char *param,char *value);

int  SetNonblocking(int  fd);
int get_ip(char *localip);
void DisplaySocketInfo(int sockfd);

int max_value(int a,...);
int min_value(int a,...);
int sum_value(int a,...);

/*
 * 当发现一个可用串口时的回调函数，这个回调函数讲用于each_comm函数
 * @param param 上下文相关参数
 * @param com 发现的串口设备字符串
 * */
typedef void (*ON_FIND_COMM)(void *param,char *com);

/**
 * 打开串口的函数
 * @param Dev  串口字符串，字符串格式为:	com=/dev/ttyUSB0(串口设备字符串),s=9600(波特率),p=N(奇偶校验),b=1(停止位),d=8(数据位数)
 * @return
 * 		返回串口句柄，如果失败则返回值<=0
 */
int com_open(char *dev,int flag);
void com_close(int fd);
/**
 * 从串口接收信息的函数
 * @param fd  串口句柄
 * @param buf 存放接收内容的缓冲区
 * @param maxLen 存放接收内容缓冲区的大小
 */
int com_recive(int fd,char *buf,int maxLen,int timeout);
int com_write(int fd,char *buf,int len);
/**
 * 枚举所有可用串口的函数,标准C中的文件结构中文件名是13个字符。
 * @param filter 查找串口的过滤字符串，如"/dev/ttyUSB*"表示查找所有以ttyUSB开头的设备
 * @param  ofc  回调函数
 * @param param 调用回调函数时提供的上下文相关的参数
 */
void each_comm(char *filter,ON_FIND_COMM ofc,void *param);

#ifdef __cplusplus
}
#endif

#endif
