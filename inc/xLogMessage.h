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

typedef void (*LogMessageT)(int Level,char *msgstr);					//��־�ص�����
typedef void (*ReportErrorMessageT)(char *msgstr);		//���󱨸�ص�����
typedef void (*ReportWarningMessageT)(char *msgstr);	//�澯�ص�����

#define MAX_MODULE_NUM	32

#define MakeMsgHeader(d,s)	(((d) << 24) | ((s) << 16))	//�ϳ���Ϣ����Դ��Ŀ����Ϣ

#define M_LogLevel(x,m)		(x |(m<<16))

#define LM_FLAG(x)		(1 << x)			//ģ���Log��־��xΪģ��ĺ궨��
#define LM_SYSTEM		0					//ϵͳ��Ϣ
#define LM_ERROR		0xFFFF				//������Ϣ
#define LM_WARNING		0xFFFE				//������Ϣ

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
	����
		��ʾ��־��Ϣ�ĺ���
	�޸���ʷ
		2003-11-20 ��� �����������緢����־�Ĺ���
	****************************************************************************************************
*/
EZUTILS_API	void LogMessage(int Level,char *file,int line,const char *tag,char *fmt,...);
EZUTILS_API	void LogErrorMessage(char *file,int line,const char *tag,char *fmt,...);
EZUTILS_API	void LogWarningMessage(char *file,int line,const char *tag,char *fmt,...);

/*��ָ���ļ������־��¼�ĺ���
    fn  : ��־�ļ���
    flag : ��ӡλ�ñ�ʶ��DEBUG_CONSOLE��ʾ���������̨��DEBUG_FILE��ʾ������ļ�
    pszDest : ��־��Ϣ
*/
#define DEBUG_CONSOLE   0x10
#define DEBUG_FILE      0x01
EZUTILS_API void _log(char *fn,int flag,char *pszDest);
#ifdef __cplusplus
}
#endif

#endif
