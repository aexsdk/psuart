#ifndef  _H_LOGMESSAGE_H_
#define  _H_LOGMESSAGE_H_
#include	<stdio.h>
#include	<stdlib.h>

#ifndef EZUTILS_API
#define EZUTILS_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*LogMessageT)(int Level,char *msgstr);					//日志回调函数
typedef void (*ReportErrorMessageT)(char *msgstr);		//错误报告回调函数
typedef void (*ReportWarningMessageT)(char *msgstr);	//告警回调函数

#define MAX_MODULE_NUM	32

#define MakeMsgHeader(d,s)	(((d) << 24) | ((s) << 16))	//合成消息的来源和目标信息

#define M_LogLevel(x,m)		(x |(m<<16))

#define LM_FLAG(x)		(1 << x)			//模块的Log标志，x为模块的宏定义
#define LM_SYSTEM		0					//系统消息
#define LM_ERROR		0xFFFF				//错误消息
#define LM_WARNING		0xFFFE				//敬告消息

EZUTILS_API	void InitLogMessage(void);
EZUTILS_API	void DeleteLogMessage(void);
EZUTILS_API	void SetLogCallback(LogMessageT lmt,ReportWarningMessageT rwmt,ReportErrorMessageT remt);


EZUTILS_API	unsigned short GetMaxLogLevel(void);
EZUTILS_API	unsigned short GetMinLogLevel(void);
EZUTILS_API	void SetLogLevel(unsigned short aMinLevel,unsigned short aMaxLevel);
EZUTILS_API	void RemoveLogModule(unsigned short aModule);
EZUTILS_API	void AddLogModule(unsigned short aModule);
EZUTILS_API	unsigned int GetLogModule(void);
EZUTILS_API	void SetLogModule(unsigned int aLogModule);

/*
	****************************************************************************************************
	概述
		显示日志信息的函数
	修改历史
		2003-11-20 杨军 增加了向网络发送日志的功能
	****************************************************************************************************
*/
EZUTILS_API	void LogMessage(int Level,char *file,int line,const char *tag,char *fmt,...);
EZUTILS_API	void LogErrorMessage(char *file,int line,const char *tag,char *fmt,...);
EZUTILS_API	void LogWarningMessage(char *file,int line,const char *tag,char *fmt,...);

/*向指定文件添加日志记录的函数
    fn  : 日志文件名
    flag : 打印位置标识，DEBUG_CONSOLE表示输出到控制台，DEBUG_FILE表示输出到文件
    pszDest : 日志信息
*/
#define DEBUG_CONSOLE   0x10
#define DEBUG_FILE      0x01
EZUTILS_API void _log(char *fn,int flag,char *pszDest);
#ifdef __cplusplus
}
#endif

#endif
