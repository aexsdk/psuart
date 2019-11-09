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
#include "rader.h"

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
    -1,
    5000,
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
    #define CFG(s, n, default) printf("%s_%s = %s\n", #s, #n, cfg->s##_##n == NULL?"":cfg->s##_##n);
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
	
	psudata->selectTimeout = StrToInt(psudata->network_selectTimeout);
	psudata->udpport = StrToInt(psudata->network_udpport);
	psudata->udpport = psudata->udpport==0?5000:psudata->udpport;

	//初始化服务Socket地址信息
	unsigned int addr;
	memset(&(psudata->net_addr),0,sizeof(struct sockaddr));
	addr = ParseIPAddr(psudata->network_ipaddr,&port);
	psudata->net_addr.sin_family = AF_INET;
	psudata->net_addr.sin_addr.s_addr = addr;
	psudata->net_addr.sin_port = htons(port);

    return r;
}

void dump_buffer(char *pstr,int size)
{
	char buf[40];
	int hexlen = sizeof(buf);
	char *p = pstr;

	while(size > 0){
		memset(buf,0,sizeof(buf));
		hexlen = sizeof(buf);
		HexEncode(p,MIN(16,size),buf,&hexlen);
		printf("\t%s \n",buf);
		p += 16;
		size -= 16;
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
	
	sprintf(buf,"%d.%d.%d.%d",ip&0xFF,(ip>>8)&0xFF,(ip>>16)&0xFF,(ip>>24)&0xFF);
	return buf;
}

int StartUdpSocket(psus_config *psudata)
{
	struct  sockaddr_in addr;
	
	if(psudata->udp_fd > 0){
		//如果已经创建了此socket，那么直接返回避免重复创建
		return psudata->udp_fd;
	}
	if ((psudata->udp_fd = socket ( AF_INET , SOCK_DGRAM , 0)) == -1) {
		psus_error("Udp socket error:%s",strerror(errno));
		return -1;
	}

	/*if(!SetNonblocking(psudata->udp_fd)){
		psus_warning("set socket %d nonblocking fail.",psudata->udp_fd);
	}*/

	
	memset(&addr,0,sizeof(struct sockaddr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(psudata->udpport);
	if(bind(psudata->udp_fd,(struct sockaddr *)&addr,sizeof(struct sockaddr))==-1)
	{
		psus_error("Udp bind socket error:%d",psudata->udpport);
		close(psudata->udp_fd);
		psudata->udp_fd = 0;
		return -2;
	}
	psus_log(10,"Udp socket bind to %d...",psudata->udpport);
	
	return psudata->udp_fd;
}

void CloseUdpSocket(psus_config *psudata)
{
	if(psudata->udp_fd > 0){
		close(psudata->udp_fd);
		psudata->udp_fd = 0;
	}
}

int StartTcpSocket(psus_config *psudata)
{
	if(psudata->tcp_fd > 0){
		//如果已经创建了此socket，那么直接返回避免重复创建
		return psudata->tcp_fd;
	}
	if ((psudata->tcp_fd = socket ( AF_INET , SOCK_STREAM , 0)) == -1) {
		psus_error("Service socket error:%s",strerror(errno));
		return -1;
	}

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	socklen_t len = sizeof(struct timeval);
	if(setsockopt( psudata->tcp_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len ) == -1){
		psus_error("Socket setsockopt error [%d]: %s",errno,strerror(errno));
	}

	if(!SetNonblocking(psudata->tcp_fd)){
		psus_warning("set socket %d nonblocking fail.",psudata->tcp_fd);
	}
	
	psus_log(10,"Socket connecting to %s:%d...",IP2Str(psudata->net_addr.sin_addr.s_addr),ntohs(psudata->net_addr.sin_port));
	if(connect(psudata->tcp_fd,(struct sockaddr *)&psudata->net_addr,sizeof(struct sockaddr))<0)
	{
		if(errno != EINPROGRESS){
			psus_error("Socket connect %s error [%d]: %s",psudata->network_ipaddr,errno,strerror(errno));
			close(psudata->tcp_fd);
			psudata->tcp_fd = 0;
			return -2;
		}
	}else{
		psus_log(10,"Socket connected to %s:%d...",IP2Str(psudata->net_addr.sin_addr.s_addr),ntohs(psudata->net_addr.sin_port));
	}
	return psudata->tcp_fd;
}

void CloseTcpSocket(psus_config *psudata)
{
	if(psudata->tcp_fd > 0){
		close(psudata->tcp_fd);
		psudata->tcp_fd = 0;
	}
}

int TcpSocketStatus(psus_config *psudata)
{
	int err = -1;
	socklen_t len = sizeof(int);
	
	if(psudata->tcp_fd > 0){
		if(getsockopt(psudata->tcp_fd, SOL_SOCKET, SO_ERROR ,&err, &len) < 0 )
		{
			CloseTcpSocket(psudata);
			psus_error("Tcp getsockopt errno:%d %s\n", errno, strerror(errno));
			return -2;
		}
		if (err)
		{
			errno = err;
			CloseTcpSocket(psudata);
			psus_error("Tcp socket connect error:%d\n", err);
			return -3;
		}
		return 0;
	}
	return -1;
}

static tcp_connected = 0;
int WaitMessageAndHandle(char *configfn,int timeout)
{
	int iRet = 0,tcpFd,udpFd,uartFd;
	fd_set rfds,wfds;
	psus_config *psudata = get_psus_data();
	int len;
	unsigned char buf[4096];
	
	uartFd = OpenUart(psudata);
	tcpFd = StartTcpSocket(psudata);
	udpFd = StartUdpSocket(psudata);
	if(uartFd <=0 && udpFd <=0){
		psus_error("Uart or network not ready.");
		sleep(1);
		return 0;
	}

	FD_ZERO(&rfds);
	FD_SET(0,&rfds);
	if(tcpFd > 0){
		FD_SET(tcpFd, &rfds);
	}
	if(udpFd > 0){
		FD_SET(udpFd, &rfds);
	}
	if(uartFd > 0)
		FD_SET(uartFd, &rfds);

	FD_ZERO(&wfds);
	if(tcp_connected == 0){
		FD_SET(tcpFd,&wfds);
	}

	struct timeval tv={5,0};
	tv.tv_sec = timeout;
	//psus_log(10,"Waiting for DATA(uid=%d,nid=%d,%d,max=%d)...",uartFd,tcpFd,udpFd,max_value(3,tcpFd,udpFd,uartFd));
	if(tv.tv_sec == -1)
		iRet = select(max_value(3,tcpFd,udpFd,uartFd)+1, &rfds, &wfds, NULL, NULL);
	else
		iRet = select(max_value(3,tcpFd,udpFd,uartFd)+1, &rfds, &wfds, NULL, &tv);
	if(0 >= iRet){
		if(FD_ISSET(tcpFd, &rfds)){
			psus_log(10,"Tcp select =%d",iRet);
			CloseTcpSocket(psudata);
		}
		if(FD_ISSET(uartFd, &rfds))
			CloseUart(psudata);
		if(FD_ISSET(udpFd, &rfds))
			CloseUdpSocket(psudata);
		return 0;   //等待超时，进入下一次循环
	}
	if(FD_ISSET(tcpFd,&wfds)){
		if(TcpSocketStatus(psudata) == 0){
			printf("TCP Connected\n");
			tcp_connected = 1;
		}else{
			printf("TCP Connect fail\n");
		}
	}
	if(FD_ISSET(uartFd, &rfds))
	{
		//这里处理通过UART收到的命令
		//_log(psus_data.log_file,psus_data.flag,"UART:\n");
		memset(buf,0,sizeof(buf));
		len = read(uartFd,buf,sizeof(buf));
		if(len > 0 && tcpFd > 0 && tcp_connected != 0){
			#ifdef RADER_PASS
				iRet = write(tcpFd,buf,len);
			#else
				rader_recv(tcpFd,buf,len);
			#endif
			printf("UART:%d\n",len);
			#ifdef PSU_DEBUG
				dump_buffer(buf,len);
			#endif
		}
	}
	if(FD_ISSET(tcpFd, &rfds))
	{
		//这里处理的是服务器主动向客户端发送的消息，处理完以后客户端沿沿原途径做出回应
		//_log(psus_data.log_file,psus_data.flag,"TCPNET:\n");
		memset(buf,0,sizeof(buf));
		len = read(tcpFd,buf,sizeof(buf));
		if(len > 0 && uartFd > 0){
			#ifdef RADER_PASS
				iRet = write(uartFd,buf,len);
			#else
				rader_cmd(uartFd,buf,len);
			#endif
			printf("TCPNET:%d\n",len);
			//#ifdef PSU_DEBUG
				dump_buffer(buf,len);
			//#endif
		}
	}
	if(FD_ISSET(udpFd, &rfds))
	{
		//这里处理的是服务器主动向客户端发送的消息，处理完以后客户端沿沿原途径做出回应
		//_log(psus_data.log_file,psus_data.flag,"UDPNET:\n");
		memset(buf,0,sizeof(buf));
		len = read(udpFd,buf,sizeof(buf));
		if(len > 0 && udpFd > 0){
			#ifdef RADER_PASS
				iRet = write(uartFd,buf,len);
			#else
				rader_cmd(uartFd,buf,len);
			#endif
			printf("UDPNET:%d\n",len);
			//#ifdef PSU_DEBUG
				dump_buffer(buf,len);
			//#endif
		}
	}
	if(FD_ISSET(0,&rfds))
	{
		memset(buf,0,sizeof(buf));
		scanf("%s",buf);
		rader_cmd_from_stdin(uartFd,buf,strlen(buf));
		//write(uartFd,buf,strlen(buf));
	}
	return iRet;
}

void signal_func(int sign_no)
{ 
	if(sign_no == SIGINT){ 
		printf("\nI have get SIGINT\n"); 
		CloseUart(get_psus_data());
		CloseUdpSocket(get_psus_data());
		CloseTcpSocket(get_psus_data());
		exit(0);
	}else if(sign_no == SIGQUIT){
		printf("\nI have get SIGQUIT\n"); 
		CloseUart(get_psus_data());
		CloseUdpSocket(get_psus_data());
		CloseTcpSocket(get_psus_data());
		exit(0);
	}
}

int main(int argc, char* argv[])
{
    char fn[MAX_PATH];
    char tmp[128];
    int i=0;
    psus_config *psudata = get_psus_data();

    if(!GetCmdParamValue(argc,argv,"config",fn))
        strcpy(fn,"/etc/psu/psuserver.conf");
    //printf("%s\n",argv[0]);
    mkdir("/etc/psu",0777);
    printf("Use config file : %s\n",fn);

    LoadPsuConfig(fn);
	signal(SIGINT,signal_func); 
	signal(SIGQUIT,signal_func); 

    if(GetCmdParamValue(argc,argv,"log",tmp)){
        strcpy(psus_data.log_file,tmp);
    }
    memset(tmp,0,sizeof(tmp));
    ExtractFilePath(psus_data.log_file,tmp);
    mkdir(tmp,0777);
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
    rader_init();
    //启动服务处理循环
    //处理命令接口
    int iRet = 0;
    do{
        iRet = WaitMessageAndHandle(fn,psudata->selectTimeout);
    }while(iRet >= 0);

    psus_warning("Process %d end.",getpid());
    CloseUart(psudata);
    CloseUdpSocket(psudata);
    CloseTcpSocket(psudata);
    return 0;
}
