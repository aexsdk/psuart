/**
 * 公用函数库
 */
#include <stdio.h>      /*标准输入输出定义*/
#include <stdlib.h>     /*标准函数库定义*/
#include <unistd.h>     /*Unix标准函数定义*/
#include <fcntl.h>      /*文件控制定义*/
#include <termios.h>    /*PPSIX终端控制定义*/
#include <errno.h>      /*错误号定义*/
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <dirent.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "utils.h"

typedef unsigned char BYTE;

int HexEncodeGetRequiredLength(int nSrcLen)
{
	return 2 * nSrcLen + 1;
}

int HexDecodeGetRequiredLength(int nSrcLen)
{
	return nSrcLen/2;
}

int HexEncode(const unsigned char *pbSrcData, int nSrcLen, char *szDest, int *pnDestLen)
{
	int nRead = 0;
	int nWritten = 0;
	BYTE ch;
	static const char s_chHexChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'A', 'B', 'C', 'D', 'E', 'F'};

	if (!pbSrcData || !szDest || !pnDestLen)
	{
		return FALSE;
	}

	if(*pnDestLen < HexEncodeGetRequiredLength(nSrcLen))
	{
		return FALSE;
	}

	while (nRead < nSrcLen)
	{
		ch = *pbSrcData++;
		nRead++;
		*szDest++ = s_chHexChars[(ch >> 4) & 0x0F];
		*szDest++ = s_chHexChars[ch & 0x0F];
		nWritten += 2;
	}

	*pnDestLen = nWritten;

	return TRUE;
}

#define HEX_INVALID ((char)-1)

//Get the decimal value of a hexadecimal character
char GetHexValue(char ch)
{
	if (ch >= '0' && ch <= '9')
		return (ch - '0');
	if (ch >= 'A' && ch <= 'F')
		return (ch - 'A' + 10);
	if (ch >= 'a' && ch <= 'f')
		return (ch - 'a' + 10);
	return HEX_INVALID;
}

int HexDecode(const char *pSrcData, int nSrcLen, unsigned char *pbDest, int* pnDestLen)
{
	int nRead = 0;
	int nWritten = 0;

	if (!pSrcData || !pbDest || !pnDestLen)
	{
		return 0;
	}

	if(*pnDestLen < HexDecodeGetRequiredLength(nSrcLen))
	{
		return 0;
	}

	while (nRead < nSrcLen)
	{
		char ch1,ch2;

		if((char)*pSrcData == '\r' || (char)*pSrcData == '\n')break;
		ch1 = GetHexValue((char)*pSrcData++);
		ch2 = GetHexValue((char)*pSrcData++);
		if ((ch1==HEX_INVALID) || (ch2==HEX_INVALID))
		{
			return 0;
		}
		*pbDest++ = (unsigned char)(16*ch1+ch2);
		nWritten++;
		nRead += 2;
	}

	*pnDestLen = nWritten;
	return 1;
}

unsigned char utils_crc(const unsigned char *buf,int size)
{
	unsigned char ret = 0;
	int i;
	
	for(i=0; i < size;i++)
		ret += (unsigned char)(buf[i]);
	return ret;
}

int utils_strincmp(const char *s1,const char *s2,int n)
{
	/* case insensitive comparison */
	int d;
	
	if(n == 0){
	    n = strlen(s1);
	    if(n < strlen(s2))
	        n = strlen(s2);
	}
	while (--n >= 0) {
#ifdef ASCII_CTYPE
	  if (!isascii(*s1) || !isascii(*s2))
	    d = *s1 - *s2;
	  else
#endif
	    d = (tolower((unsigned char)*s1) - tolower((unsigned char)*s2));
	  if ( d != 0 || *s1 == '\0' || *s2 == '\0' )
	    return d;
	  ++s1;
	  ++s2;
	}
	return(0);
}

/*
	function SplitArguments split string argument to a array.
	parameters:
		arginfo	: Asterisk introduction arguments list.It is compart by char '|' .
		argArray : a string array,it will store each argument.
		maxsize	: the max size of argArray.
		spliter : it is a char witch arginfo used.default is '|'.
*/
int split_arguments(const char *arginfo,char argArray[][MAX_ARG_LEN],int maxsize,char spliter)
{
	int index =0;
	char *ps, *pe;

	//printf("\nSplitArguments  %s\n",arginfo);
	ps = (char *)arginfo;//strchr(arginfo,spliter);
	while(ps && index < maxsize)
	{
		if(*ps == spliter)ps++;
		pe = strchr(ps,spliter);
		if(pe){
			int len = pe-ps;
			strncpy(argArray[index++],ps,len < MAX_ARG_LEN? len : MAX_ARG_LEN-1);
			ps = pe + 1;//strchr(pe,spliter);
		}else{
			if(strlen(ps) > 0)
				strncpy(argArray[index++],ps,MAX_ARG_LEN-1);
			break;
		}
	}
	return index;
}

unsigned int get_ip_addr(char *ipaddr)
{
	unsigned int iip = 0;
	unsigned char *ip = (unsigned char *)&iip;
	char arg[4][MAX_ARG_LEN];
	memset(arg,0,4*MAX_ARG_LEN);
	if(utils_strincmp(ipaddr,"*",strlen(ipaddr)) == 0){
	    iip = INADDR_ANY;
	}else{
	    split_arguments(ipaddr,arg,4,'.');
	    ip[0] = atoi(arg[0]);
	    ip[1] = atoi(arg[1]);
	    ip[2] = atoi(arg[2]);
	    ip[3] = atoi(arg[3]);
	}
	return iip;
}

/*
	**************************************************************************
	概述
		解析网络传输地址
	**************************************************************************
*/
unsigned int ParseIPAddr(char *cAddr,unsigned short *port)
{
	char Buf[50];
	char *p=Buf,*ppos=NULL;

	strcpy(Buf,cAddr);
	ppos = strchr(p,':');
	if(ppos == NULL)
		*port = 0;
	else{
		*port = StrToInt(ppos+1);
		*ppos = '\0';
	}
	return get_ip_addr(Buf);
}

//返回BreakChar后的字符串
EZUTILS_API	char * LeftString(char *Dest,char *aSource,char BreakChar)
{
	char *p = aSource;
	while (*p && *p != ' ' && *p!=13 && *p!=BreakChar)p++;
	strncpy(Dest,aSource,p-aSource);
	if (*p) p++;
	return p;
}

EZUTILS_API	int RemovePrefix(char *Result,char *Source,char *DecPrefix)
{
	int SourceLen,DecPrefixLen,sPos,bfound=FALSE;
	SourceLen = (int)strlen(Source);
	sPos = DecPrefixLen = strlen(DecPrefix);
	if(utils_strincmp(DecPrefix,"*a",0)==0)
	{
		//不删除前缀
		sPos = 0;
		strcpy(Result,Source+sPos);
		return TRUE;
	}else{
	    int i;
		for(i = 0; i<DecPrefixLen;i++ )
		{
			if (i >= SourceLen) break;//如果到源号码的尾部，跳出循环
			if (DecPrefix[i] == '*'){
				sPos = SourceLen;
				break;
			}
			if ((Source[i] != DecPrefix[i]) && (DecPrefix[i] != '?'))
			{
				sPos = 0;
				break;
			}
		}
	}
	strcpy(Result,Source+sPos);
	return sPos>0;
}

EZUTILS_API	char * Addprefix(char *Result,char *Source,char *AddPrefix)
{
	strcpy(Result ,AddPrefix);
	strcat(Result,Source);
	return Result;
}

EZUTILS_API	char * ExtractFilePath(const char *FileName,char *path)
{
	char *p = (char *)strrchr(FileName,PATHSPLIT);
	int len = p == NULL ? strlen(FileName) : (int)(p - FileName)+1;
	strncpy(path,FileName,len);
	path[len] = '\0';
	return path;
}

EZUTILS_API	char * ExtractFileName(const char *FileName,char *fn)
{
	if(!FileName)
	{
		strcpy(fn,"");
	}else{
		char *p = (char *)strrchr(FileName,PATHSPLIT);
		if(p)
			strcpy(fn,p+1);
		else
			strcpy(fn,FileName);
	}
	return fn;
}

EZUTILS_API	char * ChangeFileExt(char *src,char *ext)
{
	char *p = strrchr(src,'.');
	if(p)*p = '\0';
	strcat(src,ext);
	return src;
}

EZUTILS_API	char * ChangeFilePath(char * src,char *path)
{
	char fn[255];
	ExtractFileName(src,fn);
	strcpy(src,path);
	if(path[strlen(path)-1] != '\\')
		strcat(src,PATHSPLITS);
	strcat(src,fn);
	return src;
}

/*
	说明：
		比较版本号，版本号为x.x.x.x格式。这需要调用GetFileVersionS函数来获得。
	参数：
		char *newversion : 版本号1
		char *oldversion : 版本号2
	返回值：
		1 ： 版本号1大于版本号2
		0 ： 两个版本号相同
		-1 ： 版本号1小于版本号2
*/
int VersionCompare(char* newversion,char* oldversion)
{
	return 0;
}

EZUTILS_API	char *Copy(char *dstr,const char *sstr,int from,int count)
{
	int i = 0;
	
	if(from >= (int)strlen(sstr) || count <=0)strcpy(dstr,"");
	else{
		while(from+i<(int)strlen(sstr) && i<count){
			dstr[i] = sstr[i + from];
			i++;
		}
		dstr[i] = '\0';
	}
	return dstr;
}

/*
	*********************************************************************
	概述
		去掉字符串前后的空格、制表符、换行符等
	*********************************************************************
*/
EZUTILS_API	char *Trim(char *dstr,const char *str)
{
	int iStart=0,iEnd = (int)strlen(str)-1;
	while(str[iStart] == ' '|| str[iStart]=='\t'||str[iStart]=='\n'||str[iStart]=='\r')iStart++;
	while(str[iEnd] ==' '|| str[iEnd]=='\t'||str[iEnd]=='\n'||str[iEnd]=='\r')iEnd--;
	return Copy(dstr,str,iStart,iEnd-iStart+1);
}
/*
	********************************************************************
	概数
		获得Name=Value字符串的Value部分，如果不包含=号则去全部字符串
	********************************************************************
*/
EZUTILS_API	void SplitStringValue(char *str,char *buf)
{
    char spliter[] = "=#|&@";
	char *ps=str,*ppos=NULL,*p = spliter;

    while(*p && (ppos == NULL)){
        ppos = strchr(ps,*p);
        p++;
    }
	if(ppos == NULL){
		strcpy(buf,str);
	}else{
		strcpy(buf,ppos+1);
	}
}
/*
********************************************************************
概数
	获得Name=Value字符串的Key&Value部分，如果不包含=号则Key取全部字符串
********************************************************************
*/
EZUTILS_API	void SplitStringKeyValue(char *str,char *key,char * value)
{
    char spliter[] = "=#|&@";
	char *ps=str,*ppos=NULL,*p = str;

    while(*p && (ppos == NULL)){
        ppos = strchr(spliter,*p);
        p++;
    }
	if(*p == '\0'){
		strcpy(value,"");
		strcpy(key,str);
	}else{
		strncpy(key,str,p-str-1);
		key[p-str-1] = 0;
		strcpy(value,p);
	}
}

/*
*****************************************************************
概述
判断ip地址是否为组播地址
参数
ip			:	4字节的ip地址
*****************************************************************
*/
EZUTILS_API	int IsMulticastIP(unsigned long ip)
{
	unsigned char iFirst;
	char *p = (char *)&ip;

	iFirst = p[0];
	return iFirst >=224 && iFirst <= 240;
}

/*
*****************************************************************
概述
判断是否为有效的ip
参数
ip			:	4字节的ip地址
*****************************************************************
*/
EZUTILS_API	int IsValidIP(unsigned long ip)
{
	unsigned char iFirst, iSecond;
	char *p = (char *)&ip;

	iFirst = p[0];
	iSecond = p[1];

	if (iFirst == 127 || iFirst >= 224)
	{
		return FALSE;
	}
	else if (!iFirst)
	{
		if (!iSecond)
		{
			return FALSE;
		}
	}

	return TRUE;
}

/*
*****************************************************************
概述
判断是否为私有的ip
参数
ip			:	4字节的ip地址
*****************************************************************
*/
EZUTILS_API	int	IsPrivateIP(unsigned long ip)
{
	unsigned char iFirst, iSecond;
	char *p = (char *)&ip;

	iFirst = p[0];
	iSecond = p[1];

	if (iFirst == 10)	return TRUE;
	if (iFirst == 172 &&  (iSecond >= 16 && iSecond <= 32))return TRUE;
	if (iFirst == 192 && iSecond == 168)return TRUE;
	return FALSE;
}

/*
*****************************************************************
概述
判断是否为公网的ip
参数
ip			:	4字节的ip地址
*****************************************************************
*/
EZUTILS_API	int IsPublicIP(unsigned long ip)
{
	return IsValidIP(ip)  &&  (!IsPrivateIP(ip));
}

int GetTickCount(void)
{
     struct timeval tv;
     gettimeofday( &tv, NULL );
     // this will rollover ~ every 49.7 days
     return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

/**
考虑了8-10-16进制的字符串转整数的函数
*/
int StrToInt(char * str)
{
   int value = 0;
   int sign = 1;
   int radix;
 
   if(str == NULL)return 0;
   if(*str == '-'){
      sign = -1;
      str++;
   }
   if(*str == '0' && (*(str+1) == 'x' || *(str+1) == 'X')){
      radix = 16;
      str += 2;
   }else if(*str == '0'){
      radix = 8;
      str++;
   }else{
      radix = 10;
    }
   while(*str)
   {
      if(radix == 16)
      {
        if(*str >= '0' && *str <= '9')
           value = value * radix + *str - '0';
        else if(*str >= 'a' && *str <= 'f')
           value = value * radix + (*str | 0x20) - 'a' + 10;
        else  if(*str >= 'A' && *str <= 'F')
           value = value * radix + (*str | 0x20) - 'A' + 10;
      }else{
      	if(*str >= '0' && *str <= '9')
        	value = value * radix + *str - '0';
      }
      str++;
   }
   return sign*value;
}

int ParseCmdParamValue(const char *cmd,const char *param,char *value)
{
    char *str=(char *)cmd,*strp=(char *)cmd,sw[100]="";
    
    memset(sw,0,sizeof(sw));
    while(*str==' ' || *str=='/' || *str=='-' || *str == '\\')str++;
    strp = str;
    while((*strp!='\0')&&(*strp != ':'))strp++;
    strncpy(sw,str,strp-str);
    switch(*strp)
    {
    case '\0':
        strcpy(value,"");			//如果后面没有参数，则使用空值作为参数
        break;
    case ':':
        strcpy(value,strp+1);
        break;
    default:
        strcpy(value,"");
        break;
    }
    if (utils_strincmp(sw,param,0)==0){
        return TRUE;
    }
    return FALSE;
}

int CheckCmdLine(int argc, char* argv[],char *sw)
{
    int i;
    char *str;
    for(i=1;i<argc;i++)
    {
        str = argv[i];
        while(*str==' ' || *str=='/' || *str=='-' || *str == '\\')str++;
        if (utils_strincmp(str,sw,0)==0)
            return TRUE;
    }
    return FALSE;
}

int GetCmdParamValue(int argc, char* argv[],char *param,char* paramValue)
{
    int i;
    char *str,*strp,sw[100]="";
    strcpy(paramValue,"");
    for(i=1;i<argc;i++)
    {
        str = argv[i];
        memset(sw,0,sizeof(sw));
        //printf("argv[%d]=%s\r\n",i,argv[i]);
        while(*str==' ' || *str=='/' || *str=='-' || *str == '\\')str++;
        strp = str;
        while((*strp!='\0')&&(*strp != ':'))strp++;
        strncpy(sw,str,strp-str);
        switch(*strp)// == _T('\0'))strcpy(paramValue,"");
        {
        case '\0':
            if(i+1 < argc)
            {
                if((argv[i+1][0] != '/') && (argv[i+1][0] != '-'))
                    strcpy(paramValue,argv[i+1]);	//如果后面还有参数，且参数不是以'/' '-'开头时，则使用下一个参数作为参数值
                else
                    strcpy(paramValue,"");			//如果后面没有参数，则使用空值作为参数
            }else{
                strcpy(paramValue,"");			//如果后面没有参数，则使用空值作为参数
            }
            break;
        case ':':
            strcpy(paramValue,strp+1);
            break;
        default:
            break;
        }
        //printf("\t%s=%s\r\n",sw,paramValue);
        if (utils_strincmp(sw,param,0)==0){
            return TRUE;
        }
    }
    return FALSE;
}

int  SetNonblocking(int  fd)
{
    int  opts;

    opts = fcntl(fd,F_GETFL);
    if (opts >= 0 )
    {
        opts  =  opts | O_NONBLOCK;
        if(fcntl(fd,F_SETFL,opts) >= 0 )
        {
            return TRUE;
        }
    }
    return FALSE;
}

int get_ip(char *localip) 
{
    int i = 0;
    struct hostent *hp;
    struct utsname localname;
    uname(&localname);

    hp = gethostbyname(localname.nodename);
    for (i = 0; hp->h_addr_list[i] != NULL ; ++i){
        strcpy(localip, inet_ntoa(*(struct in_addr*) hp->h_addr_list[i]));
        //psrlib_log(10,"IP:%s",localip);
    }
    return 0;
}

void DisplaySocketInfo(int sockfd)
{
    struct sockaddr_in *addr;
    struct ifreq ifr;
    char address[100];
    
    get_ip(address);
    //printf("IP:%s",address);
    bzero(&ifr, sizeof(ifr));
    strcpy(ifr.ifr_name,"en0");
    if(ioctl(sockfd,SIOCGIFADDR,&ifr) == -1){
        perror("ioctl error");
        return;
    }

    addr = (struct sockaddr_in *)&(ifr.ifr_addr);
    if(addr){
        //psrlib_log(10,"inet addr: %s:%d ",inet_ntoa(addr->sin_addr),ntohs(addr->sin_port));
    }

    if(ioctl(sockfd,SIOCGIFBRDADDR,&ifr) == -1){
        //psrlib_error("ioctl error");
    }
    addr = (struct sockaddr_in *)&ifr.ifr_broadaddr;
    //psrlib_log(10,"broad addr: %s ",(char*)inet_ntoa(addr->sin_addr));

    if(ioctl(sockfd,SIOCGIFNETMASK,&ifr) == -1){
        //psrlib_error("ioctl error");
    }
    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    //psrlib_log(10,"inet mask: %s ",(char*)inet_ntoa(addr->sin_addr));
}


/**
 * 串口操作的函数库
 */

/*
 * 设置串口的波特率参数
 */
static void com_set_speed(int fd, int speed) {
	int i;
	int status;
	struct termios Opt;
	int speed_arr[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300, B38400,
			B19200, B9600, B4800, B2400, B1200, B300, };
	int name_arr[] = {115200, 38400, 19200, 9600, 4800, 2400, 1200, 300, 38400, 19200,
			9600, 4800, 2400, 1200, 300, };

	//检查串口是否打开如果没有打开则返回，什么也不做
	if(fd<=0){
		return;
	}
	tcgetattr(fd, &Opt); //用来得到机器原端口的默认设置
	for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++) {
		if (speed == name_arr[i]) {
			tcflush(fd, TCIOFLUSH); //刷新输入输出缓冲
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status = tcsetattr(fd, TCSANOW, &Opt);
			if (status != 0)
				perror("set com speed error:tcsetattr.");
			tcflush(fd, TCIOFLUSH);
			return;
		}
	}
}

/**
 * 设置串口优先级
 *@param  fd     类型  int  打开的串口文件句柄*
 *@param  databits 类型  int 数据位   取值 为 7 或者8*
 *@param  stopbits 类型  int 停止位   取值为 1 或者2*
 *@param  parity  类型  int  效验类型 取值为N,E,O,,S
 */
static int com_set_parity(int fd, int databits, int stopbits, char parity) {
	struct termios options;

	//检查串口是否打开如果没有打开则返回，什么也不做
	//__android_log_print(ANDROID_LOG_INFO, "utils", "com_set_parity(%d,数据位%d,停止位%d,校验类型%c)",fd,databits,stopbits,parity);
	if(fd<=0){
		return FALSE;
	}
	if (tcgetattr(fd, &options) != 0) {
		perror("SetupSerial 1");
		return FALSE;
	}
	options.c_cflag &= ~CSIZE;
	switch (databits) /*设置数据位数*/
	{
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		perror("Unsupported data size\n");
		return FALSE;
	}
	switch (parity) {
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB; //Clear parity enable
		options.c_iflag &= ~INPCK; //Enable parity checking
		break;
	case 'E':
		options.c_cflag |= PARENB; // Enable parity
		options.c_cflag &= ~PARODD; // 转换为偶效验
		options.c_iflag |= INPCK; // Disnable parity checking
		break;
	default:
		perror("Unsupported parity\n");
		return FALSE;
	}

	/* 设置停止位*/
	switch (stopbits) {
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		perror("Unsupported stop bits\n");
		return FALSE;
	}

	options.c_cc[VTIME] = 0; //150; // 15 seconds
	options.c_cc[VMIN] = 0;
	//options.c_cc[VMIN] = 1;                  //read()到一个char时就返回
	tcflush(fd, TCIFLUSH); //Update the options and do it NOW

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN);
	options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	options.c_oflag &= ~(OPOST);

	if (tcsetattr(fd, TCSANOW, &options) != 0) //使端口属性设置生效
	{
		perror("SetupSerial 3");
		return FALSE;
	}
	tcflush(fd, TCIOFLUSH);

	return TRUE;
}

/**
 * 打开串口的函数
 * @param Dev  串口字符串，字符串格式为:com=/dev/ttyUSB0(串口设备字符串),s=9600(波特率),p=N(奇偶校验),b=1(停止位),d=8(数据位数)
 * @return
 * 		返回串口句柄，如果失败则返回值<=0
 */
int com_open(char *dev,int flag) {
	int fd;
	char arg[5][MAX_ARG_LEN];
	char *com="/dev/ttyACM0",*s="115200",*p="N",*b="1",*d="8";

	memset(arg,0,5*MAX_ARG_LEN);
	int argc = split_arguments(dev,arg,5,',');

	switch(argc){
	case 0:
		break;
	case 1:
		com = arg[0];
		break;
	case 2:
		com = arg[0];
		s = arg[1];
		break;
	case 3:
		com = arg[0];
		s = arg[1];
		p = arg[2];
		break;
	case 4:
		com = arg[0];
		s = arg[1];
		p = arg[2];
		b = arg[3];
		break;
	default:
		com = arg[0];
		s = arg[1];
		p = arg[2];
		b = arg[3];
		d = arg[4];
		break;
	}
	fd = open(com, flag | O_NOCTTY | O_NDELAY);

	if (fd<=0) {
		_log("utils","Can't Open Serial Port %s:%s\n",com,strerror(errno));
	} else {
		//int pn = (p[0] != 'N') || (p[0] != 'n');
		char pn= p[0];
		//__android_log_print(ANDROID_LOG_INFO, "utils", "波特率 %s",s);
		com_set_speed(fd, atoi(s));
		if (com_set_parity(fd, atoi(d), atoi(b), pn)==FALSE)
		{
			perror("设置串口参数出错");
			com_close(fd);
			return 0;
		}
	}
	return fd;
}

/**
 * 枚举所有可用串口的函数,标准C中的文件结构中文件名是13个字符。
 * @param filter 查找串口的过滤字符串，如"/dev/ttyUSB*"表示查找所有以ttyUSB开头的设备
 * @param  ofc  回调函数
 * @param param 调用回调函数时提供的上下文相关的参数
 */
void each_comm(char *filter,ON_FIND_COMM ofc,void *param)
{
	/*struct ffblk ff;
	int done;

	if(ofc == NULL)return;
	done = findfirst(filter,&ff,0);
	while(!done)
	{
		if(ff.ff_attrib&FA_DIREC != FA_DIREC)
			ofc(param,ff.ff_name);
		done=findnext(&ff);
	}*/
}

void com_close(int fd)
{
	//检查串口是否打开如果没有打开则返回，什么也不做
	if(fd>0){
		if(close(fd)==-1)
			_log( "utils","Close Serial Port:%s\n",strerror(errno));
	}
}
/**
 * 从串口接收信息的函数
 * @param fd  串口句柄
 * @param buf 存放接收内容的缓冲区
 * @param maxLen 存放接收内容缓冲区的大小
 */
int com_recive(int fd,char *buf,int maxLen,int timeout)
{
	fd_set rfds;
	int len=0,r=0;
	struct timeval tv;

	//检查串口是否打开如果没有打开则返回，什么也不做
	if(fd<=0){
		return -1;
	}

	FD_ZERO(&rfds);
	FD_SET(fd,&rfds);
	//_log( "utils", "Select %d",fd);
	if(timeout == -1){
		r = select(fd+1,&rfds,NULL,NULL,NULL);
	}else{
		memset(&tv,0,sizeof(tv));

	    tv.tv_sec = 0;
	    tv.tv_usec = timeout*1000;//1000000us = 1s
		r = select(fd+1,&rfds,NULL,NULL,&tv);
	}
	if(r==-1){
		//发生错误
		tcflush(fd, TCIOFLUSH);
		//_log( "utils", "Select return %d(timeout=%d)",r,timeout);
		return r;
	}else if (r==0)
	{
		//等待超时
		tcflush(fd, TCIOFLUSH);
		//_log( "utils", "Select return %d(timeout=%d)",r,timeout);
		return r;
    }else{
        if(FD_ISSET(fd,&rfds)){
        	len = read(fd,buf,maxLen);
        	if(len > 0){
        		#if 0
					char hexbuf[512];
					int dlen=sizeof(hexbuf);

					memset(hexbuf,0,sizeof(hexbuf));
					HexEncode(buf,len,hexbuf,&dlen);
					_log( "utils", "Recive data(%d) from fd=%d,hexbuf=%s",len,fd,hexbuf);
        		#endif
        	}
        	return len;
        }else{
        	return -1;
        }
    }
	return r;
}

int com_write(int fd,char *buf,int len)
{
	//检查串口是否打开如果没有打开则返回，什么也不做
	if(fd<=0){
		return -1;
	}
	#if 0
	{
		char hexbuf[512];
		int dlen=sizeof(hexbuf);

		memset(hexbuf,0,sizeof(hexbuf));
		HexEncode(buf,len,hexbuf,&dlen);
		//_log( "utils", "Send data(%d) from fd=%d,hexbuf=%s",len,fd,hexbuf);
	}
	#endif
	return write(fd,buf,len);
}

