/*
	****************************************************************************************
	概述
		日志输出的相关函数。
	说明
		要使用LogMessage请先调用一次InitLogMessage，当应用程序不再使用此单元的内容时应调用
	DeleteLogMessage以释放内存。
	创建
		2003-7-10	杨军
	修改历史
		+ 2003-11-26	杨军	增加了对网络日志对本子网采用广播的方式，默认为广播方式
		+ 2003-11-20	杨军	增加了对网络日志的支持，可以向网络指定地址和端口发送日志，使
	用此功能需要增加 LOGTONET 编译开开关。
	****************************************************************************************
*/
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include "utils.h"
#include "xLogMessage.h"

static volatile unsigned int log_signle = 0;
//显示Log的级别
static volatile unsigned short MinLogLevel=0,MaxLogLevel = 255;		//当LogLevel=0表示系统消息,-1（0xFFFF）表示错误,-2(0xFFFE)是警告；一定会输出
static volatile unsigned int LogModule = 0xFFFFFFFF;			//默认只打印系统消息

#define MAX_REV_BUFFER_SIZE 			1024

/*
	*************************************************************************
	概述
		初始化LogMessage的函数。
	说明
		+ LogMessage的显示级别适用了访问互斥，在此
	函数中回初始化互斥变量。
		+ 在使用LogMessage之前一定要调用此函数进行
	初始化。强烈建议在系统初始化的开始调用此函
	数。
	*************************************************************************
*/
EZUTILS_API	void InitLogMessage(void)
{
    log_signle = 0;
    MinLogLevel = 0;
    MaxLogLevel = 255;
    LogModule = 0xFFFFFFFF;
}

/*
	******************************************************************
	概述
		释放内存
	******************************************************************
*/
EZUTILS_API	void DeleteLogMessage(void)
{
    log_signle = 0;
    MinLogLevel = 0;
    MaxLogLevel = 255;
    LogModule = 0xFFFFFFFF;
}

EZUTILS_API int LockLogLevel(int flag)
{
    if(flag){
        while(log_signle > 0){
            //sleep(10);
            fd_set fdR; 
            struct timeval timeout={0,0};
            
            FD_ZERO(&fdR); 
            FD_SET(fileno(stdin), &fdR);
            memset(&timeout,0,sizeof(struct timeval));
            timeout.tv_usec = 1000;
            select(fileno(stdin)+1, &fdR, NULL, NULL, &timeout);
        } 
        log_signle++;
        return TRUE;
    }else{
        if(log_signle == 0){
            log_signle++;
            return TRUE;
        }else{
            return FALSE;
        }
    }
}

EZUTILS_API int UnlockLogLevel(void)
{
    if(log_signle > 0){
        log_signle--;
        return TRUE;
    }else{
        return FALSE;
    }
}

/*
	*************************************************************************
	概述
		取得日志显示级别的函数。
	说明
		此函数是为了封装LogLevel的访问读。
	*************************************************************************
*/
EZUTILS_API	unsigned short GetMaxLogLevel(void)
{
	unsigned short Level;
	LockLogLevel(TRUE);
	Level = MaxLogLevel;
	UnlockLogLevel();
	return Level;
}

/*
	*************************************************************************
	概述
		取得日志显示级别的函数。
	说明
		此函数是为了封装LogLevel的访问读。
	*************************************************************************
*/
unsigned short GetMinLogLevel(void)
{
	unsigned short Level;
	LockLogLevel(TRUE);
	Level = MinLogLevel;
	UnlockLogLevel();
	return Level;
}

/*
	*************************************************************************
	概述
		设置日志显示级别的函数。
	说明
		此函数是为了封装LogLevel的访问写。
		最大最小级别顺序可以互换，命令会根据实际数值调整。
	*************************************************************************
*/
EZUTILS_API	void SetLogLevel(unsigned short aMinLevel,unsigned short aMaxLevel)
{
	LockLogLevel(TRUE);
	MaxLogLevel = aMinLevel > aMaxLevel ? aMinLevel : aMaxLevel;
	MinLogLevel = aMinLevel < aMaxLevel ? aMinLevel : aMaxLevel;
	UnlockLogLevel();
}

/*
	*************************************************************************
	概述
		设置日志显示模块的函数。
	说明
		此函数是为了封装LogModule的访问写。
	*************************************************************************
*/
EZUTILS_API	unsigned int GetLogModule(void)
{
	unsigned int lModule;
	LockLogLevel(TRUE);
	lModule = LogModule;
	UnlockLogLevel();
	return lModule;
}

/*
	*************************************************************************
	概述
		设置日志显示模块的函数。
	说明
		此函数是为了封装LogModule的访问写。
		系统模块是否显示调试信息是通过aLogModule的相应位是否打开来确定的。当1
	左移模块的代码位为1时表示显示信息，否则为不显示。最终调试信息是否会显示还
	要配合最大最小级别来确定。
		+ 0		: 关闭所有模块
		+ -1	: 打开所有模块
	*************************************************************************
*/
EZUTILS_API	void SetLogModule(unsigned int aLogModule)
{
	LockLogLevel(TRUE);
	LogModule = aLogModule;
	UnlockLogLevel();
}

/*
	*************************************************************************
	概述
		增加日志显示模块的函数。
	说明
		此函数是为了简化LogModule的访问写。
	*************************************************************************
*/
EZUTILS_API	void AddLogModule(unsigned short aModule)
{
	LockLogLevel(TRUE);
	LogModule =  LogModule |LM_FLAG(aModule);
	UnlockLogLevel();
}

/*
   ***********************************
   
   
   说明：
   此函数是为了简化LogModule的访问写。
   
   概述：
   移除日志显示模块的函数。 
   
   参数：
   
   	aModule :   模块标识
   ***********************************
*/
EZUTILS_API	void RemoveLogModule(unsigned short aModule)
{
	LockLogLevel(TRUE);
	LogModule =  LogModule &(~ LM_FLAG(aModule));
	UnlockLogLevel();
}

static LogMessageT 				OnLogMessage = NULL;
static ReportErrorMessageT 		OnReportError = NULL;
static ReportWarningMessageT 	OnReportWarning = NULL;
/*
	*************************************************************************************************
	概述
		设置输出日志\错误\警告的回调函数的方法
	*************************************************************************************************
*/
EZUTILS_API	void SetLogCallback(LogMessageT lmt,ReportWarningMessageT rwmt,ReportErrorMessageT remt)
{
	OnLogMessage = lmt;
	OnReportWarning = rwmt;
	OnReportError = remt;
}

/*向指定文件添加日志记录的函数
    fn  : 日志文件名
    flag : 打印位置标识，DEBUG_CONSOLE表示输出到控制台，DEBUG_FILE表示输出到文件
    pszDest : 日志信息
*/
EZUTILS_API void _log(char *fn,int flag,char *pszDest)
{
	if((flag&DEBUG_CONSOLE) == DEBUG_CONSOLE){
		//为了便于测试同时输出信息到控制台
		printf("\t%s\n",pszDest);
	}
	if((flag&DEBUG_FILE) == DEBUG_FILE && fn != NULL)
	{
		FILE *log_fd = NULL;
		time_t timer; 
		struct tm *tblock; 

		timer=time(NULL); 
		tblock = localtime(&timer); 
		log_fd = fopen(fn,"a+");
		if(log_fd != NULL){
			fprintf(log_fd,"%.4d-%.2d-%.2d %.2d:%.2d:%.2d\t%s\n",
				tblock->tm_year+1900,
				tblock->tm_mon+1,
				tblock->tm_mday,
				tblock->tm_hour,
				tblock->tm_min,
				tblock->tm_sec,
				pszDest);
			fclose(log_fd);
		}else{
			//printf("Open %s error:%s.\n",fn,strerror(errno));
		}
	}
}

/*
	****************************************************************************************************
	概述
		显示日志信息的函数
	修改历史
		2003-11-20 杨军 增加了向网络发送日志的功能
	****************************************************************************************************
*/
EZUTILS_API	void LogMessage(int Level,char *file,int line,const char *tag,char *fmt,...)
{
	unsigned int lModule,lLevel;
	int ltype;
	if(!fmt)return;
	lModule= (Level & 0xFFFF0000)>>16;
	lLevel = (Level & 0x0000FFFF);
	ltype = (lLevel == LM_SYSTEM )		||		//检查是否为系统消息
			(lLevel == LM_ERROR) 		||		//检查是否为错误消息
			(lLevel == LM_WARNING);				//检查是否为警告消息
	if ((!ltype) && (lLevel > GetMaxLogLevel() || lLevel < GetMinLogLevel())) return;	//大于LogLevel的信息将放弃不显示
	if ((1<<lModule) & GetLogModule())			//检查Log的模块是否需要LOG
	{
		char pszDest[2048],*p = pszDest;
		int hlen=0;
		va_list args;

		memset(pszDest,0,sizeof(pszDest));
		if(0){
			sprintf(pszDest,"Line %d in %s\nHINT:[%s]",line,file,tag);
			hlen = strlen(pszDest);
			p = pszDest + hlen;
		}
		va_start(args, fmt);
		vsnprintf(p, sizeof(pszDest)-hlen, fmt, args);
		va_end(args);
		if(OnLogMessage)	OnLogMessage(Level,pszDest);
	}
}

/*
	****************************************************************************************************
	概述
		显示错误信息的函数
	****************************************************************************************************
*/
EZUTILS_API	void LogErrorMessage(char *file,int line,const char *tag,char *fmt,...)
{
    char pszDest[2048],*p;
    int hlen=0;
    va_list args;

    memset(pszDest,0,sizeof(pszDest));
    sprintf(pszDest,"Line %d in %s\nERROR:[%s]",line,file,tag);
    hlen = strlen(pszDest);
    p = pszDest + hlen;
    va_start(args, fmt);
    vsnprintf(p, sizeof(pszDest)-hlen, fmt, args);
    va_end(args);
	if(OnReportError)	OnReportError(pszDest);
}

/*
	****************************************************************************************************
	概述
		显示警告信息的函数
	****************************************************************************************************
*/
EZUTILS_API	void LogWarningMessage(char *file,int line,const char *tag,char *fmt,...)
{
    char pszDest[2048],*p;
    int hlen=0;
    va_list args;

    memset(pszDest,0,sizeof(pszDest));
    sprintf(pszDest,"Line %d in %s\nWARNING:[%s]",line,file,tag);
    hlen = strlen(pszDest);
    p = pszDest + hlen;
    va_start(args, fmt);
    vsnprintf(p, sizeof(pszDest)-hlen, fmt, args);
    va_end(args);
	if(OnReportWarning)	OnReportWarning(pszDest);
}

