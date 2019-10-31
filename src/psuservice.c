/* Parse a configuration file into a struct using X-Macros */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "psuservice.h"
static psus_config psus_data;               //从配置文件中读取的服务端配置数据


/* create one and fill in its default values */
psus_config Psus_Config_Default = {
    0x11,
    0,
    9,
    10,
    {},
    -1,
    -1,
    #define CFG(s, n, default) default,
    #include "psuservice.def"
};

/* process a line of the INI file, storing valid values into config struct */
static int handler(void *user, const char *section, const char *name,const char *value)
{
    psus_config *cfg = (psus_config *)user;

    if (0) ;
    #define CFG(s, n, default) else if (strcmp(section, #s)==0 && \
        strcmp(name, #n)==0) cfg->s##_##n = strdup(value);
    #include "psuservice.def"

    return 1;
}

/* print all the variables in the config, one per line */
static void dump_config(psus_config *cfg)
{
    #define CFG(s, n, default) psus_log(10,"%s_%s = %s", #s, #n, cfg->s##_##n == NULL?"":cfg->s##_##n);
    #include "psuservice.def"
}

/* clear all the variables in the config, one per line */
static void clear_config(psus_config *cfg)
{
	#define CFG(s, n, default)	if(cfg->s##_##n != NULL){	\
									free(cfg->s##_##n);		\
								cfg->s##_##n = NULL;	\
							}
	#include "psuservice.def"
}

/* clear all the variables in the config, one per line */
static void save_config(psus_config *cfg,char *file)
{
	//printf("%s = %d\n",__func__,__LINE__);
	#define CFG(s, n, default)	write_profile_string(#s,#n,cfg->s##_##n,file);
	#include "psuservice.def"
}

static void default_config(psus_config *cfg,psus_config *defcfg)
{
	//printf("%s = %d\n",__func__,__LINE__);
	#define CFG(s, n, default)	if(cfg->s##_##n != NULL)	\
									free(cfg->s##_##n);		\
								if(defcfg->s##_##n != NULL)	\
									cfg->s##_##n = strdup(defcfg->s##_##n);
	#include "psuservice.def"
}


/*
	设置启动和关闭输出到文件的标识。
*/
void _set_debug_file(int v)
{
	if(v == 0){
		psus_data.flag &= ~DEBUG_FILE;
	}else{
		psus_data.flag |= DEBUG_FILE;
	}
}

/*
	最终输出日志的函数，输出位置使用psu_data的flag变量的设置。可以选择是输出到控制台还是文件。
*/
static void _log_message(int Level,char *msgstr)
{
    _log(psus_data.log_file,psus_data.flag,msgstr);
}

/*
	最终输出错误的函数，错误信息会同时输出到控制台和文件。
*/
static void _log_error(char *msgstr)
{
    _log(psus_data.log_file,0x11,msgstr);
}

/*
	最终输出告警的函数，告警信息会同时输出到控制台和文件。
*/
static void _log_warning(char *msgstr)
{
    _log(psus_data.log_file,0x11,msgstr);
}

//外部获取平台库配置的数据结构的方法
psus_config *get_psus_data(void)
{
    return &psus_data;
}

/**
    从配置文件读取配置的函数，当管理命令需要执行载入配置命令时可以使用此函数
*/
static int LoadPsuConfig(char *fn)
{
	int r = TRUE;
	psus_config *psudata = get_psus_data();
	int ret = 1;
	unsigned short port;

	SetLogCallback(_log_message,_log_warning,_log_error);
    if (ini_parse(fn, handler, &psus_data) < 0){
        psus_error("Can't load '%s', using defaults",fn);
		default_config(psudata,&Psus_Config_Default);
		save_config(psudata,fn);
        r = FALSE;
    }
    dump_config(&psus_data);

	psudata->flag = StrToInt(psudata->log_flag);
	psudata->minLevel = StrToInt(psudata->log_minLevel);
	psudata->maxLevel = StrToInt(psudata->log_maxLevel);
	SetLogLevel(psudata->minLevel,psudata->maxLevel);
	
	psudata->flag = StrToInt(psudata->log_flag);
	psudata->minLevel = StrToInt(psudata->log_minLevel);
	psudata->maxLevel = StrToInt(psudata->log_maxLevel);
	SetLogLevel(psudata->minLevel,psudata->maxLevel);
	psudata->selectTimeout = StrToInt(psudata->network_selectTimeout);

	//初始化服务Socket地址信息
	unsigned int addr;
	memset(&(psudata->net_addr),0,sizeof(struct sockaddr));
	addr = ParseIPAddr(psudata->network_ipaddr,&port);
	psudata->net_addr.sin_family = AF_INET;
	psudata->net_addr.sin_addr.s_addr = htonl(addr);
	psudata->net_addr.sin_port = htons(port);

    return r;
}

void dump_buffer(char *pstr,int size)
{
	char buf[1024];
	int hexlen = sizeof(buf);

	if(pstr != NULL && size > 0){
		memset(buf,0,sizeof(buf));
		HexEncode(pstr,size,buf,&hexlen);
		_log(psus_data.log_file,psus_data.flag,buf);
	}else{
		_log(psus_data.log_file,psus_data.flag,"(null) or empty.");
	}
}

int OpenUart(psus_config *psudata)
{
	if(psudata->uart_fd > 0)
		return psudata->uart_fd;

	psudata->uart_fd = com_open(psudata->uart_port,O_RDWR);
	if(psudata->uart_fd <= 0){
		psus_error("open uart %s error:%s",psudata->uart_port,strerror(errno));
		return -1;
	}
	return psudata->uart_fd;
}

void CloseUart(psus_config *psudata)
{
	if(psudata->uart_fd > 0)
		close(psudata->uart_fd);
	psudata->uart_fd  = 0;
}

char *IP2Str(unsigned long ip)
{
	static char buf[20];
	
	sprintf(buf,"%d.%d.%d.%d",(ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,ip&0xFF);
	return buf;
}

int StartServiceSocket(psus_config *psudata)
{
	unsigned short port;
	if(psudata->net_fd > 0){
		//如果已经创建了此socket，那么直接返回避免重复创建
		return psudata->net_fd;
	}
	if ((psudata->net_fd = socket ( AF_INET , SOCK_STREAM , 0)) == -1) {
		psus_error("Service socket error:%s",strerror(errno));
		return -1;
	}
	if(!SetNonblocking(psudata->net_fd)){
		psus_warning("set socket %d nonblocking fail.",psudata->net_fd);
	}
	psus_log(10,"Socket connecting to %s:%d...",IP2Str(psudata->net_addr.sin_addr.s_addr),ntohs(psudata->net_addr.sin_port));
	if(connect(psudata->net_fd,(struct sockaddr *)&psudata->net_addr,sizeof(struct sockaddr))==-1)
	{
		psus_error("Socket connect error:%s",psudata->network_ipaddr);
		close(psudata->net_fd);
		psudata->net_fd = 0;
		return -2;
	}
	psus_log(10,"Socket connected to %s:%d...",IP2Str(psudata->net_addr.sin_addr.s_addr),ntohs(psudata->net_addr.sin_port));
	/*
	if(bind(psudata->net_fd,(struct sockaddr *)&psudata->net_addr,sizeof(struct sockaddr))==-1)
	{
		psus_error("Service bind socket error:%d",ntohs(psudata->net_addr.sin_port));
		close(psudata->net_fd);
		psudata->net_fd = 0;
		return -2;
	}
	psus_log(10,"Service socket bind to %d...",ntohs(psudata->net_addr.sin_port));
	*/
	return psudata->net_fd;
}

void CloseServiceSocket(psus_config *psudata)
{
	if(psudata->net_fd > 0){
		close(psudata->net_fd);
		psudata->net_fd = 0;
	}
}


int WaitMessageAndHandle(char *configfn,int timeout)
{
	int iRet = 0,mfd,sfd,ffd;
	fd_set rfds;
	psus_config *psudata = get_psus_data();
	
	ffd = OpenUart(psudata);
	sfd = StartServiceSocket(psudata);
	if(ffd <=0 || sfd <=0){
		psus_error("Uart or network not ready.");
		sleep(1);
		return 0;
	}
	if(ffd > 0)
        FD_CLR(ffd,&rfds);
	if(sfd > 0)
	    FD_CLR(sfd,&rfds);
	FD_ZERO(&rfds);
	if(sfd > 0)
        FD_SET(sfd, &rfds);
	if(ffd > 0)
        FD_SET(ffd, &rfds);
	struct timeval tv={5,0};
	tv.tv_sec = timeout;
	if(tv.tv_sec == -1)
		iRet = select(MAX(sfd,ffd), &rfds, NULL, NULL, NULL);
	else
		iRet = select(MAX(sfd,ffd), &rfds, NULL, NULL, &tv);
	if(0 >= iRet)	return 0;   //等待超时，进入下一次循环
	if(FD_ISSET(ffd, &rfds))
	{
		//这里处理通过UART收到的命令
		int len;
		unsigned char buf[4096];

		memset(buf,0,sizeof(buf));
		len = read(ffd,buf,sizeof(buf));
		if(len > 0 && sfd > 0){
			iRet = write(sfd,buf,len);
			#ifdef PSU_DEBUG
				printf("UART:\n");
				dump_buffer(buf,len);
			#endif
		}
	}
	else if(FD_ISSET(sfd, &rfds))
	{
		//这里处理的是服务器主动向客户端发送的消息，处理完以后客户端沿沿原途径做出回应
		int len;
		unsigned char buf[4096];

		memset(buf,0,sizeof(buf));
		len = read(sfd,buf,sizeof(buf));
		if(len > 0 && ffd > 0){
			iRet = write(ffd,buf,len);
			#ifdef PSU_DEBUG
				printf("NET:\n");
				dump_buffer(buf,len);
			#endif
		}
	}
	return iRet;
}

int main(int argc, char* argv[])
{
    char fn[MAX_PATH];
    char tmp[128];
    int i=0;
    psus_config *psudata = get_psus_data();

    if(!GetCmdParamValue(argc,argv,"config",fn))
        strcpy(fn,"/etc/psr/psuserver.conf");
    //printf("%s\n",argv[0]);
    printf("Use config file : %s\n",fn);

    LoadPsuConfig(fn);
    if(GetCmdParamValue(argc,argv,"log",tmp)){
        strcpy(psus_data.log_file,tmp);
    }
    if(CheckCmdLine(argc,argv,"d")){
        pid_t fpid; //fpid表示fork函数返回的值
        psus_log(10,"Using deamon runing parameter.");
        fpid = fork();
        if (fpid < 0){
            psus_error("error in fork!");
            return -1;
        }else if (fpid == 0){
            //子进程
            psus_log(10,"Child process runing(%d)...",getpid());
        }else{
            //父进程，服务程序退出父进程
            psus_log(10,"Child process %d runing,Parent process exit.",fpid);
            return 0;
        }
    }
    //启动服务处理循环
    //处理命令接口
    int iRet = 0;
    do{
        iRet = WaitMessageAndHandle(fn,psudata->selectTimeout);
    }while(iRet >= 0);

    psus_warning("Process %d end.",getpid());
    CloseUart(psudata);
    CloseServiceSocket(psudata);
    return 0;
}
