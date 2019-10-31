#ifndef __H_PSRS_H__
#define __H_PSRS_H__
#include "ini.h"
#include "utils.h"
#include "xLogMessage.h"

#define PSUSERVICE_TAG     "PSUSERVICE"

/*定义本文件中使用的打印日志、告警和错误的宏
*/
#define psus_log(level,fmt, args...)      LogMessage(level,__FILE__,__LINE__,PSUSERVICE_TAG,fmt,## args)
#define psus_error(fmt, args...)          LogErrorMessage(__FILE__,__LINE__,PSUSERVICE_TAG,fmt,## args)
#define psus_warning(fmt, args...)        LogWarningMessage(__FILE__,__LINE__,PSUSERVICE_TAG,fmt,## args)

#define IP2STR(ip)      (ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,ip&0xFF

/* define the config struct type */
typedef struct {
    int flag;                           //日志输出类别
    unsigned short minLevel;            //日志级别的最小值
    unsigned short maxLevel;            //日志级别的最大值
    int   selectTimeout;                //Select等待的超时时间
    struct  sockaddr_in net_addr;       //提供服务的监听地址和端口
    int uart_fd;                        //存储本地管理和接口命令的FIFO文件描述符
    int net_fd;                         //存储服务监听的Socket文件描述符

    #define CFG(s, n, default) char *s##_##n;
    #include "psuservice.def"
} psus_config;

/* create one and fill in its default values */
psus_config Psus_Config_Default = {
    #define CFG(s, n, default) default,
    #include "psuservice.def"
};

typedef struct net_handle PSUSHANDLE;

psus_config *get_psus_data(void);

#endif  //__H_PSRS_H__
