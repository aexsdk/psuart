/*
	****************************************************************************************
	����
		��־�������غ�����
	˵��
		Ҫʹ��LogMessage���ȵ���һ��InitLogMessage����Ӧ�ó�����ʹ�ô˵�Ԫ������ʱӦ����
	DeleteLogMessage���ͷ��ڴ档
	����
		2003-7-10	���
	�޸���ʷ
		+ 2003-11-26	���	�����˶�������־�Ա��������ù㲥�ķ�ʽ��Ĭ��Ϊ�㲥��ʽ
		+ 2003-11-20	���	�����˶�������־��֧�֣�����������ָ����ַ�Ͷ˿ڷ�����־��ʹ
	�ô˹�����Ҫ���� LOGTONET ���뿪���ء�
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
//��ʾLog�ļ���
static volatile unsigned short MinLogLevel=0,MaxLogLevel = 255;		//��LogLevel=0��ʾϵͳ��Ϣ,-1��0xFFFF����ʾ����,-2(0xFFFE)�Ǿ��棻һ�������
static volatile unsigned int LogModule = 0xFFFFFFFF;			//Ĭ��ֻ��ӡϵͳ��Ϣ

#define MAX_REV_BUFFER_SIZE 			1024

/*
	*************************************************************************
	����
		��ʼ��LogMessage�ĺ�����
	˵��
		+ LogMessage����ʾ���������˷��ʻ��⣬�ڴ�
	�����лس�ʼ�����������
		+ ��ʹ��LogMessage֮ǰһ��Ҫ���ô˺�������
	��ʼ����ǿ�ҽ�����ϵͳ��ʼ���Ŀ�ʼ���ô˺�
	����
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
	����
		�ͷ��ڴ�
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
	����
		ȡ����־��ʾ����ĺ�����
	˵��
		�˺�����Ϊ�˷�װLogLevel�ķ��ʶ���
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
	����
		ȡ����־��ʾ����ĺ�����
	˵��
		�˺�����Ϊ�˷�װLogLevel�ķ��ʶ���
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
	����
		������־��ʾ����ĺ�����
	˵��
		�˺�����Ϊ�˷�װLogLevel�ķ���д��
		�����С����˳����Ի�������������ʵ����ֵ������
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
	����
		������־��ʾģ��ĺ�����
	˵��
		�˺�����Ϊ�˷�װLogModule�ķ���д��
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
	����
		������־��ʾģ��ĺ�����
	˵��
		�˺�����Ϊ�˷�װLogModule�ķ���д��
		ϵͳģ���Ƿ���ʾ������Ϣ��ͨ��aLogModule����Ӧλ�Ƿ����ȷ���ġ���1
	����ģ��Ĵ���λΪ1ʱ��ʾ��ʾ��Ϣ������Ϊ����ʾ�����յ�����Ϣ�Ƿ����ʾ��
	Ҫ��������С������ȷ����
		+ 0		: �ر�����ģ��
		+ -1	: ������ģ��
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
	����
		������־��ʾģ��ĺ�����
	˵��
		�˺�����Ϊ�˼�LogModule�ķ���д��
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
   
   
   ˵����
   �˺�����Ϊ�˼�LogModule�ķ���д��
   
   ������
   �Ƴ���־��ʾģ��ĺ����� 
   
   ������
   
   	aModule :   ģ���ʶ
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
	����
		���������־\����\����Ļص������ķ���
	*************************************************************************************************
*/
EZUTILS_API	void SetLogCallback(LogMessageT lmt,ReportWarningMessageT rwmt,ReportErrorMessageT remt)
{
	OnLogMessage = lmt;
	OnReportWarning = rwmt;
	OnReportError = remt;
}

/*��ָ���ļ������־��¼�ĺ���
    fn  : ��־�ļ���
    flag : ��ӡλ�ñ�ʶ��DEBUG_CONSOLE��ʾ���������̨��DEBUG_FILE��ʾ������ļ�
    pszDest : ��־��Ϣ
*/
EZUTILS_API void _log(char *fn,int flag,char *pszDest)
{
	if((flag&DEBUG_CONSOLE) == DEBUG_CONSOLE){
		//Ϊ�˱��ڲ���ͬʱ�����Ϣ������̨
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
	����
		��ʾ��־��Ϣ�ĺ���
	�޸���ʷ
		2003-11-20 ��� �����������緢����־�Ĺ���
	****************************************************************************************************
*/
EZUTILS_API	void LogMessage(int Level,char *file,int line,const char *tag,char *fmt,...)
{
	unsigned int lModule,lLevel;
	int ltype;
	if(!fmt)return;
	lModule= (Level & 0xFFFF0000)>>16;
	lLevel = (Level & 0x0000FFFF);
	ltype = (lLevel == LM_SYSTEM )		||		//����Ƿ�Ϊϵͳ��Ϣ
			(lLevel == LM_ERROR) 		||		//����Ƿ�Ϊ������Ϣ
			(lLevel == LM_WARNING);				//����Ƿ�Ϊ������Ϣ
	if ((!ltype) && (lLevel > GetMaxLogLevel() || lLevel < GetMinLogLevel())) return;	//����LogLevel����Ϣ����������ʾ
	if ((1<<lModule) & GetLogModule())			//���Log��ģ���Ƿ���ҪLOG
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
	����
		��ʾ������Ϣ�ĺ���
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
	����
		��ʾ������Ϣ�ĺ���
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

